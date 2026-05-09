/*
 * uart_to_spi_bridge.c
 * * ESP32 sends UART text commands.
 * MSPM0 parses the command and controls SK6812 LEDs through SPI.
 */

#include "uart_to_spi_bridge.h"
#include "sk6812_spi.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

static uint8_t currentPattern = 0;
static uint8_t currentBrightness = 120;

static void applyPattern(uint8_t pattern)
{
    currentPattern = pattern;

    switch (pattern) {
        case 0:
            SK6812_SetAll(255, 0, 0);      // red
            break;

        case 1:
            SK6812_SetAll(0, 255, 0);      // green
            break;

        case 2:
            SK6812_SetAll(0, 0, 255);      // blue
            break;

        case 3:
            for (int i = 0; i < NUM_LEDS; i++) {
                if (i % 3 == 0) {
                    SK6812_SetPixel(i, 255, 0, 0);
                } else if (i % 3 == 1) {
                    SK6812_SetPixel(i, 0, 255, 0);
                } else {
                    SK6812_SetPixel(i, 0, 0, 255);
                }
            }
            break;

        default:
            SK6812_Clear();
            break;
    }

    SK6812_Show();
}

void UARTBridge_Init(void)
{
    currentPattern = 0;
    currentBrightness = 120;

    SK6812_Init();
    SK6812_SetBrightness(currentBrightness);
    SK6812_Clear();
    SK6812_Show();
}

void UARTBridge_ProcessCommand(const char *cmd)
{
    if (strcmp(cmd, "RED") == 0) {
        SK6812_SetAll(255, 0, 0);
        SK6812_Show();
    }

    else if (strcmp(cmd, "GREEN") == 0) {
        SK6812_SetAll(0, 255, 0);
        SK6812_Show();
    }

    else if (strcmp(cmd, "BLUE") == 0) {
        SK6812_SetAll(0, 0, 255);
        SK6812_Show();
    }

    else if (strcmp(cmd, "WHITE") == 0) {
        SK6812_SetAll(255, 255, 255);
        SK6812_Show();
    }

    else if (strcmp(cmd, "OFF") == 0) {
        SK6812_Clear();
        SK6812_Show();
    }

    else if (strncmp(cmd, "BRIGHTNESS:", 11) == 0) {
        int value = atoi(cmd + 11);

        if (value < 0) {
            value = 0;
        }

        if (value > 255) {
            value = 255;
        }

        currentBrightness = (uint8_t)value;
        SK6812_SetBrightness(currentBrightness);
        applyPattern(currentPattern);
    }

    else if (strncmp(cmd, "LED_PATTERN:", 12) == 0) {
        int pattern = atoi(cmd + 12);

        if (pattern < 0) {
            pattern = 0;
        }

        if (pattern > 3) {
            pattern = 3;
        }

        applyPattern((uint8_t)pattern);
    }

    else if (strcmp(cmd, "ANIMATION_ON") == 0) {
        applyPattern(currentPattern);
    }

    else if (strcmp(cmd, "ANIMATION_OFF") == 0) {
        SK6812_Clear();
        SK6812_Show();
    }
}