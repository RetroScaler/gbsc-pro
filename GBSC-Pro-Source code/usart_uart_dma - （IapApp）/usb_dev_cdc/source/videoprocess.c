#include "videoprocess.h"
#include "ev_hc32f460_lqfp100_v2_bsp.h"
#include "main.h"

uint8_t btn_flag;


static uint8_t status = 0;
uint8_t Adv_7391_sw = 0;
uint8_t adv_input;
uint8_t adv_double = true;
uint8_t adv_smooth = false;
uint8_t adv_ace = false;
uint8_t adv_sw = false;
uint8_t adv_tv = 0xff;
uint8_t err_flag = 0;
uint8_t c_state = 0;

uint8_t Bright = 0x00;
uint8_t Contrast = 0x80;
uint8_t Saturation = 0x80;

bool asw_01, asw_02, asw_03, asw_04; //  01 :  02 : 兼容开关  03 ：     04 ：

bool AVsw;
uint8_t Input_signal = 0;
uint8_t buff_send[APP_FRAME_LEN_MAX];

uint8_t I2C_COMMANDS_I2P_ON[] =
{
    0x84,0x55,0x80, // ADV7280 - I2C_DEINT_ENABLE: Enable I2P Converter
    0x84,0x5A,0x02, // ADV7280 - Not documented on page 99 - Configure I2P Parameters Smooth 1A (?)
};

//  I2C_COMMANDS_I2P_OFF_p
//  I2C_COMMANDS_I2P_OFF_240p
const uint8_t I2C_COMMANDS_I2P_OFF_p[] = {
   0x84,0x55,0x00, // ADV7280 - I2C_DEINT_ENABLE: Disable I2P Converter
   0x84,0x5A,0x02, // ADV7280 - Not documented on page 99 - Configure I2P Parameters Smooth 1A (?)
};

//  I2C_COMMANDS_I2P_OFF_p
//  I2C_COMMANDS_I2P_OFF_576i
uint8_t I2C_COMMANDS_I2P_OFF_576i[] =
{
    0x84,0x55,0x00, // ADV7280 - I2C_DEINT_ENABLE: Disable I2P Converter
    0x84,0x5B,0x00, // ADV7280 - ADV_TIMING_MODE_EN: Enable advanced timing mode
    0x84,0x5A,0x02, // ADV7280 - Not documented on page 99 - Configure I2P Parameters Smooth 1A (?)
    0x56,0x17,0x02, // ADV7391 - Software reset: Software reset
    0xFF,0x0A,0x00, // Unknown
    0x56,0x00,0x1C, // ADV7391 - Power mode: DAC1 on - DAC2 on - DAC3 on - PLL off - Sleep off
    0x56,0x01,0x00, // ADV7391 - Mode select: reset
    0x56,0x80,0x10, // ADV7391 - SD Mode Register 1: reset
    0x56,0x82,0xC9, // ADV7391 - SD Mode Register 2: SD active video edge control on, SD pixel data valid on, SD VCR FF/RW sync off, SD square pixel mode off, SD pedestal on, SD DAC Output 1 B or Pb (need to check), SD PrPb SSAF Filter on
    0x56,0x87,0x20, // ADV7391 - SD Mode Register 6: SD RGB input enable: YCrCb, SD Input standard auto detect on, all others off
    0x56,0x88,0x00, // ADV7391 - SD Mode Register 7: reset
    0x56,0x30,0x10, // ADV7391 - ED/HD standard: BTA-1004, ITU-BT.1362 525p at 59.94 Hz, ED/HD input synchronization format: External HSYNC, VSYNC and field inputs, ED/HD output standard: EIA-770.2 output EIA-770.3 output
    0xFF,0x0A,0x00, // Unknown
    0x84,0x55,0x00, // ADV7280 - I2C_DEINT_ENABLE: Disable I2P Converter
    0x84,0x5A,0x02  // ADV7280 - Not documented on page 99 - Configure I2P Parameters Smooth 1A (?)
};


uint8_t I2C_COMMANDS_SMOOTH_OFF[] =
    {
        0x84, 0x55, 0x80,
        0x84, 0x5A, 0x02, // 平滑关闭
        0x42, 0x0E, 0x00};

