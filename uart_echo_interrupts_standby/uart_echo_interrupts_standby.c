/*
 * RX test — just listen on PA11 and store everything received
 * If IO17 on ESP32 is reaching PA11, we will see the ESP32
 * boot log bytes appear in rxBuffer as soon as ESP32 resets
 */

#include "ti_msp_dl_config.h"
#include <string.h>
#include <stdint.h>

#define RX_BUF_SIZE 256

volatile char     rxBuffer[RX_BUF_SIZE];
volatile uint32_t rxIndex = 0;

int main(void)
{
    SYSCFG_DL_init();
    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);

    /* Just sit and listen— do NOT transmit anything */
    while (1)
    {
        delay_cycles(32000000);  /* 1 second loop */
    }
}

void UART_0_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_0_INST))
    {
        case DL_UART_MAIN_IIDX_RX:
        {
            char c = (char)DL_UART_Main_receiveData(UART_0_INST);
            if (rxIndex < RX_BUF_SIZE - 1)
            {
                rxBuffer[rxIndex] = c;
                rxIndex++;
            }
            break;
        }
        default:
            break;
    }
}