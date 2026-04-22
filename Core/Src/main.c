/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_rx;
DMA_HandleTypeDef hdma_i2c1_tx;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

NOR_HandleTypeDef hnor1;

/* Stubbed - HAL driver not available */
/* CRC_HandleTypeDef hcrc; */
/* RTC_HandleTypeDef hrtc; */

/* USER CODE BEGIN PV */

/* ADC DMA Buffer for continuous conversion */
volatile uint16_t g_adc_dma_buffer[ADC_DMA_BUFFER_SIZE] = {0};
volatile uint8_t g_adc_conversion_complete = 0;

/* System tick counter from TIM3 */
volatile uint32_t g_system_tick = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_FSMC_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_CRC_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */
static void BOARD_LED_DiagnosticInit(void);
static void BOARD_LED_DiagnosticLoop(void);
static void BOARD_LCD_TestLoop(void);
static void BOARD_LCD_ProbeLoop(void);
static void BOARD_LCD_SweepLoop(void);
static void BOARD_LCD_ForceBacklightDefaultState(void);
static void BOARD_IrrigationDemoInit(void);
static void BOARD_IrrigationDemoProcess(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile uint16_t g_lcd_probe_words[96] = {0};
volatile uint32_t g_lcd_probe_signature = 0U;
volatile uint32_t g_lcd_sweep_profile = 0U;

typedef struct {
  uint8_t active;
  uint8_t valve_id;
  uint8_t phase;
  uint32_t phase_start_tick;
} board_irrigation_demo_t;

static board_irrigation_demo_t g_board_irrigation_demo = {0};

typedef struct {
  const char *name;
  uint32_t data_offset;
  GPIO_PinState pb10_state;
  lcd_color_t color;
  void (*init_fn)(uint32_t data_offset);
} board_lcd_sweep_profile_t;

static inline void BOARD_LCD_WriteCommandAt(uint32_t data_offset, uint16_t cmd)
{
  (void)data_offset;
  *(volatile uint16_t *)LCD_BASE_ADDR = cmd;
  __asm volatile("nop");
  __asm volatile("nop");
}

static inline void BOARD_LCD_WriteDataAt(uint32_t data_offset, uint16_t data)
{
  *(volatile uint16_t *)(LCD_BASE_ADDR + data_offset) = data;
  __asm volatile("nop");
  __asm volatile("nop");
}

static void BOARD_LCD_WriteRegAt(uint32_t data_offset, uint16_t reg,
                                 uint16_t data)
{
  BOARD_LCD_WriteCommandAt(data_offset, reg);
  BOARD_LCD_WriteDataAt(data_offset, data);
}

static void BOARD_LCD_ILI9341_InitAt(uint32_t data_offset)
{
  BOARD_LCD_WriteCommandAt(data_offset, 0x01);
  HAL_Delay(10);

  BOARD_LCD_WriteCommandAt(data_offset, 0xCB);
  BOARD_LCD_WriteDataAt(data_offset, 0x39);
  BOARD_LCD_WriteDataAt(data_offset, 0x2C);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);
  BOARD_LCD_WriteDataAt(data_offset, 0x34);
  BOARD_LCD_WriteDataAt(data_offset, 0x02);

  BOARD_LCD_WriteCommandAt(data_offset, 0xCF);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);
  BOARD_LCD_WriteDataAt(data_offset, 0xC1);
  BOARD_LCD_WriteDataAt(data_offset, 0x30);

  BOARD_LCD_WriteCommandAt(data_offset, 0xE8);
  BOARD_LCD_WriteDataAt(data_offset, 0x85);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);
  BOARD_LCD_WriteDataAt(data_offset, 0x78);

  BOARD_LCD_WriteCommandAt(data_offset, 0xEA);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);

  BOARD_LCD_WriteCommandAt(data_offset, 0xED);
  BOARD_LCD_WriteDataAt(data_offset, 0x64);
  BOARD_LCD_WriteDataAt(data_offset, 0x03);
  BOARD_LCD_WriteDataAt(data_offset, 0x12);
  BOARD_LCD_WriteDataAt(data_offset, 0x81);

  BOARD_LCD_WriteCommandAt(data_offset, 0xF7);
  BOARD_LCD_WriteDataAt(data_offset, 0x20);

  BOARD_LCD_WriteCommandAt(data_offset, 0xC0);
  BOARD_LCD_WriteDataAt(data_offset, 0x23);
  BOARD_LCD_WriteCommandAt(data_offset, 0xC1);
  BOARD_LCD_WriteDataAt(data_offset, 0x10);

  BOARD_LCD_WriteCommandAt(data_offset, 0xC5);
  BOARD_LCD_WriteDataAt(data_offset, 0x3E);
  BOARD_LCD_WriteDataAt(data_offset, 0x28);
  BOARD_LCD_WriteCommandAt(data_offset, 0xC7);
  BOARD_LCD_WriteDataAt(data_offset, 0x86);

  BOARD_LCD_WriteCommandAt(data_offset, 0x36);
  BOARD_LCD_WriteDataAt(data_offset, 0x48);
  BOARD_LCD_WriteCommandAt(data_offset, 0x3A);
  BOARD_LCD_WriteDataAt(data_offset, 0x55);

  BOARD_LCD_WriteCommandAt(data_offset, 0xB1);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);
  BOARD_LCD_WriteDataAt(data_offset, 0x18);

  BOARD_LCD_WriteCommandAt(data_offset, 0xB6);
  BOARD_LCD_WriteDataAt(data_offset, 0x08);
  BOARD_LCD_WriteDataAt(data_offset, 0x82);
  BOARD_LCD_WriteDataAt(data_offset, 0x27);

  BOARD_LCD_WriteCommandAt(data_offset, 0xF2);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);
  BOARD_LCD_WriteCommandAt(data_offset, 0x26);
  BOARD_LCD_WriteDataAt(data_offset, 0x01);

  BOARD_LCD_WriteCommandAt(data_offset, 0xE0);
  BOARD_LCD_WriteDataAt(data_offset, 0x0F);
  BOARD_LCD_WriteDataAt(data_offset, 0x31);
  BOARD_LCD_WriteDataAt(data_offset, 0x2B);
  BOARD_LCD_WriteDataAt(data_offset, 0x0C);
  BOARD_LCD_WriteDataAt(data_offset, 0x0E);
  BOARD_LCD_WriteDataAt(data_offset, 0x08);
  BOARD_LCD_WriteDataAt(data_offset, 0x4E);
  BOARD_LCD_WriteDataAt(data_offset, 0xF1);
  BOARD_LCD_WriteDataAt(data_offset, 0x37);
  BOARD_LCD_WriteDataAt(data_offset, 0x07);
  BOARD_LCD_WriteDataAt(data_offset, 0x10);
  BOARD_LCD_WriteDataAt(data_offset, 0x03);
  BOARD_LCD_WriteDataAt(data_offset, 0x0E);
  BOARD_LCD_WriteDataAt(data_offset, 0x09);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);

  BOARD_LCD_WriteCommandAt(data_offset, 0xE1);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);
  BOARD_LCD_WriteDataAt(data_offset, 0x0E);
  BOARD_LCD_WriteDataAt(data_offset, 0x14);
  BOARD_LCD_WriteDataAt(data_offset, 0x03);
  BOARD_LCD_WriteDataAt(data_offset, 0x11);
  BOARD_LCD_WriteDataAt(data_offset, 0x07);
  BOARD_LCD_WriteDataAt(data_offset, 0x31);
  BOARD_LCD_WriteDataAt(data_offset, 0xC1);
  BOARD_LCD_WriteDataAt(data_offset, 0x48);
  BOARD_LCD_WriteDataAt(data_offset, 0x08);
  BOARD_LCD_WriteDataAt(data_offset, 0x0F);
  BOARD_LCD_WriteDataAt(data_offset, 0x0C);
  BOARD_LCD_WriteDataAt(data_offset, 0x31);
  BOARD_LCD_WriteDataAt(data_offset, 0x36);
  BOARD_LCD_WriteDataAt(data_offset, 0x0F);

  BOARD_LCD_WriteCommandAt(data_offset, 0x11);
  HAL_Delay(120);
  BOARD_LCD_WriteCommandAt(data_offset, 0x29);

  BOARD_LCD_WriteCommandAt(data_offset, 0x2A);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);
  BOARD_LCD_WriteDataAt(data_offset, 0xEF);
  BOARD_LCD_WriteCommandAt(data_offset, 0x2B);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);
  BOARD_LCD_WriteDataAt(data_offset, 0x00);
  BOARD_LCD_WriteDataAt(data_offset, 0x01);
  BOARD_LCD_WriteDataAt(data_offset, 0x3F);
}