uint8_t I2C_COMMANDS_SMOOTH_ON[] =
    {
        0x84, 0x55, 0x80, //
        0x84, 0x5A, 0x1A, // 平滑打开
        0x42, 0x0E, 0x00  //
};
uint8_t I2C_COMMANDS_BCSH[] =
{
    0x42,0x0E,0x00, // ADV7280 - ADI Control 1: main register
    0x42,0x0A,0x00, // ADV7280 - Brightness adjust: 0 IRE
    0x42,0x08,0x00, // ADV7280 - Contrast: 0 gain [0x80 (default) for 1 gain]
    0x42,0xE3,0x00, // ADV7280 - SD saturation Cb channel: -42dB [0x80 (default) for 0dB]
    0x42,0x0B,0x00, // ADV7280 - Hue adjust: 0 default
};

uint8_t I2C_COMMANDS_YC_INPUT[] =
{
    /* =============== ADV7280 S-Video =============== */
    0x42,0x0E,0x00, // ADV7280 - ADI Control 1: main register
    0x42,0x00,0x09, // ADV7280 - Input control: Y input on A3, C input on A4
    0x42,0x38,0x24, // ADV7280 - NTSC comb control: Disable chroma comb, Uses low-pass/notch filter
    0x42,0x39,0x24, // ADV7280 - PAL comb control: Disable chroma comb, Uses low-pass/notch filter
    0x42,0x17,0x49, // ADV7280 - Shaping Filter Control 1: SH1, SVHS 8
};

uint8_t I2C_COMMANDS_CVBS_INPUT[] =
{
    /* =============== ADV7280 CVBS =============== */
    0x42,0x0E,0x00, // ADV7280 - ADI Control 1: main register
    0x42,0x00,0x00, // ADV7280 - Input control: CVBS input on A1
    0x42,0x38,0x80, // ADV7280 - NTSC comb control: default
    0x42,0x39,0x24, // ADV7280 - PAL comb control: Disable chroma comb, Uses low-pass/notch filter
    0x42,0x17,0x47, // ADV7280 - Shaping Filter Control 1: SH1, SVHS 6
};

