/**
 *******************************************************************************
 * @file  usart/usart_uart_dma/source/main.h
 * @brief This file contains the including files of main routine.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2022-03-31       CDT             First version
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
#ifndef __MAIN_H__
#define __MAIN_H__

#include "hc32_ll.h"
#include "ev_hc32f460_lqfp100_v2_bsp.h"
#include "flash.h"
#include "uart_dma.h"
#include "i2c.h"
#include "videoprocess.h"
#include "string.h"
#include "stdbool.h"

#define GPIO_PORT_ASW  GPIO_PORT_B

#define GPIO_PIN_ASW1  GPIO_PIN_15
#define GPIO_PIN_ASW2  GPIO_PIN_14
#define GPIO_PIN_ASW3  GPIO_PIN_13
#define GPIO_PIN_ASW4  GPIO_PIN_12

#define ASW1_On()     GPIO_SetPins(GPIO_PORT_ASW,GPIO_PIN_ASW1)
#define ASW2_On()     GPIO_SetPins(GPIO_PORT_ASW,GPIO_PIN_ASW2)
#define ASW3_On()     GPIO_SetPins(GPIO_PORT_ASW,GPIO_PIN_ASW3)
#define ASW4_On()     GPIO_SetPins(GPIO_PORT_ASW,GPIO_PIN_ASW4)

#define ASW1_Off()    GPIO_ResetPins(GPIO_PORT_ASW,GPIO_PIN_ASW1)
#define ASW2_Off()    GPIO_ResetPins(GPIO_PORT_ASW,GPIO_PIN_ASW2)
#define ASW3_Off()    GPIO_ResetPins(GPIO_PORT_ASW,GPIO_PIN_ASW3)
#define ASW4_Off()    GPIO_ResetPins(GPIO_PORT_ASW,GPIO_PIN_ASW4)

#define AV_Connecte_Off()    GPIO_SetPins(GPIO_PORT_A,GPIO_PIN_08) 
#define AV_Connecte_On()     GPIO_ResetPins(GPIO_PORT_A,GPIO_PIN_08) 

#define DEFAULT 0
#define USER    1
#define FLASH_LEAF_ADDR(x)          (uint32_t)(0x7a000U+(0x2000U * (x)))

#define APP_FRAME_LEN_MAX               (500U)

/* R48 R77 R79  (200 => 75 )*/
#define RES_CHANGED  true//true  false


// 寄存器地址
#define VID_SEL_REG  0x02  // 视频制式选择寄存器
#define AUTO_DETECT_REG 0x07  // 自动检测控制寄存器

// 设置视频制式的宏
#define VID_SEL_PAL    0x80  // PAL 制式
#define VID_SEL_NTSC   0x50  // NTSC M 制式
#define VID_SEL_SECAM  0xE0  // SECAM 制式
#define VID_SEL_AUTO   0x00  // 自动检测模式

// 设置自动检测制式的宏
#define AD_PAL_EN      0x01  // 启用PAL检测
#define AD_NTSC_EN     0x02  // 启用NTSC检测
#define AD_SECAM_EN    0x04  // 启用SECAM检测


#define LED_ERR_RED     0x80
#define LED_ERR_GREEN   0x40
#define LED_ERR_BLUE    0x20
#define LED_OK          0x10


//#define LED_ERR_RED 0x80

#define LED_TIME 35

#define Boundary 10

//#define true       1
//#define false      0



#define AV_INPUT false
#define SV_INPUT true

//全局定义
/*LED_state*/
extern uint8_t led_state;
extern uint8_t led_sw;


//extern uint8_t Adv_7391_sw; 
extern uint8_t adv_input ;
extern uint8_t adv_double ;
extern uint8_t adv_smooth ;
extern uint8_t adv_ace ;
extern uint8_t adv_sw  ;
extern uint8_t adv_tv  ;
extern uint8_t err_flag ;
extern uint8_t c_state;
extern bool asw_01,asw_02,asw_03,asw_04 ;
extern bool AVsw;
extern uint8_t Input_signal;

extern uint8_t Bright     ;
extern uint8_t Contrast   ;
extern uint8_t Saturation ;


extern uint8_t btn_flag;




extern uint8_t dma_au8RxBuf[APP_FRAME_LEN_MAX];










#endif /* __MAIN_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