static void BOARD_LCD_ILI9341_FillAt(uint32_t data_offset, lcd_color_t color)
{
  uint32_t i = 0U;

  BOARD_LCD_WriteCommandAt(data_offset, 0x2C);
  for (i = 0U; i < (240UL * 320UL); i++) {
    BOARD_LCD_WriteDataAt(data_offset, color);
  }
}

static void BOARD_LCD_ILI9325_InitAt(uint32_t data_offset)
{
  BOARD_LCD_WriteRegAt(data_offset, 0x00E5, 0x8000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0000, 0x0001);
  BOARD_LCD_WriteRegAt(data_offset, 0x0001, 0x0100);
  BOARD_LCD_WriteRegAt(data_offset, 0x0002, 0x0700);
  BOARD_LCD_WriteRegAt(data_offset, 0x0003, 0x1030);
  BOARD_LCD_WriteRegAt(data_offset, 0x0004, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0008, 0x0202);
  BOARD_LCD_WriteRegAt(data_offset, 0x0009, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x000A, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x000C, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x000D, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x000F, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0010, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0011, 0x0007);
  BOARD_LCD_WriteRegAt(data_offset, 0x0012, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0013, 0x0000);
  HAL_Delay(50);

  BOARD_LCD_WriteRegAt(data_offset, 0x0010, 0x1690);
  BOARD_LCD_WriteRegAt(data_offset, 0x0011, 0x0227);
  HAL_Delay(50);
  BOARD_LCD_WriteRegAt(data_offset, 0x0012, 0x001D);
  HAL_Delay(50);
  BOARD_LCD_WriteRegAt(data_offset, 0x0013, 0x1500);
  BOARD_LCD_WriteRegAt(data_offset, 0x0029, 0x0018);
  BOARD_LCD_WriteRegAt(data_offset, 0x002B, 0x000D);
  HAL_Delay(50);

  BOARD_LCD_WriteRegAt(data_offset, 0x0020, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0021, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0050, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0051, 0x00EF);
  BOARD_LCD_WriteRegAt(data_offset, 0x0052, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0053, 0x013F);
  BOARD_LCD_WriteRegAt(data_offset, 0x0060, 0xA700);
  BOARD_LCD_WriteRegAt(data_offset, 0x0061, 0x0001);
  BOARD_LCD_WriteRegAt(data_offset, 0x006A, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0080, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0081, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0082, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0083, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0084, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0085, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0090, 0x0010);
  BOARD_LCD_WriteRegAt(data_offset, 0x0092, 0x0600);
  BOARD_LCD_WriteRegAt(data_offset, 0x0093, 0x0003);
  BOARD_LCD_WriteRegAt(data_offset, 0x0095, 0x0110);
  BOARD_LCD_WriteRegAt(data_offset, 0x0097, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0098, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0007, 0x0173);
}