void SoftwareReset_ADV7280A()
{
    uint8_t buff[2];
    buff[0] = 0x0F;
    buff[1] = 0x80;
    (void)I2C_Master_Transmit(DEVICE_ADDR, buff, 2, TIMEOUT); // 复位 ADV7280 的所有寄存器
    DDL_DelayMS(10);

    buff[0] = 0x0F;
    buff[1] = 0x00;
    (void)I2C_Master_Transmit(DEVICE_ADDR, buff, 2, TIMEOUT); // 复位 ADV7280 的所有寄存器
    DDL_DelayMS(10);
}
// RetroScaler_I2C_AUTO_COMMANDS
// I2C_AUTO_COMMANDS
uint8_t I2C_AUTO_COMMANDS[] =
{
    // Initial reset
    0x42,0x0F,0x80, // ADV7280 - Power management: Start reset sequence
    0x56,0x17,0x02, // ADV7391 - Software reset: Software reset
    0xFF,0x0A,0x00, // Unknown - Delay 10ms
    0x42,0x0F,0x00, // ADV7280 - Power management: Normal operation

    // Configure input
    0x42,0x05,0x00, // ADV7280 - Not documented on page 71
    0x42,0x02,0x04, // ADV7280 - Video Selection 2: Autodetect, set to default
    0x42,0x07,0xFF, // ADV7280 - Autodetect enable: All enabled
    0x42,0x14,0x15, // ADV7280 - Analog clamp control: Current sources enabled, Sets to default, Boundary box
    0x42,0x00,0x00, // ADV7280 - Input control: CVBS input on A1

    // ADI Required write
    0x42,0x0E,0x80, // ADV7280 - ADI Required Write; Reset Current Clamp Circuitry (step1)
    0x42,0x9C,0x00, // ADV7280 - ADI Required Write; Reset Current Clamp Circuitry (step2)
    0x42,0x9C,0xFF, // ADV7280 - ADI Required Write; Reset Current Clamp Circuitry (step3)
    0x42,0x0E,0x00, // ADV7280 - ADI Required Write; Reset Current Clamp Circuitry (step4)

    // Power Up Digital Output Pads
    0x42,0x03,0x0C, // ADV7280 - Enable Pixel & Sync output drivers
    0x42,0x04,0x07, // ADV7280 - Power-up INTRQ, HS & VS pads
    0x42,0x13,0x00, // ADV7280 - Enable ADV7182 for 28_63636MHz crystal

    // Configuration
    0x42,0x17,0x40, // ADV7280 - Shaping Filter Control 1: SH1, Autowide notch for poor quality sources or wideband filter with comb for good quality input
    0x42,0x1D,0x40, // ADV7280 - Enable LLC output driver

    // ADI Required Write for Fast Switch
    0x42,0x52,0xCD, // ADV7280 - Single Ended CVBS - Set optimized IBIAS for the AFE
    0x42,0x80,0x51, // ADV7280 - ADI Required Write
    0x42,0x81,0x51, // ADV7280 - ADI Required Write
    0x42,0x82,0x00, // ADV7280 - ADI Required Write
    0x42,0x0E,0x80, // ADV7280 - ADI Required Write
    0x42,0xD9,0x44, // ADV7280 - ADI Required Write
    0x42,0x0E,0x00, // ADV7280 - ADI Control 1: main register
    
    // Enable I2P
    0x42,0xFD,0x84, // ADV7280 - set the VPP address to 0x84
    0x84,0xA3,0x00, // ADV7280 - ADI Required Write - VPP writes begin
    0x84,0x5B,0x00, // ADV7280 - ADV_TIMING_MODE_EN: Enable advanced timing mode
    0x84,0x55,0x80, // ADV7280 - I2C_DEINT_ENABLE - Enable I2P Converter
    // 0x85, 0x5A, 0x85 //read the reg 0x5A
    0x84,0x5A,0x02, // ADV7280 - Disable I2P Smooth 1A
    0x42,0x6B,0x11, // ADV7280 - Output Sync Select 2: VSYNC

    // Reset ADV7391
    0x56,0x17,0x02, // ADV7391 - Software reset: Software reset
    0xFF,0x0A,0x00, // Unknown - Delay 10ms

    // Encoder configuration
    0x56,0x00,0x9C, // 0x1C ? // ADV7391 - Power up DACs and PLL [Encoder writes begin]
    0x56,0x01,0x70, // ADV7391 - Mode select: ED (at 54MHz) input, Chrome rising, luma falling
    0x56,0x30,0x1C, // 0x04 NTSC // ADV7391 - ED/HD Mode Register 1: SMPTE 296M-4, SMPTE 274M-5 720p at 30 Hz/29.97 Hz, EIA-770.2 output EIA-770.3 output
    0x56,0x31,0x01, // ADV7391 - ED/HD Mode Register 2: Pixel data valid on
    0x42,0x0E,0x00, // ADV7280 - ADI Control 1: main register
};


uint8_t Ace_Code_ON[] =
    {
        0x42, 0x0E, 0x40,
        0x42, 0x80, 0x80,
        0x42, 0x0E, 0x00
    };

uint8_t Ace_Code_OFF[] =
    {
        0x42, 0x0E, 0x40,
        0x42, 0x80, 0x00,
        0x42, 0x0E, 0x00
    };
    
    
// 525p Encoder Configuration
uint8_t I2C_COMMANDS_525p_CONFIG[] = {
    0x56,0x30,0x04,
    0x56,0x31,0x11,
};

// 625p Encoder Configuration
uint8_t I2C_COMMANDS_625p_CONFIG[] = {
    0x56,0x30,0x1C,
    0x56,0x31,0x11,
};

/**
 * @brief Configure video encoder based on detected video standard
 * 
 * This function comes from the brisma/gbsc-pro project:
 * https://github.com/brisma/gbsc-pro
 * encoder for 525p or 625p standards accordingly.
 */
