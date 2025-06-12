#ifndef LCD_H
#define LCD_H
#include "board.h"

typedef enum {
    LCD_CHAR_OPEN_CHINESE,
    LCD_CHAR_OPEN_ARROW,
    LCD_CHAR_CLOSE_CHINESE,
    LCD_CHAR_CLOSE_ARROW,
    LCD_CHAR_BATTERY_4,
    LCD_CHAR_BATTERY_3,
    LCD_CHAR_BATTERY_2,
    LCD_CHAR_BATTERY_1,
    LCD_CHAR_BATTERY_0,
} LCD_Char_Enum;

void lcd_init(void);
void lcd_set_char(LCD_Char_Enum lcd_char, bool enable);

#endif // LCD_H