static void BOARD_LCD_ILI9325_FillAt(uint32_t data_offset, lcd_color_t color)
{
  uint32_t i = 0U;

  BOARD_LCD_WriteRegAt(data_offset, 0x0020, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0021, 0x0000);
  BOARD_LCD_WriteCommandAt(data_offset, 0x0022);
  for (i = 0U; i < (240UL * 320UL); i++) {
    BOARD_LCD_WriteDataAt(data_offset, color);
  }
}

static void BOARD_LCD_SSD1289_InitAt(uint32_t data_offset)
{
  BOARD_LCD_WriteRegAt(data_offset, 0x0000, 0x0001);
  BOARD_LCD_WriteRegAt(data_offset, 0x0003, 0xA8A4);
  BOARD_LCD_WriteRegAt(data_offset, 0x000C, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x000D, 0x080C);
  BOARD_LCD_WriteRegAt(data_offset, 0x000E, 0x2B00);
  BOARD_LCD_WriteRegAt(data_offset, 0x001E, 0x00B7);
  BOARD_LCD_WriteRegAt(data_offset, 0x0001, 0x2B3F);
  BOARD_LCD_WriteRegAt(data_offset, 0x0002, 0x0600);
  BOARD_LCD_WriteRegAt(data_offset, 0x0010, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0011, 0x6070);
  BOARD_LCD_WriteRegAt(data_offset, 0x0005, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0006, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0016, 0xEF1C);
  BOARD_LCD_WriteRegAt(data_offset, 0x0017, 0x0003);
  BOARD_LCD_WriteRegAt(data_offset, 0x0007, 0x0233);
  BOARD_LCD_WriteRegAt(data_offset, 0x000B, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x000F, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0041, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0042, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0048, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0049, 0x013F);
  BOARD_LCD_WriteRegAt(data_offset, 0x004A, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x004B, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0044, 0xEF00);
  BOARD_LCD_WriteRegAt(data_offset, 0x0045, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x0046, 0x013F);
}