// Function to configure video encoder
static void configure_video_encoder()
{
    // Extract bits [6:5:4] for AD_RESULT
    uint8_t ad_result;
    uint8_t buff[1] = { 0x10 };
    Chip_Receive(DEVICE_ADDR, &buff[0], &ad_result, 1, TIMEOUT);

    uint8_t ad_standard = (ad_result & 0x70) >> 4;
    uint8_t is_525p = 0;
    
    // Determine if it's 525p or 625p
    // AD_RESULT values:
    // 0: NTSC M/NTSC J (525p)
    // 1: NTSC 4.43 (525p)
    // 2: PAL M (525p)
    // 3: PAL 60 (525p)
    // 4: PAL B/PAL G/PAL H/PAL I/PAL D (625p)
    // 5: SECAM (625p)
    // 6: PAL Combination N (625p)
    // 7: SECAM 525 (525p)
    
    if (ad_standard == 0 || ad_standard == 1 || ad_standard == 2 || ad_standard == 3 || ad_standard == 7) {
        is_525p = 1; // 525p
    } else {
        is_525p = 0; // 625p
    }
    
    if (is_525p) {
        // Send 525p configuration
        (void)ADV_7280_Send_Buff(I2C_COMMANDS_525p_CONFIG, sizeof(I2C_COMMANDS_525p_CONFIG) / 3, TIMEOUT);
        printf("525p format configured\n");
    }

    if (!is_525p) {
        // Send 625p configuration
        (void)ADV_7280_Send_Buff(I2C_COMMANDS_625p_CONFIG, sizeof(I2C_COMMANDS_625p_CONFIG) / 3, TIMEOUT);
        printf("625p format configured\n");
    }
}

void video_init(void)
{
    uint8_t count = 0;
    uint8_t buff[2] = {0xe0, 0x00};

    GPIO_SetPins(POWER_DOWN_PORT, GPIO_PIN_POWER_DOWN); // POWER_DOWN_N
    DDL_DelayMS(5);
    GPIO_SetPins(INPUT_RESET_PORT, GPIO_PIN_INPUT_RESET); // INPUT_RESET_N
    DDL_DelayMS(10);
    GPIO_SetPins(OUTPUT_PORT, GPIO_PIN_OUTPUT_EN); // OUTPUT_EN
    DDL_DelayMS(10);

    (void)Chip_Receive(DEVICE_ADDR, buff, NULL, 1, TIMEOUT);
    DDL_DelayMS(15);

    // SoftwareReset_ADV7280A();
    (void)ADV_7280_Send_Buff(I2C_AUTO_COMMANDS, sizeof(I2C_AUTO_COMMANDS) / 3, TIMEOUT); // AUTO_REGS
    set_input(adv_input); // read button

    buff[0] = VID_SEL_REG;
    buff[1] = adv_tv; // adv_tv
    printf(" Init adv_tv: 0x%02x", adv_tv);
    (void)I2C_Master_Transmit(DEVICE_ADDR, buff, 2, TIMEOUT); // mode
    
    // Configure video encoder based on detection
    configure_video_encoder();
    set_bcsh();
    if (adv_double)
    {
        set_double_line(adv_double); // GPIO_ReadInputPins(GPIO_PORT_B,GPIO_PIN_05)
        if (adv_smooth)
            set_smooth(adv_smooth);
    }
    else
    {
        set_double_line(true);
        set_smooth(true);
        adv_smooth = false;
        set_smooth(adv_smooth);
        set_double_line(adv_double);
    }

    DDL_DelayMS(200);

    Adv_7391_sw = 1;
    printf("ModuleOn\n");
    //    led_state &= ~0x01;
}

void video_Deinit(void)
{
    GPIO_ResetPins(POWER_DOWN_PORT, GPIO_PIN_POWER_DOWN);   // POWER_DOWN_N
    GPIO_ResetPins(INPUT_RESET_PORT, GPIO_PIN_INPUT_RESET); // INPUT_RESET_N
    GPIO_ResetPins(OUTPUT_PORT, GPIO_PIN_OUTPUT_EN);        // OUTPUT_EN
    Adv_7391_sw = 0;
    printf("ModuleOff\n");
    //    led_state = 0x01;
}

void Video_Sw(uint8_t sw)
{
    if (sw == true)
    {
        video_init();
    }
    //    else
    //    {
    //        video_Deinit();
    //    }
}

void set_input(uint8_t input)
{
    if (input)
    {
        (void)ADV_7280_Send_Buff(I2C_COMMANDS_YC_INPUT, sizeof(I2C_COMMANDS_YC_INPUT) / 3, TIMEOUT);
        printf("SvSignal\n");
        //        led_state &= ~(LED_GREEN);
        //        led_state |= LED_BLUE;
    }
    else
    {
        (void)ADV_7280_Send_Buff(I2C_COMMANDS_CVBS_INPUT, sizeof(I2C_COMMANDS_CVBS_INPUT) / 3, TIMEOUT);
        printf("AvSignal\n");
        //        led_state &= ~(LED_BLUE);
        //        led_state |= LED_GREEN;
    }
}

