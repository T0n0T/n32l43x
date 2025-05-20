#include "lcd.h"

void lcd_init(void)
{
    LCD_InitType Init = {0};

    /*LCD parameter config*/
    Init.Divider         = LCD_DIV_25;
    Init.Prescaler       = LCD_PRESCALER_1;
    Init.Duty            = LCD_DUTY_1_3;
    Init.Bias            = LCD_BIAS_1_2;
    Init.VoltageSource   = LCD_VOLTAGESOURCE_INTERNAL;
    Init.Contrast        = LCD_CONTRASTLEVEL_7;
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
    GPIO_InitType led_gpio_initstruct = {0};
    GPIO_InitStruct(&led_gpio_initstruct);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB | RCC_APB2_PERIPH_GPIOC | RCC_APB2_PERIPH_GPIOD, ENABLE);

    /*
    PB4: SEG8
    PB5: SEG9
    PB10:SEG10
    PB11:SEG11
    */
    led_gpio_initstruct.Pin            = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_10 | GPIO_PIN_11;
    led_gpio_initstruct.GPIO_Mode      = GPIO_Mode_Analog;
    led_gpio_initstruct.GPIO_Pull      = GPIO_No_Pull;
    led_gpio_initstruct.GPIO_Alternate = GPIO_AF10_LCD;
    GPIO_InitPeripheral(GPIOB, &led_gpio_initstruct);

    /*
    PC10: COM4
    PC11: COM5
    PC12: COM6
    PD2:  COM7
    */
    led_gpio_initstruct.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitPeripheral(GPIOC, &led_gpio_initstruct);

    led_gpio_initstruct.Pin = GPIO_PIN_2;
    GPIO_InitPeripheral(GPIOD, &led_gpio_initstruct);

    /*config and start LCD controller*/
    LCD_Init(&Init);

    // EXTI_InitType EXTI_InitStructure;

    // EXTI_InitStruct(&EXTI_InitStructure);
    // EXTI_InitStructure.EXTI_Line    = EXTI_LINE26;
    // EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    // EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    // EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    // EXTI_InitPeripheral(&EXTI_InitStructure);

    // NVIC_InitType NVIC_InitStructure;

    // NVIC_InitStructure.NVIC_IRQChannel                   = LCD_IRQn;
    // NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    // NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 3;
    // NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    // NVIC_Init(&NVIC_InitStructure);

    // __LCD_CLEAR_FLAG(LCD_FLAG_SOF_CLEAR);
    // __LCD_ENABLE_IT(LCD_IT_SOF);

    LCD_SetBit(LCD_RAM1_COM0, 0xFFFFFFFF);
    LCD_SetBit(LCD_RAM1_COM1, 0xFFFFFFFF);
    LCD_SetBit(LCD_RAM1_COM2, 0xFFFFFFFF);
    LCD_SetBit(LCD_RAM1_COM3, 0xFFFFFFFF);
    LCD_SetBit(LCD_RAM1_COM4, 0xFFFFFFFF);
    LCD_SetBit(LCD_RAM1_COM5, 0xFFFFFFFF);
    LCD_SetBit(LCD_RAM1_COM6, 0xFFFFFFFF);
    LCD_SetBit(LCD_RAM1_COM7, 0xFFFFFFFF);

    LCD_UpdateDisplayRequest();
}

void LCD_IRQHandler(void)
{

    if (EXTI_GetStatusFlag(EXTI_LINE26) == SET) {
        EXTI_ClrStatusFlag(EXTI_LINE26);

    }
}