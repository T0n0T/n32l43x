#include "qpc.h" /* QP/C API */
#include "bsp.h"
#include "valve.h"
#include "log.h"
#include "guard.h"
#ifdef USE_LORAWAN
#include "at_lora.h"
#endif
#ifdef DEBUG
#include "cm_backtrace.h"
#endif

/* Use for sleep judgement */
volatile uint32_t Sleep_bits;
volatile bool     run_is_reporting;
volatile bool     pvd_is_power_low;
volatile bool     transfer_is_error;
static QEvt       _lock_evt;
static QEvt       _update_evt;
static uint32_t   exiting    = false;
static uint32_t   exit_count = 0;

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
        // at_lorawan_event_post();
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

static void wakeup_handle(uint8_t source, uint8_t bit)
{
    APP_LOG_DEBUG("wakeup_handle: source = %d, bit = %d", source, bit);
    if (source == WAKE_SRC_RFID) {
        if (bit == Bit_SET) {
            exit_count = 0;
            exiting    = false;
        }
    }
    if (source == WAKE_SRC_KEY) {
        if (bit == Bit_RESET) {
            exit_count = 0;
            exiting    = false;
        }
    }
}

static void pvd_handle(void)
{
    pvd_is_power_low = !pvd_is_power_low;
}

static void lptimer_handle(void)
{
    static uint32_t lptime_tick = 0;
    lptime_tick++;
    if (lptime_tick % (LPTIM_REPORT_MS / LPTIM_INTERVAL_MS) == 0) {
        run_is_reporting = true;
        lptime_tick      = 0;
    }
    if (lptime_tick % (LPTIM_SENSOR_MS / LPTIM_INTERVAL_MS) == 0) {
        QEvt_ctor(&_update_evt, VALVE_UPDATE_SIG);
        QACTIVE_POST(AO_ValveHandler, &_update_evt, 0);
    }
    if ((GPIO_ReadInputDataBit(GPIOC, GPIO_PIN_13) == Bit_SET) && GPIO_ReadInputDataBit(GPIOB, GPIO_PIN_1) == Bit_RESET) {
        exit_count++;
        APP_LOG_INFO("Exiting! count = %d!", exit_count);
    } else {
        exit_count = 0;
    }
    if (exit_count >= (LPTIM_EXIT_THRESHOLD_MS / LPTIM_INTERVAL_MS) && !exiting) {
        QEvt_ctor(&_lock_evt, VALVE_LOCK_SIG);
        QACTIVE_PUBLISH(&_lock_evt, 0);
        exiting = true;
    }

    guard_process();
}

