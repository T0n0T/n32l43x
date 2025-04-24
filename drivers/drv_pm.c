#include <drv_pm.h>

#ifdef RT_USING_PM

#if defined(BSP_USING_LPTIMER)
#include <rtdevice.h>
#include <drv_lptimer.h>
#define LPTIM_REG_MAX_VALUE (0xFFFF)

static rt_device_t timer = RT_NULL;

rt_weak void n32_pm_device_run(struct rt_pm* pm, uint8_t mode)
{
    /*todo add your code here*/
}

static rt_tick_t n32_os_tick_from_pm_tick(rt_uint32_t tick)
{
    static rt_uint32_t os_tick_remain = 0;
    rt_tick_t          os_tick        = 0;
    rt_uint32_t        freq           = 0;
    rt_hwtimer_t*      hwtimer        = (rt_hwtimer_t*)timer;
    RT_ASSERT(hwtimer != RT_NULL);
    RT_ASSERT(hwtimer->ops != RT_NULL);
    RT_ASSERT(hwtimer->ops->control != RT_NULL);
    hwtimer->ops->control(hwtimer, DRV_HW_LPTIMER_CTRL_GET_FREQ, &freq);
    os_tick = (tick * RT_TICK_PER_SECOND + os_tick_remain) / freq;

    os_tick_remain += (tick * RT_TICK_PER_SECOND);
    os_tick_remain %= freq;

    return os_tick;
}

static rt_tick_t n32_pm_tick_from_os_tick(rt_tick_t tick)
{
    rt_uint32_t freq = 0;
    rt_hwtimer_t* hwtimer = (rt_hwtimer_t*)timer;
    RT_ASSERT(hwtimer != RT_NULL);
    RT_ASSERT(hwtimer->ops != RT_NULL);    
    RT_ASSERT(hwtimer->ops->control != RT_NULL);
    hwtimer->ops->control(hwtimer, DRV_HW_LPTIMER_CTRL_GET_FREQ, &freq);
    return (tick * freq / RT_TICK_PER_SECOND);
}

static void sleep(struct rt_pm* pm, uint8_t mode)
{
    switch (mode) {
        case PM_SLEEP_MODE_NONE:
            break;

        case PM_SLEEP_MODE_IDLE:
            break;

        case PM_SLEEP_MODE_LIGHT:
            PWR_EnterSLEEPMode(SLEEP_OFF_EXIT, PWR_SLEEPENTRY_WFI);
            break;

        case PM_SLEEP_MODE_DEEP:
            PWR_EnterSTOP2Mode(PWR_STOPENTRY_WFI, PWR_CTRL3_RAM1RET | PWR_CTRL3_RAM2RET);
            /*multiply System Clock Frequency*/
            set_sysclock_to_pll(SystemCoreClock, SYSCLK_PLLSRC_HSE_PLLDIV2);
            break;

        case PM_SLEEP_MODE_STANDBY:
            PWR_EnterSTANDBYMode(PWR_STOPENTRY_WFI, PWR_CTRL3_RAM2RET);
            break;

        case PM_SLEEP_MODE_SHUTDOWN:

            break;

        default:
            break;
    }
}

static void pm_timer_start(struct rt_pm* pm, rt_uint32_t timeout)
{
    RT_ASSERT(pm != RT_NULL);
    RT_ASSERT(timeout > 0);
    RT_ASSERT(timer != RT_NULL);

    rt_hwtimer_t* hwtimer = (rt_hwtimer_t*)timer;
    if (hwtimer->ops == RT_NULL || hwtimer->info == RT_NULL || hwtimer->ops->start == RT_NULL) return;

    if (timeout != RT_TICK_MAX) {
        rt_uint32_t max_cnt = hwtimer->info->maxcnt;

        /* Convert OS Tick to pmtimer timeout value */
        timeout = n32_pm_tick_from_os_tick(timeout);

        if (timeout > max_cnt) {
            timeout = max_cnt;
        }

        hwtimer->ops->start(hwtimer, timeout, HWTIMER_MODE_ONESHOT);
    }
}

static void pm_timer_stop(struct rt_pm* pm)
{
    RT_ASSERT(pm != RT_NULL);
    RT_ASSERT(timer != RT_NULL);

    /* Reset pmtimer status */
    rt_hwtimer_t* hwtimer = (rt_hwtimer_t*)timer;
    if (hwtimer->ops == RT_NULL || hwtimer->ops->stop == RT_NULL) {
        // LOG_E("Get PM timer %s count failed", timer->parent.name);
    } else {
        hwtimer->ops->stop(hwtimer);
    }
}

static rt_tick_t pm_timer_get_tick(struct rt_pm* pm)
{
    rt_uint32_t timer_tick;

    RT_ASSERT(pm != RT_NULL);
    RT_ASSERT(timer != RT_NULL);

    rt_hwtimer_t* hwtimer = (rt_hwtimer_t*)timer;
    if (hwtimer->ops == RT_NULL || hwtimer->ops->count_get == RT_NULL) {
        // LOG_E("Get PM timer %s count failed", timer->parent.name);
        return 0;
    } else {
        timer_tick = hwtimer->ops->count_get(hwtimer);
        return n32_os_tick_from_pm_tick(timer_tick);
    }
}

static const struct rt_pm_ops _ops = {
    sleep,
    n32_pm_device_run,
    pm_timer_start,
    pm_timer_stop,
    pm_timer_get_tick,
};

int rt_pm_hw_init(void)
{
    rt_uint8_t timer_mask = 0;

    /* initialize timer mask */
    timer_mask = 1UL << PM_SLEEP_MODE_DEEP;
    // timer_mask |= 1UL << PM_SLEEP_MODE_LIGHT;

    /* initialize system pm module */
    rt_system_pm_init(&_ops, timer_mask, RT_NULL);

    /* initialize lptimer */
    timer = rt_device_find("lptimer");

    if (timer == RT_NULL) {
        return -RT_ERROR;
    } else {
        return rt_device_init(timer);
    }
}
INIT_CORE_EXPORT(rt_pm_hw_init);
#endif /* BSP_USING_LPTIMER */
#endif /* RT_USING_PM */