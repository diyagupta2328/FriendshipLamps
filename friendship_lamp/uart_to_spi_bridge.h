#ifndef UART_TO_SPI_BRIDGE_H
#define UART_TO_SPI_BRIDGE_H

void UARTBridge_Init(void);
void UARTBridge_ProcessCommand(const char *cmd);

#endif