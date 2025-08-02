/*****************************************************************************
 * BSP for EK-TM4C123GXL with QP/C framework
 *****************************************************************************/
#include "qpc.h" /* QP/C API */
#include "bsp.h"
#include "board.h"
#include "log.h"
#include "hello.h" /* Blinky Application interface */
#include "cmd.h"   /* Command interface */
#ifdef DEBUG
#include "cm_backtrace.h"
#include "SEGGER_SYSVIEW.h"
#endif

#define BTN_SW1 (1U << 4)
#define BTN_SW2 (1U << 0)

/* Assertion handler  ======================================================*/
Q_NORETURN Q_onAssert(char const* module, int_t id)
{
    APP_LOG_RAW("ERROR in %s:%d\r\n", module, id);
#ifdef DEBUG /* debug build? */
    cm_backtrace_assert(cmb_get_sp());
    while (1); /* tie the CPU in this endless loop */
#endif
    NVIC_SystemReset(); /* reset the CPU */
}
//............................................................................
/* assert-handling function called by exception handlers in the startup code */
void assert_failed(const uint8_t* expr, const uint8_t* file, uint32_t line)
{
    APP_LOG_RAW("ERROR in %s:%d\r\n", file, line);
#ifdef DEBUG /* debug build? */
    cm_backtrace_assert(cmb_get_sp());
    while (1); /* tie the CPU in this endless loop */
#endif
    NVIC_SystemReset(); /* reset the CPU */
}

/* ISRs  ===============================================*/
void SysTick_Handler(void)
{
    QTIMEEVT_TICK_X(0, 0);
    QV_ARM_ERRATUM_838869();
}

/*..........................................................................*/
void QV_onIdle(void)
{
    static bool     sleep = false;
    static uint32_t count = 0;
    if (sleep) {
        SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
        PWR_EnterSTOP2Mode(PWR_STOPENTRY_WFI, PWR_CTRL3_RAM1RET | PWR_CTRL3_RAM2RET);
        SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
        led_toggle(LED_2);
        APP_LOG_DEBUG("wakeup");        
        sleep = false;
    } else {
        count++;
        if (count == 5000) {
            count = 0;
            sleep = true;
        }
        PWR_EnterSLEEPMode(SLEEP_OFF_EXIT, PWR_SLEEPENTRY_WFI);
    }
    QF_INT_ENABLE();
}

/* BSP functions ===========================================================*/
void BSP_init(void)
{
/* NOTE: SystemInit() has been already called from the startup code
 *  but SystemCoreClock needs to be updated
 */
#ifdef DEBUG
    cm_backtrace_init("build/n32l43x", "V1.0", "1.0.0");
    DBG_ConfigPeriph(DBG_SLEEP | DBG_STOP | DBG_TIM1_STOP, ENABLE);
#endif
    board_init();       /* initialize the board */
    led_init();         /* initialize the LEDs */
    uart_init(CONSOLE); /* initialize the console UART */
    APP_LOG_INFO("MCU RESET Systemclock: %d", SystemCoreClock);
    lptimer_init();
}

void BSP_start(void)
{
    // initialize event pools
    static QF_MPOOL_EL(QEvt) smlPoolSto[10];
    QF_poolInit(smlPoolSto, sizeof(smlPoolSto), sizeof(smlPoolSto[0]));

    // initialize publish-subscribe
    static QSubscrList subscrSto[10];
    QActive_psInit(subscrSto, Q_DIM(subscrSto));

    // instantiate and start AOs/threads...

    static QEvtPtr helloQueueSto[10];
    Hello_ctor();
    QActive_start(AO_Hello,
                  1U,                   // QP prio. of the AO
                  helloQueueSto,        // event queue storage
                  Q_DIM(helloQueueSto), // queue length [events]
                  (void*)0, 0U,         // no stack storage
                  (void*)0);            // no initialization param
}

/*..........................................................................*/
void QF_onStartup(void)
{
#define NVIC_PRI_GROUP 4

    /* set priorities of ALL ISRs used in the system, see NOTE1 */
    // NVIC_SetPriorityGrouping(NVIC_PRI_GROUP);
    NVIC_SetPriority(SysTick_IRQn, 0xf);
    NVIC_SetPriority(LPTIM_WKUP_IRQn, 0);

    SysTick->LOAD = (uint32_t)(SystemCoreClock / BSP_TICKS_PER_SEC - 1UL);
    SysTick->VAL  = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
    lptimer_start(LPTIMER_MS_TO_TICKS(1000), NULL);
}
/*..........................................................................*/
void QF_onCleanup(void)
{
}

/*..........................................................................*/
void BSP_ledOn(void)
{
    led_on(LED_3);
}

/*..........................................................................*/
void BSP_ledOff(void)
{
    led_off(LED_3);
}
