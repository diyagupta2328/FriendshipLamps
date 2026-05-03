#include "sk6812_spi.h"
#include "ti_msp_dl_config.h"

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB_t;

static RGB_t leds[NUM_LEDS];
static uint8_t brightness = 255;

static void SPI_WriteByte(uint8_t data)
{
    while (DL_SPI_isTXFIFOFull(SPI_0_INST)) {
        ;
    }

    DL_SPI_transmitData8(SPI_0_INST, data);
}

static uint8_t scale(uint8_t value)
{
    return (uint8_t)(((uint16_t)value * brightness) / 255);
}

static void SK6812_SendEncodedByte(uint8_t data)
{
    uint32_t encoded = 0;

    for (int bit = 7; bit >= 0; bit--) {
        encoded <<= 3;

        if (data & (1 << bit)) {
            encoded |= 0b110;   // logic 1
        } else {
            encoded |= 0b100;   // logic 0
        }
    }

    SPI_WriteByte((encoded >> 16) & 0xFF);
    SPI_WriteByte((encoded >> 8) & 0xFF);
    SPI_WriteByte(encoded & 0xFF);
}

static void SK6812_ResetLatch(void)
{
    for (int i = 0; i < 100; i++) {
        SPI_WriteByte(0x00);
    }
}

void SK6812_Init(void)
{
    SK6812_Clear();
    SK6812_Show();
}

void SK6812_SetPixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= NUM_LEDS) return;

    leds[index].r = r;
    leds[index].g = g;
    leds[index].b = b;
}

void SK6812_SetAll(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint32_t i = 0; i < NUM_LEDS; i++) {
        SK6812_SetPixel(i, r, g, b);
    }
}

void SK6812_SetBrightness(uint8_t value)
{
    brightness = value;
}

void SK6812_Clear(void)
{
    SK6812_SetAll(0, 0, 0);
}

void SK6812_Show(void)
{
    __disable_irq();

    for (uint32_t i = 0; i < NUM_LEDS; i++) {
        /*
         * SK6812 RGB order is usually GRB.
         * If colors are wrong, change this order.
         */
        SK6812_SendEncodedByte(scale(leds[i].g));
        SK6812_SendEncodedByte(scale(leds[i].r));
        SK6812_SendEncodedByte(scale(leds[i].b));
    }

    SK6812_ResetLatch();

    __enable_irq();
}