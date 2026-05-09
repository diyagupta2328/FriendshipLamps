#include "ti_msp_dl_config.h"
#include "uart_to_spi_bridge.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* UART receive buffer */
#define RX_BUFFER_SIZE 64

static volatile char rxBuffer[RX_BUFFER_SIZE];
static volatile uint32_t rxIndex = 0;
static volatile bool commandReady = false;

static char command[RX_BUFFER_SIZE];

static void UART_SendString(const char *str)
{
    while (*str) {
        DL_UART_Main_transmitDataBlocking(UART_BRIDGE_INST, (uint8_t)*str++);
    }
}

int main(void)
{
    SYSCFG_DL_init();

    UARTBridge_Init();

    NVIC_ClearPendingIRQ(UART_BRIDGE_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_BRIDGE_INST_INT_IRQN);

    UART_SendString("MSPM0 UART-SPI LED controller ready\n");

    while (1) {
        if (commandReady) {
            __disable_irq();

            strncpy(command, (const char *)rxBuffer, RX_BUFFER_SIZE);
            command[RX_BUFFER_SIZE - 1] = '\0';
            commandReady = false;

            __enable_irq();

            UARTBridge_ProcessCommand(command);

            UART_SendString("ACK:");
            UART_SendString(command);
            UART_SendString("\n");
        }

        __WFE();
    }
}

void UART_BRIDGE_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_BRIDGE_INST)) {

        case DL_UART_MAIN_IIDX_RX: {
            char c = (char) DL_UART_Main_receiveDataBlocking(UART_BRIDGE_INST);

            if (c == '\r') {
                break;
            }

            if (c == '\n') {
                rxBuffer[rxIndex] = '\0';
                rxIndex = 0;
                commandReady = true;
            } else {
                if (rxIndex < RX_BUFFER_SIZE - 1) {
                    rxBuffer[rxIndex++] = c;
                } else {
                    rxIndex = 0;
                    UART_SendString("ERR:BUFFER_OVERFLOW\n");
                }
            }

            break;
        }

        default:
            break;
    }
}