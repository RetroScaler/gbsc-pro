/**
 *******************************************************************************
 * @file  usart/usart_uart_dma/source/main.c
 * @brief This example demonstrates UART data receive and transfer by DMA.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2022-03-31       CDT             First version
   2022-10-31       CDT             Delete the redundant code
                                    Read USART_DR.RDR when USART overrun error occur.
   2023-01-15       CDT             Update UART timeout function calculating formula for Timer0 CMP value
   2023-09-30       CDT             Split register USART_DR to USART_RDR and USART_TDR
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022-2023, Xiaohua Semiconductor Co., Ltd. All rights reserved.
 *
 * This software component is licensed by XHSC under BSD 3-Clause license
 * (the "License"); You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                    opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "main.h"

/**
 * @addtogroup HC32F460_DDL_Examples
 * @{
 */

/**
 * @addtogroup USART_UART_DMA
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* Peripheral register WE/WP selection */
#define LL_PERIPH_SEL (LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                       LL_PERIPH_EFM | LL_PERIPH_SRAM)

extern uint16_t g_u16_sys_timer;
extern uint16_t g_u16_key_timer;
extern uint16_t g_u16_mis_timer;
extern uint16_t g_u16_osd_timer;

__IO uint8_t m_u8SpeedUpd = 0U;

// const uint8_t usFlashInitVal[4] __attribute__((at(0x00007FFC))) = {0x23, 0x01, 0x89, 0x67}; // 定位在flash中
// const uint8_t usFlashInitVal[4] __attribute__((at(IAP_BOOT_SIZE - 4))) = {0x23, 0x01, 0x89, 0x67}; // 定位在flash中

const uint8_t bright_atr[3] = {0x00, 0x7f, 0x80};
const uint8_t contrast_atr[3] = {0x00, 0x80, 0xff};
static uint8_t BrightCount = 0x00;
static uint8_t ContrastCount = 0xb0;
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

static void vd_Button_Init(void)
{
  /* configuration structure initialization */ // 输出控制端口
  stc_gpio_init_t stcGpioInit;
  (void)GPIO_StructInit(&stcGpioInit);
  stcGpioInit.u16PullUp = PIN_PU_ON;         // 上拉
  stcGpioInit.u16PinDir = PIN_DIR_OUT;       // 输出方向
  stcGpioInit.u16PinAttr = PIN_ATTR_DIGITAL; // 数字
  //    stcGpioInit.u16PinOutputType = PIN_OUT_TYPE_CMOS;   //coms强输出
  //    stcGpioInit.u16ExtInt = PIN_EXTINT_OFF;   //不使用外部中断
  //    stcGpioInit.u16PinDrv = PIN_HIGH_DRV;   //高速

  stcGpioInit.u16PinState = PIN_STAT_RST; // 拉低

  (void)GPIO_Init(GPIO_PORT_B, GPIO_PIN_01, &stcGpioInit); // OUTPUT_EN
  (void)GPIO_Init(GPIO_PORT_B, GPIO_PIN_00, &stcGpioInit); // POWER_DOWN_N
  (void)GPIO_Init(GPIO_PORT_A, GPIO_PIN_03, &stcGpioInit); // INPUT_RESET_N

  //////////////////Line
  (void)GPIO_StructInit(&stcGpioInit);
  stcGpioInit.u16PullUp = PIN_PU_ON;         // 上拉
  stcGpioInit.u16PinDir = PIN_DIR_IN;        // 输入方向
  stcGpioInit.u16PinAttr = PIN_ATTR_DIGITAL; // 数字
  stcGpioInit.u16PinState = PIN_STAT_SET;    // 拉低

  (void)GPIO_Init(GPIO_PORT_B, GPIO_PIN_05, &stcGpioInit);

  ///////////////ASW/////////////////////
  (void)GPIO_StructInit(&stcGpioInit);
  stcGpioInit.u16PullUp = PIN_PU_OFF;        // 上拉
  stcGpioInit.u16PinDir = PIN_DIR_OUT;       // 输出方向
  stcGpioInit.u16PinAttr = PIN_ATTR_DIGITAL; // 数字
  stcGpioInit.u16PinState = PIN_STAT_RST;    // 拉低
  stcGpioInit.u16PinDrv = PIN_HIGH_DRV;
  stcGpioInit.u16PinOutputType = PIN_OUT_TYPE_CMOS;

  (void)GPIO_Init(GPIO_PORT_ASW, GPIO_PIN_ASW1, &stcGpioInit); // ASW01
  (void)GPIO_Init(GPIO_PORT_ASW, GPIO_PIN_ASW3, &stcGpioInit); // ASW03
  (void)GPIO_Init(GPIO_PORT_ASW, GPIO_PIN_ASW4, &stcGpioInit); // ASW04

  asw_02 = Read_ASW2();
  //    stcGpioInit.u16PullUp = PIN_PU_OFF;         // 上拉
  if (asw_02)
  {
    stcGpioInit.u16PinState = PIN_STAT_SET; // 拉高
  }
  else
  {
    stcGpioInit.u16PinState = PIN_STAT_RST; // 拉低
  }
  (void)GPIO_Init(GPIO_PORT_ASW, GPIO_PIN_ASW2, &stcGpioInit); // ASW02

  Video_ReadNot2(1);

  AVsw = Read_AVSW();
  if (AVsw)
  {
    stcGpioInit.u16PullUp = PIN_PU_OFF;     // 不上拉
    stcGpioInit.u16PinState = PIN_STAT_RST; // 拉低
                                            // led_state = LED_ALL;
  }
  else
  {
    stcGpioInit.u16PullUp = PIN_PU_ON;      // 上拉
    stcGpioInit.u16PinState = PIN_STAT_SET; // 拉高
                                            // led_state = LED_RED;
  }
  (void)GPIO_Init(GPIO_PORT_A, GPIO_PIN_08, &stcGpioInit);
}