void set_double_line(uint8_t doubleline)
{
    if (doubleline)
    {
        (void)ADV_7280_Send_Buff(I2C_COMMANDS_I2P_ON, sizeof(I2C_COMMANDS_I2P_ON) / 3, TIMEOUT); // I2P——ON
        printf("I2pOn\n");
    }
    else
    {   

        (void)ADV_7280_Send_Buff(I2C_COMMANDS_I2P_OFF_p, sizeof(I2C_COMMANDS_I2P_OFF_p) / 3, TIMEOUT); //_240p _576i
        printf("I2pOff\n");
    }
}

void set_smooth(uint8_t smooth)
{
    if (smooth)
    {
        (void)ADV_7280_Send_Buff(I2C_COMMANDS_SMOOTH_ON, sizeof(I2C_COMMANDS_SMOOTH_ON) / 3, TIMEOUT);
        printf("SmoothOn\n");
    }
    else
    {
        (void)ADV_7280_Send_Buff(I2C_COMMANDS_SMOOTH_OFF, sizeof(I2C_COMMANDS_SMOOTH_OFF) / 3, TIMEOUT);
        printf("SmoothOff\n");
    }
}

void set_bcsh(void)
{

    I2C_COMMANDS_BCSH[5] = Bright;
    I2C_COMMANDS_BCSH[8] = Contrast;
    I2C_COMMANDS_BCSH[11] = Saturation;
    (void)ADV_7280_Send_Buff(I2C_COMMANDS_BCSH, sizeof(I2C_COMMANDS_BCSH) / 3, TIMEOUT);
    printf("Bright    : 0x%02x\n", Bright);
    printf("Contrast  : 0x%02x\n", Contrast);
    printf("Saturation: 0x%02x\n", Saturation);
};

void set_ace(uint8_t ace)
{
    if (ace)
    {
        (void)ADV_7280_Send_Buff(Ace_Code_ON, sizeof(Ace_Code_ON) / 3, TIMEOUT);
        printf("AceOn\n");
    }
    else
    {
        (void)ADV_7280_Send_Buff(Ace_Code_OFF, sizeof(Ace_Code_OFF) / 3, TIMEOUT);
        printf("AceOff\n");
    }
}

void Read_Video_key_change(void)
{
    static uint8_t key_state = 0, key_state_last = 0;

    key_state = GPIO_ReadInputPins(GPIO_PORT_B, GPIO_PIN_12);
    if (key_state_last != key_state)
    {
        adv_input = key_state;
        set_input(adv_input);
    }
    key_state_last = key_state;

    static uint8_t key_line_state = 0, key_state_line_last = 0;
    key_line_state = GPIO_ReadInputPins(GPIO_PORT_B, GPIO_PIN_05);
    if (key_state_line_last != key_line_state)
    {
        set_double_line(key_line_state);
    }
    key_state_line_last = key_line_state;
}

void detect_video_format(uint8_t btn_flag)
{   
    if(!btn_flag)
        return;
    
    uint8_t reg13 = 0, reg12 = 0, reg10 = 0;
    uint8_t buff[2];

    buff[0] = 0x13;
    Chip_Receive(DEVICE_ADDR, &buff[0], &reg13, 1, TIMEOUT);

    buff[0] = 0x12;
    Chip_Receive(DEVICE_ADDR, &buff[0], &reg12, 1, TIMEOUT);

    buff[0] = 0x10;
    Chip_Receive(DEVICE_ADDR, &buff[0], &reg10, 1, TIMEOUT);

    uint8_t interlaced = (reg13 & 0x40) >> 6;
    uint8_t in_lock    = (reg13 & 0x01);
    uint8_t ll_nstd    = (reg12 & 0x10) >> 4;
    uint8_t ad_result  =  reg10 & 0x70; // bits 6~4

    if (!in_lock) {
        printf("No signal lock.\n");
        return;
    }

    if (!interlaced)
    {
      if (ad_result == 0x40)
      {
        printf("288p (PAL progressive)\n");
      }
      else if (ad_result == 0x00 || ad_result == 0x20)
      {
        // NTSC 模式下逐行信号
        printf("240p (NTSC progressive)\n");
      }
      else
      {
        printf("Unknown\n");
      }
    }
    else
    {
      // interlaced = 1
      if (ad_result == 0x40)
      {
        printf("576i (PAL interlaced)\n");
      }
      else if (ad_result == 0x00 || ad_result == 0x20)
      {
        printf("480i (NTSC interlaced)\n");
      }
      else
      {
        printf("Unknown\n");
      }
    }
}


