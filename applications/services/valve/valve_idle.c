#include "qpc.h"
#include "bsp.h"
#include "valve.h"
#include "cmd.h"
#include "log.h"
#ifdef DEBUG
#include "SEGGER_SYSVIEW.h"
#endif

static uint32_t last_status;

void valve_idle(void)
{
#ifdef DEBUG
    SEGGER_SYSVIEW_OnIdle();
#endif
    if (last_status != global_valve_value->current_status) {
#ifdef DEBUG
        SEGGER_SYSVIEW_MarkStart(VALVE_IDLE);
#endif
        if (global_valve_value->current_status == VALVE_STATUS_ON) {
            LCD->RAM_COM[LCD_RAM1_COM0] = 0x00000000;
            LCD->RAM_COM[LCD_RAM1_COM1] = 0x00000000;
            LCD->RAM_COM[LCD_RAM1_COM2] = 0x00005000;
            __LCD_CLEAR_FLAG(LCD_FLAG_UDD_CLEAR);
            __LCD_UPDATE_REQUEST();
#ifdef USE_MODBUS
            ucSCoilBuf[0] |= (1 << 0);
#endif
        } else if (global_valve_value->current_status == VALVE_STATUS_OFF) {
            LCD->RAM_COM[LCD_RAM1_COM0] = 0x00005000;
            LCD->RAM_COM[LCD_RAM1_COM1] = 0x00000000;
            LCD->RAM_COM[LCD_RAM1_COM2] = 0x00000000;
            __LCD_CLEAR_FLAG(LCD_FLAG_UDD_CLEAR);
            __LCD_UPDATE_REQUEST();
#ifdef USE_MODBUS
            ucSCoilBuf[0] &= ~(1 << 0);
#endif
        }
#ifdef DEBUG
        SEGGER_SYSVIEW_MarkStop(VALVE_IDLE);
#endif
        last_status = global_valve_value->current_status;
    }
}