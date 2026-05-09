/*
 * ============================================================
 * FRIENDSHIP LAMP — MSPM0G3507 Final Integrated Code
 * ============================================================
 *
 * IDENTICAL FOR BOTH LAMP 1 AND LAMP 2
 *
 * WORKFLOW:
 *   1. Button pressed on PA13 (active HIGH, debounced)
 *   2. Sends "BTN\r\n" to ESP32 via UART (PA10 TX)
 *   3. When ESP32 sends "ACTIVE\r\n" back (PA11 RX):
 *      → Lights SK6812 LED strip on PA27 for 15 seconds
 *      → After 15 seconds LEDs turn off automatically
 *
 * WIRING:
 *   PA10 (TX)  → ESP32 GPIO3 (RX)
 *   PA11 (RX)  ← ESP32 GPIO1 (TX)
 *   PA27       → SK6812 LED strip DIN (white wire)
 *   PA13       ← Button switch pin 1
 *   3.3V       → Button switch pin 2 (active HIGH source)
 *   5V         → Button LED+, LED strip VDD (red wire)
 *   GND        → Button LED-, LED strip GND (black wire), ESP32 GND
 *
 * J21/J22 JUMPERS: must be in BP position
 * ============================================================
 */

#include "ti_msp_dl_config.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* ── LED Strip (SK6812 NeoPixel) ────────────────────────── */
#define LED_PORT         GPIOA
#define LED_PIN          DL_GPIO_PIN_27
#define NUM_LEDS         30
#define LED_ON_MS        15000UL   /* 15 seconds */

/* Warm pink color when active */
#define LED_R            255
#define LED_G            50
#define LED_B            120

/* ── Button (PA13, active HIGH) ─────────────────────────── */
#define DEBOUNCE_CYCLES  1600000UL  /* ~50ms at 32MHz */

/* ── UART RX buffer ─────────────────────────────────────── */
#define RX_BUF_SIZE      64
volatile char     rxBuffer[RX_BUF_SIZE];
volatile uint32_t rxIndex   = 0;
volatile bool     rxNewLine = false;

/* ── State ───────────────────────────────────────────────── */
static bool       ledsActive    = false;
static uint32_t   ledOnStartMs  = 0;

/* ── Millisecond tick ────────────────────────────────────── */
static volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void)
{
    ms_ticks++;
}

static void systick_init(void)
{
    SysTick_Config(32000);  /* 32MHz / 1000 = 32000 cycles per ms */
}

static uint32_t millis(void)
{
    return ms_ticks;
}

/* ══════════════════════════════════════════════════════════ */
/* SK6812 NeoPixel bit-bang driver                            */
/* ══════════════════════════════════════════════════════════ */
static inline void delay_ns_10(void) {
    __asm volatile ("nop\nnop\nnop\nnop\nnop\n"
                    "nop\nnop\nnop\nnop\nnop\n");
}
static inline void delay_ns_19(void) {
    __asm volatile ("nop\nnop\nnop\nnop\nnop\n"
                    "nop\nnop\nnop\nnop\nnop\n"
                    "nop\nnop\nnop\nnop\nnop\n"
                    "nop\nnop\nnop\nnop\n");
}
static inline void delay_ns_29(void) {
    __asm volatile ("nop\nnop\nnop\nnop\nnop\n"
                    "nop\nnop\nnop\nnop\nnop\n"
                    "nop\nnop\nnop\nnop\nnop\n"
                    "nop\nnop\nnop\nnop\nnop\n"
                    "nop\nnop\nnop\nnop\nnop\n"
                    "nop\nnop\nnop\nnop\n");
}

static inline void send_bit(uint8_t bit)
{
    if (bit) {
        DL_GPIO_setPins(LED_PORT, LED_PIN);   delay_ns_19();
        DL_GPIO_clearPins(LED_PORT, LED_PIN); delay_ns_19();
    } else {
        DL_GPIO_setPins(LED_PORT, LED_PIN);   delay_ns_10();
        DL_GPIO_clearPins(LED_PORT, LED_PIN); delay_ns_29();
    }
}

static inline void send_byte(uint8_t b)
{
    for (int i = 7; i >= 0; i--) send_bit((b >> i) & 1);
}

/* SK6812 byte order is G, R, B */
void sk6812_write_all(uint8_t r, uint8_t g, uint8_t b)
{
    __disable_irq();
    for (int i = 0; i < NUM_LEDS; i++) {
        send_byte(g);
        send_byte(r);
        send_byte(b);
    }
    __enable_irq();
    DL_GPIO_clearPins(LED_PORT, LED_PIN);
    delay_cycles(3200);  /* 100µs reset pulse */
}