static void BOARD_LCD_SSD1289_FillAt(uint32_t data_offset, lcd_color_t color)
{
  uint32_t i = 0U;

  BOARD_LCD_WriteRegAt(data_offset, 0x004E, 0x0000);
  BOARD_LCD_WriteRegAt(data_offset, 0x004F, 0x0000);
  BOARD_LCD_WriteCommandAt(data_offset, 0x0022);
  for (i = 0U; i < (240UL * 320UL); i++) {
    BOARD_LCD_WriteDataAt(data_offset, color);
  }
}

static void BOARD_LCD_ProbeRead(uint8_t reg, uint32_t base_index,
                                uint32_t samples)
{
  uint32_t i = 0U;

  if ((base_index + samples + 1U) > (sizeof(g_lcd_probe_words) /
                                     sizeof(g_lcd_probe_words[0]))) {
    return;
  }

  g_lcd_probe_words[base_index] = reg;
  LCD_WriteCommand(reg);
  (void)LCD_ReadData16(); /* dummy read */

  for (i = 0U; i < samples; i++) {
    g_lcd_probe_words[base_index + 1U + i] = LCD_ReadData16();
  }
}

static void BOARD_LED_DiagnosticInit(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*
   * D2 and D3 are wired to PA6/PA7 as active-low LEDs on this board.
   * Drive them high first so the board powers up with both LEDs off.
   */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void BOARD_LED_DiagnosticLoop(void)
{
  while (1)
  {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(200);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_Delay(200);
  }
}

static void BOARD_LCD_TestLoop(void)
{
  static const lcd_color_t test_colors[] = {
      ILI9341_RED,
      ILI9341_GREEN,
      ILI9341_BLUE,
      ILI9341_WHITE,
      ILI9341_BLACK,
  };
  uint32_t i = 0U;

  while (1)
  {
    LCD_Clear(test_colors[i]);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(800);

    i++;
    if (i >= (sizeof(test_colors) / sizeof(test_colors[0]))) {
      i = 0U;
    }
  }
}

static void BOARD_LCD_ProbeLoop(void)
{
  LCD_Reset();
  HAL_Delay(20);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
  BOARD_LCD_ProbeRead(ILI9341_RDDID, 0U, 4U);
  BOARD_LCD_ProbeRead(ILI9341_RDDST, 6U, 4U);
  BOARD_LCD_ProbeRead(ILI9341_RDMODE, 12U, 2U);
  BOARD_LCD_ProbeRead(ILI9341_RDPIXFMT, 16U, 2U);
  BOARD_LCD_ProbeRead(ILI9341_RDID1, 20U, 2U);
  BOARD_LCD_ProbeRead(ILI9341_RDID2, 24U, 2U);
  BOARD_LCD_ProbeRead(ILI9341_RDID3, 28U, 2U);
  BOARD_LCD_ProbeRead(ILI9341_RDID4, 32U, 2U);
  /* Older 320x240 parallel controllers often expose the chip ID on reg 0x00. */
  BOARD_LCD_ProbeRead(0x00, 36U, 2U);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
  HAL_Delay(5);
  BOARD_LCD_ProbeRead(ILI9341_RDDID, 40U, 4U);
  BOARD_LCD_ProbeRead(ILI9341_RDDST, 46U, 4U);
  BOARD_LCD_ProbeRead(ILI9341_RDMODE, 52U, 2U);
  BOARD_LCD_ProbeRead(ILI9341_RDPIXFMT, 56U, 2U);
  BOARD_LCD_ProbeRead(ILI9341_RDID1, 60U, 2U);
  BOARD_LCD_ProbeRead(ILI9341_RDID2, 64U, 2U);
  BOARD_LCD_ProbeRead(ILI9341_RDID3, 68U, 2U);
  BOARD_LCD_ProbeRead(ILI9341_RDID4, 72U, 2U);
  BOARD_LCD_ProbeRead(0x00, 76U, 2U);

  g_lcd_probe_signature = 0xC0D3D00DU;

  while (1)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(250);
  }
}

