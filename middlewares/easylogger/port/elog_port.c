#include <elog.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

static struct rt_semaphore output_lock;

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
static struct rt_semaphore output_notice;

static void async_output(void* arg);
#endif

int rt_elog_init(void)
{
    /* initialize EasyLogger */
    elog_init();
    /* set EasyLogger log format */
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL & ~ELOG_FMT_FUNC);
    /* start EasyLogger */
    elog_start();
    return 0;
}
INIT_APP_EXPORT(rt_elog_init);

/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void)
{
    ElogErrCode result = ELOG_NO_ERR;

    rt_sem_init(&output_lock, "elog lock", 1, RT_IPC_FLAG_PRIO);

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    rt_thread_t async_thread = NULL;

    rt_sem_init(&output_notice, "elog async", 0, RT_IPC_FLAG_PRIO);

    async_thread = rt_thread_create("elog_async", async_output, NULL, 1024, RT_THREAD_PRIORITY_MAX - 1, 10);
    if (async_thread) {
        rt_thread_startup(async_thread);
    }
#endif
    return result;
}

/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char* log, size_t size)
{
    /* output to terminal */
    rt_kprintf("%.*s", size, log);
    /* output to flash */
    // elog_flash_write(log, size);
}

/**
 * output lock
 */
void elog_port_output_lock(void)
{
    rt_sem_take(&output_lock, RT_WAITING_FOREVER);
}

/**
 * output unlock
 */
void elog_port_output_unlock(void)
{
    rt_sem_release(&output_lock);
}

/**
 * get current time interface
 *
 * @return current time
 */
const char* elog_port_get_time(void)
{
    static char cur_system_time[16] = {0};
    rt_snprintf(cur_system_time, 16, "tick:%010d", rt_tick_get());
    return cur_system_time;
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char* elog_port_get_p_info(void)
{
    return "";
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char* elog_port_get_t_info(void)
{
    return rt_thread_self()->parent.name;
}

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
void elog_async_output_notice(void)
{
    rt_sem_release(&output_notice);
}

static void async_output(void* arg)
{
    size_t      get_log_size = 0;
    static char poll_get_buf[ELOG_LINE_BUF_SIZE - 4];

    while (true) {
        /* waiting log */
        rt_sem_take(&output_notice, RT_WAITING_FOREVER);
        rt_pm_request(PM_SLEEP_MODE_NONE);
        /* polling gets and outputs the log */
        while (true) {

#ifdef ELOG_ASYNC_LINE_OUTPUT
            get_log_size = elog_async_get_line_log(poll_get_buf, sizeof(poll_get_buf));
#else
            get_log_size = elog_async_get_log(poll_get_buf, sizeof(poll_get_buf));
#endif

            if (get_log_size) {
                elog_port_output(poll_get_buf, get_log_size);
            } else {
                break;
            }
        }
        /* release pm request */
        rt_pm_release(PM_SLEEP_MODE_NONE);
    }
}
#endif