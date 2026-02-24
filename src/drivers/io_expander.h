#pragma once
#include <stdint.h>
#include <stdbool.h>

// I2C-Adresse
#define PCA9555_ADDR 0x20

// Register
#define REG_INPUT_P0   0x00
#define REG_INPUT_P1   0x01
#define REG_OUTPUT_P0  0x02
#define REG_OUTPUT_P1  0x03
#define REG_CONFIG_P0  0x06
#define REG_CONFIG_P1  0x07

typedef enum {
  PIN_GRID_ON  = (0<<8) | 4,
  PIN_ISLE_ON  = (0<<8) | 5,
  PIN_GI_SEL   = (0<<8) | 6,
  PIN_NTC_HOT  = (0<<8) | 7,
  PIN_ASEL1    = (1<<8) | 0,
  PIN_ASEL2    = (1<<8) | 1,
} PCA9555_Pin;

void PCA9555_Init(void);
void PCA9555_WritePin(PCA9555_Pin pin, bool level);
bool PCA9555_ReadPin(PCA9555_Pin pin);