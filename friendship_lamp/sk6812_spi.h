#ifndef SK6812_SPI_H
#define SK6812_SPI_H

#include <stdint.h>

#define NUM_LEDS 8

void SK6812_Init(void);
void SK6812_SetPixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b);
void SK6812_SetAll(uint8_t r, uint8_t g, uint8_t b);
void SK6812_SetBrightness(uint8_t brightness);
void SK6812_Show(void);
void SK6812_Clear(void);

#endif