/*****************************************************************************
 * BSP for EK-TM4C123GXL with QP/C framework
 *****************************************************************************/
#include "qpc.h" /* QP/C API */
#include "bsp.h"
#include "valve.h"
#include "log.h"

#ifdef DEBUG
#include "cm_backtrace.h"
#include "SEGGER_SYSVIEW.h"
#endif

/* Use for sleep judgement */
uint32_t    Sleep_bits;
static QEvt _lock_evt;


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
#ifdef DEBUG
    SEGGER_SYSVIEW_RecordEnterISR();
#endif
    QTIMEEVT_TICK_X(0, 0);
#ifdef DEBUG
    SEGGER_SYSVIEW_RecordExitISRToScheduler();
#endif
    QV_ARM_ERRATUM_838869();
}

static void wakeup_handle(uint8_t bit)
{
    if (bit == Bit_RESET) {
        QEvt_ctor(&_lock_evt, VALVE_LOCK_SIG);
        QACTIVE_PUBLISH(&_lock_evt, 0);
    } else {
        QEvt_ctor(&_lock_evt, VALVE_UNLOCK_SIG);
        QACTIVE_PUBLISH(&_lock_evt, 0);
    }
}

static void pvd_handle(void)
{
    /* 设置守卫检阅值 */
}

/*..........................................................................*/
void QV_onIdle(void)
{
    extern void valve_idle(void);
    valve_idle();
    QV_CPU_SLEEP();
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
    cm_backtrace_init("build/n32l43x", "V1.0", "1.0.0");
    SEGGER_SYSVIEW_Conf();
#endif
    NVIC_SetPriorityGrouping(4);
    NVIC_SetPriority(SysTick_IRQn, DEF_ISR_PRI);
    NVIC_SetPriority(RTC_IRQn, DEF_ISR_PRI);
    NVIC_SetPriority(USART2_IRQn, DEF_ISR_PRI - 1);
    NVIC_SetPriority(UART5_IRQn, DEF_ISR_PRI - 2);
    NVIC_SetPriority(TIM1_UP_IRQn, DEF_ISR_PRI - 2);
    NVIC_SetPriority(EXTI15_10_IRQn, DEF_ISR_PRI - 2);
    board_init(); /* initialize the board */
    led_init();   /* initialize the LEDs */
    hall_init();  /* initialize the Hall sensor */
    uart_init(CONSOLE);
    APP_LOG_RAW(" \r\n");
    APP_LOG_RAW("┌──────────────────────────────────────────────┐\r\n");
    APP_LOG_RAW("│   N32L43x Valve App  %s-%s    │\r\n", __DATE__, __TIME__);
    APP_LOG_RAW("└──────────────────────────────────────────────┘\r\n");
    lcd_init(); /* initialize the LCD */
    // dump_clk();
    // rtc_init();
    // wakeup_init(wakeup_handle);
    // pvd_init(pvd_handle);
}

void BSP_start(void)
{
#ifdef DEBUG
    extern SEGGER_SYSVIEW_TASKINFO _Q_taskInfo[3];
#endif
    // initialize event pools
    // static QF_MPOOL_EL(QEvt) smlPoolSto[10];
    // QF_poolInit(smlPoolSto, sizeof(smlPoolSto), sizeof(smlPoolSto[0]));

    // initialize publish-subscribe
    static QSubscrList subscrSto[MAX_PUB_SIG];
    QActive_psInit(subscrSto, Q_DIM(subscrSto));

    // instantiate and start AOs/threads...

    static QEvtPtr valveCounterQueueSto[128];
    ValveCounter_ctor();
    QActive_start(AO_ValveCounter,
                  4U,
                  valveCounterQueueSto,
                  Q_DIM(valveCounterQueueSto),
                  (void*)0, 0U,
                  (void*)0);

    static QEvtPtr valveHandlerQueueSto[128];
    ValveHandler_ctor();
    QActive_start(AO_ValveHandler,
                  3U,
                  valveHandlerQueueSto,
                  Q_DIM(valveHandlerQueueSto),
                  (void*)0, 0U,
                  (void*)0);
    static QEvtPtr valveConfQueueSto[16];
    ValveConf_ctor();
    QActive_start(AO_ValveConf,
                  2U,
                  valveConfQueueSto,
                  Q_DIM(valveConfQueueSto),
                  (void*)0, 0U,
                  (void*)0);
    static QEvtPtr valveValvePersistQueueSto[16];
    ValvePersist_ctor();
    QActive_start(AO_ValvePersist,
                  1U,
                  valveValvePersistQueueSto,
                  Q_DIM(valveValvePersistQueueSto),
                  (void*)0, 0U,
                  (void*)0);

#ifdef DEBUG
    _Q_taskInfo[0].TaskID = (uint32_t)AO_ValveCounter;
    _Q_taskInfo[0].sName  = "AO_ValveCounter";
    _Q_taskInfo[0].Prio   = 4U;
    _Q_taskInfo[1].TaskID = (uint32_t)AO_ValveHandler;
    _Q_taskInfo[1].sName  = "AO_ValveHandler";
    _Q_taskInfo[1].Prio   = 3U;
    _Q_taskInfo[2].TaskID = (uint32_t)AO_ValveConf;
    _Q_taskInfo[2].sName  = "AO_ValveConf";
    _Q_taskInfo[2].Prio   = 2U;
    _Q_taskInfo[3].TaskID = (uint32_t)AO_ValvePersist;
    _Q_taskInfo[3].sName  = "AO_ValvePersist";
    _Q_taskInfo[3].Prio   = 1U;
#endif
}

/*..........................................................................*/
void QF_onStartup(void)
{
    RCC_ClocksType RCC_ClockFreq;
    RCC_GetClocksFreqValue(&RCC_ClockFreq);
    APP_LOG_INFO("SYSCLK: %u", (unsigned int)RCC_ClockFreq.SysclkFreq);
    APP_LOG_INFO("HCLK: %u", (unsigned int)RCC_ClockFreq.HclkFreq);
    APP_LOG_INFO("PCLK1: %u", (unsigned int)RCC_ClockFreq.Pclk1Freq);
    APP_LOG_INFO("PCLK2: %u", (unsigned int)RCC_ClockFreq.Pclk2Freq);
    SysTick_Config(RCC_ClockFreq.SysclkFreq / TICK_RATE);
    // if (GPIO_ReadInputDataBit(GPIOC, GPIO_PIN_13) == SET) {
    QEvt_ctor(&_lock_evt, VALVE_UNLOCK_SIG);
    QACTIVE_PUBLISH(&_lock_evt, 0);
    // }
}
/*..........................................................................*/
void QF_onCleanup(void)
{
}

#ifdef QF_ON_CONTEXT_SW
void QF_onContextSw(QActive* prev, QActive* next)
{
#ifdef DEBUG
    if (prev) {
        SEGGER_SYSVIEW_OnTaskStopExec();
    }
    if (next) {
        SEGGER_SYSVIEW_OnTaskStartExec((uint32_t)next);
    }
#endif
}
#endif