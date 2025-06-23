/*****************************************************************************
 * BSP for EK-TM4C123GXL with QP/C framework
 *****************************************************************************/
#include "qpc.h" /* QP/C API */
#include "bsp.h"
#include "valve.h"
#ifdef DEBUG
#include "cm_backtrace.h"
#endif

/* Use for sleep judgement */
uint32_t Sleep_bits;

/* Assertion handler  ======================================================*/
Q_NORETURN Q_onAssert(char const* module, int_t id)
{
    printf("ERROR in %s:%d\r\n", module, id);
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
    printf("ERROR in %s:%d\r\n", file, line);
#ifdef DEBUG /* debug build? */
    cm_backtrace_assert(cmb_get_sp());
    while (1); /* tie the CPU in this endless loop */
#endif
    NVIC_SystemReset(); /* reset the CPU */
}

/* ISRs  ===============================================*/
void RTC_WKUP_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_LINE20)) {
        EXTI_ClrITPendBit(EXTI_LINE20);
        RTC_ClrIntPendingBit(RTC_INT_WUT);
        QTIMEEVT_TICK_X(0, 0);
    }
    QV_ARM_ERRATUM_838869();
}

void SysTick_Handler(void)
{
    QTIMEEVT_TICK_X(0, 0);
    QV_ARM_ERRATUM_838869();
}

static void wakeup_handle(uint8_t bit)
{
    if (bit == Bit_RESET) {
        static QEvt const wakeEvt = QEVT_INITIALIZER(LOCK_ON_SIG);
        QACTIVE_PUBLISH(&wakeEvt, &me->super);
    } else {
        static QEvt const wakeEvt = QEVT_INITIALIZER(LOCK_OFF_SIG);
        QACTIVE_PUBLISH(&wakeEvt, &me->super);
    }
}

/*..........................................................................*/
void QV_onIdle(void)
{
    QF_INT_ENABLE();
    if (!Sleep_bits) {
        // PWR_EnterSTOP2Mode(PWR_STOPENTRY_WFI, PWR_CTRL3_RAM1RET | PWR_CTRL3_RAM2RET);
    }
}

/* BSP functions ===========================================================*/
void BSP_init(void)
{
    /* NOTE: SystemInit() has been already called from the startup code
     *  but SystemCoreClock needs to be updated
     */
#ifdef DEBUG
    cm_backtrace_init("N32L4", "V1.0", "1.0.0");
#endif
    board_init(); /* initialize the board */
    led_init();   /* initialize the LEDs */
    hall_init();  /* initialize the Hall sensor */
    uart_init(CONSOLE);
    printf(" \r\n");
    printf("┌──────────────────────────────────────────────┐\r\n");
    printf("│   N32L43x Valve App  %s-%s    │\r\n", __DATE__, __TIME__);
    printf("└──────────────────────────────────────────────┘\r\n");
    lcd_init(); /* initialize the LCD */
    lcd_set_char(LCD_CHAR_CLOSE_CHINESE, true);
    lcd_set_char(LCD_CHAR_CLOSE_ARROW, true);
    lcd_set_char(LCD_CHAR_OPEN_CHINESE, false);
    lcd_set_char(LCD_CHAR_OPEN_ARROW, false);
    // dump_clk();
    // rtc_init();
    // wakeup_pin_init(wakeup_handle);
}

void BSP_start(void)
{
    // initialize event pools
    static QF_MPOOL_EL(QEvt) smlPoolSto[10];
    QF_poolInit(smlPoolSto, sizeof(smlPoolSto), sizeof(smlPoolSto[0]));

    // initialize publish-subscribe
    static QSubscrList subscrSto[MAX_PUB_SIG];
    QActive_psInit(subscrSto, Q_DIM(subscrSto));

    // instantiate and start AOs/threads...

    static QEvtPtr valveCounterQueueSto[10];
    ValveCounter_ctor();
    QActive_start(AO_ValveCounter,
                  1U,
                  valveCounterQueueSto,
                  Q_DIM(valveCounterQueueSto),
                  (void*)0, 0U,
                  (void*)0);
    static QEvtPtr valveHandlerQueueSto[10];
    ValveHandler_ctor();
    QActive_start(AO_ValveHandler,
                  2U,
                  valveHandlerQueueSto,
                  Q_DIM(valveHandlerQueueSto),
                  (void*)0, 0U,
                  (void*)0);
    static QEvtPtr valveConfQueueSto[10];
    ValveConf_ctor();
    QActive_start(AO_ValveConf,
                  3U,
                  valveConfQueueSto,
                  Q_DIM(valveConfQueueSto),
                  (void*)0, 0U,
                  (void*)0);
}

/*..........................................................................*/
void QF_onStartup(void)
{
    SysTick_Config(SystemCoreClock / 1000);
    NVIC_SetPriority(SysTick_IRQn, QF_AWARE_ISR_CMSIS_PRI + 1U);
    static QEvt const wakeEvt = QEVT_INITIALIZER(LOCK_OFF_SIG);
    QACTIVE_PUBLISH(&wakeEvt, &me->super);
}
/*..........................................................................*/
void QF_onCleanup(void)
{
}
