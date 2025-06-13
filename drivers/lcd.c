#include "lcd.h"

#define COM_NUM_MAX  3
#define CHAR_NUM_MAX 9

const uint32_t lcd_bit_define[CHAR_NUM_MAX][COM_NUM_MAX] = {
    {0x00000000, 0x00000000, 0x00004000}, // LCD_CHAR_OPEN_CHINESE
    {0x00000000, 0x00000000, 0x00001000}, // LCD_CHAR_OPEN_ARROW
    {0x00004000, 0x00000000, 0x00000000}, // LCD_CHAR_CLOSE_CHINESE
    {0x00001000, 0x00000000, 0x00000000}, // LCD_CHAR_CLOSE_ARROW
    {0x00002000, 0x00007000, 0x00002000}, // LCD_CHAR_BATTERY_4
    {0x00002000, 0x00003000, 0x00002000}, // LCD_CHAR_BATTERY_3
    {0x00002000, 0x00001000, 0x00002000}, // LCD_CHAR_BATTERY_2
    {0x00002000, 0x00001000, 0x00000000}, // LCD_CHAR_BATTERY_1
    {0x00000000, 0x00001000, 0x00000000}, // LCD_CHAR_BATTERY_0
};

void lcd_init(void)
{
    LCD_InitType     Init = {0};
    LCD_ErrorTypeDef ret  = 0;
    /*LCD parameter config*/
    Init.Divider         = LCD_DIV_25;
    Init.Prescaler       = LCD_PRESCALER_1;
    Init.Duty            = LCD_DUTY_1_3;
    Init.Bias            = LCD_BIAS_1_2;
    Init.VoltageSource   = LCD_VOLTAGESOURCE_EXTERNAL;
    Init.Contrast        = LCD_CONTRASTLEVEL_5;
    Init.DeadTime        = LCD_DEADTIME_0;
    Init.PulseOnDuration = LCD_PULSEONDURATION_1;
    Init.HighDrive       = LCD_HIGHDRIVE_ENABLE;
    Init.HighDriveBuffer = LCD_HIGHDRIVEBUFFER_ENABLE;
    Init.BlinkMode       = LCD_BLINKMODE_OFF;
    Init.BlinkFreq       = LCD_BLINKFREQ_DIV_8;
    Init.MuxSegment      = LCD_MUXSEGMENT_DISABLE;

    /* Initialize the LCD clk */
    LCD_ClockConfig(LCD_CLK_SRC_LSI);

    /* Initialize used gpio */
    GPIO_InitType lcd_gpio_initstruct = {0};
    GPIO_InitStruct(&lcd_gpio_initstruct);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA | RCC_APB2_PERIPH_GPIOB | RCC_APB2_PERIPH_GPIOC, ENABLE);

    /*
    PB12: SEG12
    PB13: SEG13
    PB14: SEG14
    */
    lcd_gpio_initstruct.Pin            = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14;
    lcd_gpio_initstruct.GPIO_Mode      = GPIO_Mode_Analog;
    lcd_gpio_initstruct.GPIO_Pull      = GPIO_No_Pull;
    lcd_gpio_initstruct.GPIO_Alternate = GPIO_AF10_LCD;
    GPIO_InitPeripheral(GPIOB, &lcd_gpio_initstruct);

    /*
    PA15: COM3
    PC10: COM4
    PC11: COM5
    */
    lcd_gpio_initstruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitPeripheral(GPIOC, &lcd_gpio_initstruct);

    lcd_gpio_initstruct.Pin            = GPIO_PIN_15;
    lcd_gpio_initstruct.GPIO_Alternate = GPIO_AF11_LCD;
    GPIO_InitPeripheral(GPIOA, &lcd_gpio_initstruct);

    /*config and start LCD controller*/
    ret = LCD_Init(&Init);
    assert_param(ret == LCD_ERROR_OK);

    LCD_RamClear();
    LCD_UpdateDisplayRequest();
}

void lcd_set_char(LCD_Char_Enum lcd_char, bool enable)
{
    if (lcd_char >= CHAR_NUM_MAX || lcd_char < 0) {
        return; // Invalid parameters
    }

    if (enable) {
        LCD_SetBit(LCD_RAM1_COM3, lcd_bit_define[lcd_char][0]);
        LCD_SetBit(LCD_RAM1_COM4, lcd_bit_define[lcd_char][1]);
        LCD_SetBit(LCD_RAM1_COM5, lcd_bit_define[lcd_char][2]);
    } else {
        LCD_ClearBit(LCD_RAM1_COM3, lcd_bit_define[lcd_char][0]);
        LCD_ClearBit(LCD_RAM1_COM4, lcd_bit_define[lcd_char][1]);
        LCD_ClearBit(LCD_RAM1_COM5, lcd_bit_define[lcd_char][2]);
    }

    LCD_UpdateDisplayRequest();
}