static void BOARD_LCD_SweepLoop(void)
{
  static const board_lcd_sweep_profile_t profiles[] = {
      {"ILI9341-A18-CSH", 0x00080000U, GPIO_PIN_SET, ILI9341_RED,
       BOARD_LCD_ILI9341_InitAt},
      {"ILI9341-A18-CSL", 0x00080000U, GPIO_PIN_RESET, ILI9341_GREEN,
       BOARD_LCD_ILI9341_InitAt},
      {"ILI9341-A6-CSH", 0x00000080U, GPIO_PIN_SET, ILI9341_DARKRED,
       BOARD_LCD_ILI9341_InitAt},
      {"ILI9341-A6-CSL", 0x00000080U, GPIO_PIN_RESET, ILI9341_DARKGRAY,
       BOARD_LCD_ILI9341_InitAt},
      {"ILI9341-A16-CSH", 0x00020000U, GPIO_PIN_SET, ILI9341_BLUE,
       BOARD_LCD_ILI9341_InitAt},
      {"ILI9341-A16-CSL", 0x00020000U, GPIO_PIN_RESET, ILI9341_YELLOW,
       BOARD_LCD_ILI9341_InitAt},
      {"ILI9325-A18-CSH", 0x00080000U, GPIO_PIN_SET, ILI9341_CYAN,
       BOARD_LCD_ILI9325_InitAt},
      {"ILI9325-A18-CSL", 0x00080000U, GPIO_PIN_RESET, ILI9341_MAGENTA,
       BOARD_LCD_ILI9325_InitAt},
      {"ILI9325-A6-CSH", 0x00000080U, GPIO_PIN_SET, ILI9341_GRAY,
       BOARD_LCD_ILI9325_InitAt},
      {"ILI9325-A6-CSL", 0x00000080U, GPIO_PIN_RESET, ILI9341_DARKGREEN,
       BOARD_LCD_ILI9325_InitAt},
      {"ILI9325-A16-CSH", 0x00020000U, GPIO_PIN_SET, ILI9341_WHITE,
       BOARD_LCD_ILI9325_InitAt},
      {"ILI9325-A16-CSL", 0x00020000U, GPIO_PIN_RESET, ILI9341_ORANGE,
       BOARD_LCD_ILI9325_InitAt},
      {"SSD1289-A18-CSH", 0x00080000U, GPIO_PIN_SET, ILI9341_NAVY,
       BOARD_LCD_SSD1289_InitAt},
      {"SSD1289-A18-CSL", 0x00080000U, GPIO_PIN_RESET, ILI9341_DARKGREEN,
       BOARD_LCD_SSD1289_InitAt},
      {"SSD1289-A6-CSH", 0x00000080U, GPIO_PIN_SET, ILI9341_PINK,
       BOARD_LCD_SSD1289_InitAt},
      {"SSD1289-A6-CSL", 0x00000080U, GPIO_PIN_RESET, ILI9341_BROWN,
       BOARD_LCD_SSD1289_InitAt},
      {"SSD1289-A16-CSH", 0x00020000U, GPIO_PIN_SET, ILI9341_PINK,
       BOARD_LCD_SSD1289_InitAt},
      {"SSD1289-A16-CSL", 0x00020000U, GPIO_PIN_RESET, ILI9341_BROWN,
       BOARD_LCD_SSD1289_InitAt},
  };
  uint32_t i = 0U;

  while (1)
  {
    for (i = 0U; i < (sizeof(profiles) / sizeof(profiles[0])); i++) {
      g_lcd_sweep_profile = i;
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, profiles[i].pb10_state);
      LCD_Reset();
      HAL_Delay(20);
      profiles[i].init_fn(profiles[i].data_offset);

      if (profiles[i].init_fn == BOARD_LCD_ILI9341_InitAt) {
        BOARD_LCD_ILI9341_FillAt(profiles[i].data_offset, profiles[i].color);
      } else if (profiles[i].init_fn == BOARD_LCD_ILI9325_InitAt) {
        BOARD_LCD_ILI9325_FillAt(profiles[i].data_offset, profiles[i].color);
      } else {
        BOARD_LCD_SSD1289_FillAt(profiles[i].data_offset, profiles[i].color);
      }

      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
      HAL_Delay(2500);
    }
  }
}