/*..........................................................................*/
void QV_onIdle(void)
{
    extern void valve_idle(void);
    valve_idle();
    if (!Sleep_bits && !run_is_reporting) {
        SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
        // PWR_EnterSTOP2Mode(PWR_STOPENTRY_WFI, PWR_CTRL3_RAM1RET | PWR_CTRL3_RAM2RET);
        // SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
        // SetSysClockToPLL(SystemCoreClock, SYSCLK_PLLSRC_HSE_PLLDIV2);
        // SystemCoreClockUpdate();
        APP_LOG_INFO("EXIT!");
        GPIO_ResetBits(GPIOB, GPIO_PIN_9);
        while (1);
        // PWR_EnterSTANDBYMode(PWR_STOPENTRY_WFI, PWR_CTRL3_RAM2RET);
    } else {
        /* NOTE: should not use SLEEPONEXIT mode, it will cause qv sheduling blocked
         */
        PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
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
    DBG_ConfigPeriph(DBG_SLEEP | DBG_STOP | DBG_CTRL_TIM1_STOP, ENABLE);
#endif
    board_init(); /* initialize the board */
    led_init();   /* initialize the LEDs */

    uart_init(CONSOLE);
    APP_LOG_RAW(" \r\n");
    APP_LOG_RAW("┌──────────────────────────────────────────────┐\r\n");
    APP_LOG_RAW("│   N32L43x AirPre App  %s-%s   │\r\n", __DATE__, __TIME__);
    APP_LOG_RAW("└──────────────────────────────────────────────┘\r\n");
    lptimer_init();
    // dump_clk();
}

void BSP_start(void)
{
    // initialize event pools
    // static QF_MPOOL_EL(QEvt) smlPoolSto[10];
    // QF_poolInit(smlPoolSto, sizeof(smlPoolSto), sizeof(smlPoolSto[0]));

    // initialize publish-subscribe
    static QSubscrList subscrSto[MAX_PUB_SIG];
    QActive_psInit(subscrSto, Q_DIM(subscrSto));

    // instantiate and start AOs/threads...

    static QEvtPtr valveHandlerQueueSto[128];
    ValveHandler_ctor();
    QActive_start(AO_ValveHandler,
                  4U,
                  valveHandlerQueueSto,
                  Q_DIM(valveHandlerQueueSto),
                  (void*)0, 0U,
                  (void*)0);
    // ValvePersist_ctor();
    // static QEvtPtr valvePersistQueueSto[16];
    // QActive_start(AO_ValvePersist,
    //               3U,
    //               valvePersistQueueSto,
    //               Q_DIM(valvePersistQueueSto),
    //               (void*)0, 0U,
    //               (void*)0);
    static QEvtPtr valveConfQueueSto[16];
    ValveConf_ctor();
    QActive_start(AO_ValveConf,
                  2U,
                  valveConfQueueSto,
                  Q_DIM(valveConfQueueSto),
                  (void*)0, 0U,
                  (void*)0);
    // static QEvtPtr valveTransferQueueSto[16];
    // ValveTransfer_ctor();
    // QActive_start(AO_ValveTransfer,
    //               1U,
    //               valveTransferQueueSto,
    //               Q_DIM(valveTransferQueueSto),
    //               (void*)0, 0U,
    //               (void*)0);
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

    NVIC_SetPriorityGrouping(NVIC_PriorityGroup_0);
    NVIC_SetPriority(RTC_IRQn, DEF_ISR_PRI);
    NVIC_SetPriority(LPTIM_WKUP_IRQn, DEF_ISR_PRI);
    NVIC_SetPriority(USART2_IRQn, DEF_ISR_PRI - 1);
    NVIC_SetPriority(UART5_IRQn, DEF_ISR_PRI - 2);
    NVIC_SetPriority(TIM1_UP_IRQn, DEF_ISR_PRI - 2);
    NVIC_SetPriority(EXTI0_IRQn, DEF_ISR_PRI - 2);
    NVIC_SetPriority(EXTI1_IRQn, DEF_ISR_PRI - 2);
    NVIC_SetPriority(EXTI15_10_IRQn, DEF_ISR_PRI - 2);

    // rtc_init();
    wakeup_init(wakeup_handle);
    SysTick_Config(RCC_ClockFreq.SysclkFreq / TICK_RATE);
    // if (GPIO_ReadInputDataBit(GPIOC, GPIO_PIN_13) == Bit_RESET) {
    //     Wake_source |= WAKE_SRC_KEY;
    //     APP_LOG_DEBUG("key wakeup");
    // }
    // if (GPIO_ReadInputDataBit(GPIOB, GPIO_PIN_1) == Bit_SET) {
    //     Wake_source |= WAKE_SRC_RFID;
    //     APP_LOG_DEBUG("rifd wakeup");
    // }
    pvd_init(pvd_handle);
    lptimer_init();
    lptimer_start(LPTIMER_MS_TO_TICKS(LPTIM_INTERVAL_MS), lptimer_handle);
    QEvt_ctor(&_lock_evt, VALVE_UNLOCK_SIG);
    QACTIVE_PUBLISH(&_lock_evt, 0);
}
/*..........................................................................*/
void QF_onCleanup(void)
{
}
