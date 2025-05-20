/*****************************************************************************
 * BSP for EK-TM4C123GXL with QP/C framework
 *****************************************************************************/
#include "qpc.h" /* QP/C API */
#include "bsp.h"
#include "board.h"
#include "blinky.h" /* Blinky Application interface */
#include "hall.h"
#include "lcd.h"
#include "led.h"
#include "rtc.h"
#include "uart.h"
#include "cm_backtrace.h"

/* Assertion handler  ======================================================*/
Q_NORETURN Q_onAssert(char const* module, int_t id)
{
    printf("ERROR in %s:%d\r\n", module, id);
#ifndef NDEBUG  /* debug build? */
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
#ifndef NDEBUG /* debug build? */
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

/*..........................................................................*/
void QV_onIdle(void)
{
    QF_INT_ENABLE();
    // PWR_EnterSTOP2Mode(PWR_STOPENTRY_WFI, PWR_CTRL3_RAM1RET | PWR_CTRL3_RAM2RET);
}

/* BSP functions ===========================================================*/
void BSP_init(void)
{
    /* NOTE: SystemInit() has been already called from the startup code
     *  but SystemCoreClock needs to be updated
     */
    SystemCoreClockUpdate();
    cm_backtrace_init("N32L4", "V1.0", "1.0.0");
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
    led_init(); /* initialize the LEDs */
    uart_init();
    // rtc_init();
    hall_init();
    hall_set_ctr(ENABLE);
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

    static QEvtPtr blinkyQueueSto[10];
    Blinky_ctor();
    QActive_start(AO_Blinky,
                  1U,                    // QP prio. of the AO
                  blinkyQueueSto,        // event queue storage
                  Q_DIM(blinkyQueueSto), // queue length [events]
                  (void*)0, 0U,          // no stack storage
                  (void*)0);             // no initialization param
}

/*..........................................................................*/
void QF_onStartup(void)
{
}
/*..........................................................................*/
void QF_onCleanup(void)
{
}

/*..........................................................................*/
void BSP_ledOn(void)
{
    led_on(LED_3);
    printf("LED ON\r\n");
}

/*..........................................................................*/
void BSP_ledOff(void)
{
    led_off(LED_3);
    printf("LED OFF\r\n");
}