void detect_loop(void)
{
    if (Adv_7391_sw == 1)
    {

        uint8_t detect_result, ad_result;
        uint8_t buff[2];

        buff[0] = 0x0E;
        buff[1] = 0x00;
        (void)I2C_Master_Transmit(DEVICE_ADDR, buff, 2, TIMEOUT);
        DDL_DelayMS(10);

        buff[0] = 0x0E;
        buff[1] = 0x00;
        (void)I2C_Master_Transmit(DEVICE_ADDR, buff, 2, TIMEOUT);

        buff[0] = 0x13;
        buff[1] = 0x10;
        Chip_Receive(DEVICE_ADDR, &buff[0], &detect_result, 1, TIMEOUT);
        Chip_Receive(DEVICE_ADDR, &buff[1], &ad_result, 1, TIMEOUT);
        
        detect_video_format(btn_flag);
//        if ((uint8_t)(detect_result & 0x81) == 0x80 && (err_flag))
//        {
//            if (adv_input == AV_INPUT)
//            {
//                //                video_init();
//                set_input(SV_INPUT);
//                set_input(AV_INPUT);
//                err_flag = 0;
//            }
//            else if (adv_input == SV_INPUT)
//            {
//                //                video_init();
//                set_input(AV_INPUT);
//                set_input(SV_INPUT);
//                err_flag = 0;
//            }

//            led_sw = 0x01 | LED_ERR_RED;
//            //          led_state |= LED_RED;
//            printf("err\n");
//            err_flag = 0;
//        }
        if (((uint8_t)(ad_result & 0x02) == 0x02) && !status && (err_flag))
        {
            status = 1;
            //            set_double_line(false);
            //            set_smooth(false);

            //            if(adv_double)
            //            {
            //                set_double_line(adv_double); // GPIO_ReadInputPins(GPIO_PORT_B,GPIO_PIN_05)
            //                if(adv_smooth)
            //                    set_smooth(adv_smooth);
            //            }
            //            else
            //            if(adv_double)
            //            {
            //                set_double_line(true);
            //                set_smooth(true);
            //                set_smooth(false);
            //                set_double_line(false);
            //            }

            //            Adv_7391_sw = 1;
            printf(" Run Free Mode \n");
            c_state = 2;
            err_flag = 0;
            led_state = LED_RED;
        }
        else if (status && ((uint8_t)(ad_result & 0x05) == 0x05) && (err_flag))
        {
            status = 0;
            err_flag = 0;

            //            if(adv_double)
            //            {
            //                set_double_line(adv_double);
            //                set_smooth(adv_smooth);
            //            }
            printf(" Close Free Mode \n");
            Signal_led(Input_signal);
        }

        //        if (((uint8_t)(ad_result & 0x25)==0x25))  //PAL 60
        //        {
        ////            buff[0] = 0x02;
        ////            buff[1] = 0xB4;    //PAL M
        //            buff[0] = 0x02;
        //            buff[1] = 0x64;  //PAL 60
        //        }   //                                                                             60    Bit2==0
        //        //检测到摆动脉冲串Bit7==1  检测到标清50赫兹Bit2==1  实现水平锁定Bit0 ==1
        //        else  if ( ((uint8_t)(ad_result & 0x15)==0x15) && ((uint8_t)(detect_result&0x85) == 0x81))   //NTSC443
        //        {
        ////            buff[0] = 0x02;
        ////            buff[1] = 0x64;  //PAL 60
        //            buff[0] = 0x02;
        //            buff[1] = 0xB4;    //PAL M
        //        }
        //        else
        //        {
        //            buff[0] = 0x02;
        //            buff[1] = 0x04;  //Auto
        //        }

        //        (void)I2C_Master_Transmit(DEVICE_ADDR, buff, 2 , TIMEOUT);
    }
    else
        return;
}