/* ══════════════════════════════════════════════════════════ */
/* UART helpers                                               */
/* ══════════════════════════════════════════════════════════ */
static void uart_tx_byte(uint8_t b)
{
    while (DL_UART_Main_isBusy(UART_0_INST)) {}
    DL_UART_Main_transmitData(UART_0_INST, b);
}

static void uart_tx_string(const char *str)
{
    while (*str) uart_tx_byte((uint8_t)*str++);
}

static bool rx_contains(const char *cmd)
{
    uint32_t slen = strlen(cmd);
    if (rxIndex < slen) return false;
    for (uint32_t i = 0; i + slen <= rxIndex; i++)
    {
        bool match = true;
        for (uint32_t j = 0; j < slen; j++)
        {
            if (rxBuffer[i+j] != cmd[j]) { match = false; break; }
        }
        if (match) return true;
    }
    return false;
}

static void clear_rx(void)
{
    rxIndex   = 0;
    rxNewLine = false;
    memset((void*)rxBuffer, 0, sizeof(rxBuffer));
}

/* ══════════════════════════════════════════════════════════ */
/* UART0 ISR — stores bytes received from ESP32               */
/* ══════════════════════════════════════════════════════════ */
void UART_0_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_0_INST))
    {
        case DL_UART_MAIN_IIDX_RX:
        {
            char c = (char)DL_UART_Main_receiveData(UART_0_INST);
            if (rxIndex < RX_BUF_SIZE - 1)
            {
                rxBuffer[rxIndex++] = c;
            }
            if (c == '\n')
            {
                rxBuffer[rxIndex] = '\0';
                rxNewLine = true;
            }
            break;
        }
        default:
            break;
    }
}

/* ══════════════════════════════════════════════════════════ */
/* MAIN                                                        */
/* ══════════════════════════════════════════════════════════ */
int main(void)
{
    SYSCFG_DL_init();

    /* ── PA27 LED strip — manual init (confirmed working) ── */
    DL_GPIO_initDigitalOutput(IOMUX_PINCM60);
    DL_GPIO_enableOutput(GPIOA, DL_GPIO_PIN_27);
    DL_GPIO_clearPins(GPIOA, DL_GPIO_PIN_27);

    /* ── PA13 button — manual init with pull-down ── */
    DL_GPIO_initDigitalInputFeatures(
        IOMUX_PINCM35,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE
    );

    /* ── UART RX interrupt ── */
    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);

    /* ── Start millisecond tick ── */
    systick_init();

    /* ── LEDs off at startup ── */
    sk6812_write_all(0, 0, 0);

    /* ── Wait 3 seconds for ESP32 to boot ── */
    delay_cycles(96000000UL);

    /* ── Button debounce state ── */
    bool     prevState   = false;
    bool     stableState = false;
    uint32_t sameCount   = 0;

    /* ══════════════════════════════════════════════════════ */
    /* Main loop                                              */
    /* ══════════════════════════════════════════════════════ */
    while (1)
    {
        uint32_t now = millis();

        /* ================================================ */
        /* 1. BUTTON DETECTION (debounced, rising edge)     */
        /* ================================================ */
        bool currentRaw = (DL_GPIO_readPins(GPIOA, DL_GPIO_PIN_13) != 0);

        if (currentRaw == stableState)
            sameCount++;
        else
        {
            sameCount   = 0;
            stableState = currentRaw;
        }

        if (sameCount > DEBOUNCE_CYCLES)
        {
            if (stableState == true && prevState == false)
            {
                /* Rising edge — button just pressed */
                /* Tell ESP32 to publish to MQTT     */
                uart_tx_string("BTN\r\n");
            }
            prevState = stableState;
        }

        /* ================================================ */
        /* 2. HANDLE INCOMING UART FROM ESP32               */
        /* Received when partner pressed their button       */
        /* ================================================ */
        if (rxNewLine)
        {
            if (rx_contains("ACTIVE"))
            {
                ledsActive   = true;
                ledOnStartMs = now;
                sk6812_write_all(LED_R, LED_G, LED_B);
            }
            clear_rx();
        }

        /* ================================================ */
        /* 3. AUTO-OFF AFTER 15 SECONDS                     */
        /* ================================================ */
        if (ledsActive && (now - ledOnStartMs) >= LED_ON_MS)
        {
            ledsActive = false;
            sk6812_write_all(0, 0, 0);
        }
    }
}