static void BOARD_LCD_ForceBacklightDefaultState(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /*
   * Hardware inspection shows the TFT backlight transistor base is routed
 * through R30 to PB1. For sweep diagnostics we drive PB1 high directly so the
 * panel stays visibly lit even before the full UI stack starts.
   */
  HAL_GPIO_DeInit(GPIOB, GPIO_PIN_1);

  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
}

static void BOARD_IrrigationDemoInit(void)
{
  VALVES_CloseAll();
  GUI_NavigateTo(SCREEN_MANUAL);

  g_board_irrigation_demo.active = 1U;
  g_board_irrigation_demo.valve_id = 1U;
  g_board_irrigation_demo.phase = 0U;
  g_board_irrigation_demo.phase_start_tick = HAL_GetTick();
}

static void BOARD_IrrigationDemoProcess(void)
{
  static const uint8_t demo_valve_count = 4U;
  static const uint32_t demo_open_ms = 3000U;
  static const uint32_t demo_gap_ms = 800U;
  uint32_t now = HAL_GetTick();

  if (g_board_irrigation_demo.active == 0U) {
    return;
  }

  if (g_board_irrigation_demo.phase == 0U) {
    VALVES_Open(g_board_irrigation_demo.valve_id);
    g_board_irrigation_demo.phase = 1U;
    g_board_irrigation_demo.phase_start_tick = now;
    return;
  }

  if (g_board_irrigation_demo.phase == 1U) {
    if ((now - g_board_irrigation_demo.phase_start_tick) < demo_open_ms) {
      return;
    }

    VALVES_Close(g_board_irrigation_demo.valve_id);
    g_board_irrigation_demo.phase = 2U;
    g_board_irrigation_demo.phase_start_tick = now;
    return;
  }

  if ((now - g_board_irrigation_demo.phase_start_tick) < demo_gap_ms) {
    return;
  }

  if (g_board_irrigation_demo.valve_id >= demo_valve_count) {
    g_board_irrigation_demo.active = 0U;
    g_board_irrigation_demo.phase = 0U;
    VALVES_CloseAll();
    GUI_NavigateTo(SCREEN_MAIN);
    return;
  }

  g_board_irrigation_demo.valve_id++;
  g_board_irrigation_demo.phase = 0U;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  uint32_t last_status_update = 0;
  uint32_t last_task_tick = 0;
  uint8_t validation_counter = 0U;

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
#if !BOARD_LED_DIAGNOSTIC_MODE && !BOARD_LCD_TEST_MODE && !BOARD_LCD_PROBE_MODE && !BOARD_LCD_SWEEP_MODE
  MX_ADC1_Init();
  MX_FSMC_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_CRC_Init();
  MX_RTC_Init();
#elif BOARD_LCD_TEST_MODE || BOARD_LCD_PROBE_MODE || BOARD_LCD_SWEEP_MODE
  MX_FSMC_Init();
  MX_TIM1_Init();
#endif
  /* USER CODE BEGIN 2 */
#if BOARD_LED_DIAGNOSTIC_MODE
  BOARD_LED_DiagnosticInit();
  BOARD_LED_DiagnosticLoop();
#elif BOARD_LCD_TEST_MODE
  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  LCD_SetBacklight(100);
  LCD_Init();
  BOARD_LCD_TestLoop();
#elif BOARD_LCD_PROBE_MODE
  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  LCD_SetBacklight(100);
  BOARD_LCD_ProbeLoop();
#elif BOARD_LCD_SWEEP_MODE
  BOARD_LCD_ForceBacklightDefaultState();
  BOARD_LCD_SweepLoop();
#else
  /* Start ADC DMA Continuous Conversion */
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&g_adc_dma_buffer, ADC_DMA_BUFFER_SIZE) != HAL_OK)
  {
    Error_Handler();
  }
  
  /* Start TIM3 for non-blocking system timing (1ms base) */
  if (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  
  if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  System_Init();
  MX_USB_DEVICE_Init();
#if BOARD_IRRIGATION_DEMO_MODE
  BOARD_IrrigationDemoInit();
#endif
#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  
  /* Test: Blink LED to verify code is running */
  uint32_t led_tick = 0;
  
  while (1)
  {
    /* USER CODE END WHILE */

    /* Test LED blink - PC13 */
    if ((HAL_GetTick() - led_tick) >= 500U) {
      led_tick = HAL_GetTick();
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }

    if ((HAL_GetTick() - last_task_tick) >= SYSTEM_TASK_PERIOD_MS) {
      last_task_tick = HAL_GetTick();
      System_CommunicationTask();
      System_HMITask();
      System_ProgramManagementTask();
      System_IrrigationTask();
      System_MaintenanceTask();

      validation_counter++;
      if (validation_counter >= SYSTEM_VALIDATION_INTERVAL) {
        validation_counter = 0U;
        System_SafetyMonitoringTask();
      }
    }

#if BOARD_IRRIGATION_DEMO_MODE
    BOARD_IrrigationDemoProcess();
#endif

    if ((HAL_GetTick() - last_status_update) >= SYSTEM_STATUS_PERIOD_MS) {
      last_status_update = HAL_GetTick();
      System_Status_Update();
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;  /* Scan mode for multiple channels */
  hadc1.Init.ContinuousConvMode = ENABLE;  /* Continuous conversion */
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;  /* pH and EC channels */
  hadc1.Init.DMAContinuousRequests = ENABLE;  /* DMA circular mode */
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;  /* PA0 - pH sensor */
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;  /* High accuracy */
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_1;  /* PA1 - EC sensor */
  sConfig.Rank = 2;
  sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;  /* Fast mode 400kHz for faster EEPROM access */
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
 * @brief SPI2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
#if SENSORS_INTERFACE_MODBUS
  huart1.Init.BaudRate = SENSORS_MODBUS_BAUDRATE;
#else
  huart1.Init.BaudRate = 115200;
#endif
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC,
                    GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
                    GPIO_PIN_4|GPIO_PIN_13|GPIO_PIN_14,
                    GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Configure valve outputs and board LEDs on GPIOC. */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
                        GPIO_PIN_4|GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* Configure touch PENIRQ on PC5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, TOUCH_IRQ_PRIORITY, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* FSMC initialization function */
static void MX_FSMC_Init(void)
{

  /* USER CODE BEGIN FSMC_Init 0 */

  /* USER CODE END FSMC_Init 0 */

  FSMC_NORSRAM_TimingTypeDef Timing = {0};

  /* USER CODE BEGIN FSMC_Init 1 */

  /* USER CODE END FSMC_Init 1 */

  /** Configure FSMC bank for a 16-bit 8080-style TFT interface.
   * The LCD is memory-mapped like SRAM/NORSRAM, not like a NOR flash chip.
   */
  hnor1.Instance = FSMC_NORSRAM_DEVICE;
  hnor1.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
  /* hnor1.Init */
  hnor1.Init.NSBank = FSMC_NORSRAM_BANK1;
  hnor1.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
  hnor1.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
  hnor1.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
  hnor1.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
  hnor1.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
  hnor1.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
  hnor1.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
  hnor1.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
  hnor1.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
  hnor1.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
  hnor1.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
  hnor1.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
  hnor1.Init.PageSize = FSMC_PAGE_SIZE_NONE;
  /* Timing */
  Timing.AddressSetupTime = 15;
  Timing.AddressHoldTime = 15;
  Timing.DataSetupTime = 255;
  Timing.BusTurnAroundDuration = 15;
  Timing.CLKDivision = 16;
  Timing.DataLatency = 17;
  Timing.AccessMode = FSMC_ACCESS_MODE_A;

  /*
   * MX_FSMC_Init uses the low-level FMC_NORSRAM helpers directly, so the usual
   * HAL_NOR_Init path that would invoke HAL_NOR_MspInit is bypassed. Call the
   * MSP hook here to ensure the TFT pins are actually switched to AF12/FSMC.
   */
  HAL_NOR_MspInit(&hnor1);

  (void)FMC_NORSRAM_Init(hnor1.Instance, &(hnor1.Init));
  (void)FMC_NORSRAM_Timing_Init(hnor1.Instance, &Timing, hnor1.Init.NSBank);
  __FMC_NORSRAM_ENABLE(hnor1.Instance, hnor1.Init.NSBank);

  /* USER CODE BEGIN FSMC_Init 2 */

  /* USER CODE END FSMC_Init 2 */
}

/* USER CODE BEGIN 4 */

/* DMA and Interrupt Callbacks =================================================*/

/**
 * @brief TIM3 Update Interrupt Callback (System Tick - 1ms)
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3)
  {
    g_system_tick++;
    /* System tick for non-blocking timing */
  }
}

/* USER CODE END 4 */

/**
 * @brief TIM2 Initialization (Buzzer PWM)
 */
static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;  /* 168MHz / 84 = 2MHz (1MHz PWM) */
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;    /* 1kHz PWM frequency */
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim2);
}

/**
 * @brief TIM3 Initialization (System Time Base - 1ms)
 */
static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 8399;  /* 84MHz / 8400 = 10kHz */
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 9;        /* 10kHz / 10 = 1kHz (1ms) */
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief CRC Initialization (Hardware CRC for data validation)
 * @note Stubbed - HAL CRC driver not available
 */
static void MX_CRC_Init(void)
{
  /* CRC initialization stubbed - driver not available in this build */
  /* Full implementation requires stm32f4xx_hal_crc.c */
}

/**
 * @brief RTC Initialization (Real-time clock for scheduling)
 * @note Stubbed - HAL RTC driver not available
 */
static void MX_RTC_Init(void)
{
  /* RTC initialization stubbed - driver not available in this build */
  /* Full implementation requires stm32f4xx_hal_rtc.c */
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* Blink RED LED (PC14) to indicate error */
  __disable_irq();
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_14);  /* RED LED blink */
    HAL_Delay(200);  /* Fast blink = error */
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