void ASC(uint8_t sw1, uint8_t sw2, uint8_t sw3, uint8_t sw4, uint8_t state)
{
    if ((sw1) && ((sw1 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW1)) || state))
        ASW1_On();
    else if ((!sw1) && ((sw1 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW1)) || state))
        ASW1_Off();

    if ((sw2) && ((sw2 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW2)) || state))
        ASW2_On();
    else if ((!sw2) && ((sw2 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW2)) || state))
        ASW2_Off();

    if ((sw3) && ((sw3 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW3)) || state))
        ASW3_On();
    else if ((!sw3) && ((sw3 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW3)) || state))
        ASW3_Off();

    if ((sw4) && ((sw4 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW4)) || state))
        ASW4_On();
    else if ((!sw4) && ((sw4 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW4)) || state))
        ASW4_Off();
    //    mem_settings();
}

void ASC_Not02(uint8_t sw1, uint8_t sw2, uint8_t sw3, uint8_t sw4, uint8_t state)
{
    if ((sw1) && ((sw1 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW1)) || state))
        ASW1_On();
    else if ((!sw1) && ((sw1 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW1)) || state))
        ASW1_Off();

    //    if  (sw2)
    //        ASW2_On();
    //    else
    //        ASW2_Off();

    if ((sw3) && ((sw3 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW3)) || state))
        ASW3_On();
    else if ((!sw3) && ((sw3 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW3)) || state))
        ASW3_Off();

    if ((sw4) && ((sw4 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW4)) || state))
        ASW4_On();
    else if ((!sw4) && ((sw4 != GPIO_ReadInputPins(GPIO_PORT_ASW, GPIO_PIN_ASW4)) || state))
        ASW4_Off();
    //    mem_settings();
}

void AV_Connecte_Set(uint8_t sw)
{
    stc_gpio_init_t stcGpioInit;
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDir = PIN_DIR_OUT;       // 输出方向
    stcGpioInit.u16PinAttr = PIN_ATTR_DIGITAL; // 数字
    stcGpioInit.u16PinDrv = PIN_HIGH_DRV;
    stcGpioInit.u16PinOutputType = PIN_OUT_TYPE_CMOS;
    if (sw)
    {
        //        AV_Connecte_On();
        stcGpioInit.u16PullUp = PIN_PU_OFF;     // 不上拉
        stcGpioInit.u16PinState = PIN_STAT_RST; // 拉低
        printf("Connecte_on");
        //        C_LED_OK();
        c_state = 1;
    }
    else
    {
        //        AV_Connecte_Off();
        stcGpioInit.u16PullUp = PIN_PU_ON;      // 上拉
        stcGpioInit.u16PinState = PIN_STAT_SET; // 拉高
        printf("Connecte_off");
        //        C_LED_ERR_RED();
        c_state = 2;
    }

    (void)GPIO_Init(GPIO_PORT_A, GPIO_PIN_08, &stcGpioInit);
}

// void SwRgbSync_On(void)
//{
//     asw_01=1;
//     asw_03=1;
// }

// void SwSyncOnGreen_On(void)
//{
//     asw_04=0;
// }

// void SwRgbSync_Off(void)
//{
//     asw_01=1;
//     asw_03=1;
// }

// void SwSyncOnGreen_Off(void)
//{
//     asw_04=0;
// }

void Signal_led(uint8_t signal)
{
    switch (signal)
    {
    case 1: // RGBs
    {
        led_state = LED_RED | LED_GREEN | LED_BLUE;
    }
    break;
    case 2: // RGsB
    {
        led_state = LED_GREEN;
    }
    break;
    case 3: // VGA
    {
        led_state = LED_BLUE;
    }
    break;
    case 4: // YUV
    {
        led_state = LED_RED | LED_GREEN;
    }
    break;
    case 5: // SV
    {
        led_state = LED_RED | LED_BLUE;
    }
    break;
    case 6: // AV
    {
        led_state = LED_GREEN | LED_BLUE;
    }
    break;
    }
}

void Reser_Status(void)
{
    status = 0;
}