static void Key_Init(void)
{
  stc_gpio_init_t stcGpioInit;

  /* configuration structure initialization */

  (void)GPIO_StructInit(&stcGpioInit);
  stcGpioInit.u16PullUp = PIN_PU_ON; // KeyMode
  stcGpioInit.u16PinDir = PIN_DIR_IN;
  stcGpioInit.u16PinAttr = PIN_ATTR_DIGITAL;
  stcGpioInit.u16ExtInt = PIN_EXTINT_OFF;

  (void)GPIO_Init(GPIO_PORT_B, GPIO_PIN_06, &stcGpioInit);
}

static uint8_t Key_Read(uint8_t mode)
{
  static uint8_t key_up = 1; // 按键松开标志
  if (mode == 1)
    key_up = 1; // 支持连按

  if (key_up && (GPIO_ReadInputPins(GPIO_PORT_B, GPIO_PIN_06) == 0))
  {
    DDL_DelayMS(10);
    key_up = 0;
    if (GPIO_ReadInputPins(GPIO_PORT_B, GPIO_PIN_06) == 0)
      return 1;
  }
  else if (GPIO_ReadInputPins(GPIO_PORT_B, GPIO_PIN_06) == 1)
    key_up = 1;
  return 0; // 无按键按下
}

// 启用自动检测某些制式
void enable_auto_detection(uint8_t enable)
{
  static uint8_t art[2];
  if (enable)
  {
    // 启用PAL、NTSC和SECAM的自动检测
    art[0] = AUTO_DETECT_REG;
    art[1] = AD_PAL_EN | AD_NTSC_EN | AD_SECAM_EN | 0x80;
    (void)I2C_Master_Transmit(DEVICE_ADDR, art, 2, TIMEOUT);
  }
  else
  {
    // 禁用自动检测
    art[0] = AUTO_DETECT_REG;
    art[1] = 0x00;
    (void)I2C_Master_Transmit(DEVICE_ADDR, art, 2, TIMEOUT);
  }
}

void printBinary(unsigned char num)
{

  for (int i = sizeof(num) * 8 - 1; i >= 0; i--)
  {

    putchar((num & (1U << i)) ? '1' : '0');
  }
  putchar('\n');
}
/**
 * @brief  Main function of UART DMA project
 * @param  None
 * @retval int32_t return value, if needed
 */

int32_t main(void)
{
  uint8_t buff_main[2];
  stc_usart_uart_init_t stcUartInit;
  stc_irq_signin_config_t stcIrqSigninConfig;

  /* MCU Peripheral registers write unprotected */
  LL_PERIPH_WE(LL_PERIPH_SEL);
  __enable_irq();
  EFM_FWMC_Cmd(ENABLE);
  BSP_CLK_Init();
  BSP_LED_Init();
  vd_Button_Init();
  Key_Init();
#if (LL_TMR0_ENABLE == DDL_ON)
  TMR02_A_Config();
#endif

#if (LL_PRINT_ENABLE == DDL_ON)
  DDL_PrintfInit(BSP_PRINTF_DEVICE, BSP_PRINTF_BAUDRATE, BSP_PRINTF_Preinit);
#endif
  i2c_init();
  TMR0_Start(CM_TMR0_2, TMR0_CHA);
  uart_dma_init();
  Video_Sw(adv_sw);
  Signal_led(Input_signal);
  for (;;)
  {

    if (Key_Read(0))
    {
      //            btn_flag = ! btn_flag;
      c_state = 1;
    }

    if (g_u16_sys_timer >= SYS_TIMEOUT_100MS) // 100ms
    {
      //            printf("123456789abcdef");
      detect_loop();
      g_u16_sys_timer = 0;
    }
    if (g_u16_key_timer >= SYS_TIMEOUT_50MS) // 50MS
    {
      signal_turn();
      g_u16_key_timer = 0;
    }
    if (g_u16_mis_timer >= SYS_TIMEOUT_100MS) // 100ms
    {
      static uint8_t Input_signal_last = 0;
      if (c_state == 1)
        C_LED_OK();
      else if (c_state == 2)
        C_LED_ERR_RED();
      else if (c_state == 3)
        C_LED_ERR_GREEN();
      else if (c_state == 4)
        C_LED_ERR_BLUE();
      c_state = 0;
      if (Input_signal != Input_signal_last)
      {
        Signal_led(Input_signal);
        Input_signal_last = Input_signal;
      }
      g_u16_mis_timer = 0;
    }
    if (g_u16_osd_timer >= SYS_TIMEOUT_500MS) // 500MS   OSD
    {
      static uint8_t count;
      err_flag = 1;

      //            count++;
      //            if (count >= 3)
      //            {
      //                uint8_t buff = 0x10 ,buff_Re;
      //                count = 0;
      //                Chip_Receive(DEVICE_ADDR, &buff, &buff_Re, 1, TIMEOUT);
      //                printf("buff_Re 0x%02x \n",buff_Re);
      //
      ////                printf("ASW %d %d %d %d \n",
      ////                       GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW1), GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW2), GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW3), GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW4));
      ////                printf(" AVsw %d\n" ,AVsw);
      //            }

      g_u16_osd_timer = 0;
    }
  }
}

/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
