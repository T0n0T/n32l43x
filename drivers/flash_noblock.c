#include "flash_noblock.h"
#include "SEGGER_SYSVIEW.h"

/* 超时时间定义 */
#define FLASH_OP_TIMEOUT_ERASE   0x000B0000
#define FLASH_OP_TIMEOUT_PROGRAM 0x00002000

/* Flash Access Control Register bits */
#define AC_LATENCY_MSK           ((uint32_t)0x000000F8)
#define AC_PRFTBE_MSK            ((uint32_t)0xFFFFFFEF)
#define AC_ICAHEN_MSK            ((uint32_t)0xFFFFFF7F)
#define AC_LVMEN_MSK             ((uint32_t)0xFFFFFDFF)
#define AC_SLMEN_MSK             ((uint32_t)0xFFFFF7FF)

/* Flash Access Control Register bits */
#define AC_PRFTBS_MSK            ((uint32_t)0x00000020)
#define AC_ICAHRST_MSK           ((uint32_t)0x00000040)
#define AC_LVMF_MSK              ((uint32_t)0x00000100)
#define AC_SLMF_MSK              ((uint32_t)0x00000400)

/* Flash Control Register bits */
#define CTRL_Set_PG              ((uint32_t)0x00000001)
#define CTRL_Reset_PG            ((uint32_t)0x00003FFE)
#define CTRL_Set_PER             ((uint32_t)0x00000002)
#define CTRL_Reset_PER           ((uint32_t)0x00003FFD)
#define CTRL_Set_MER             ((uint32_t)0x00000004)
#define CTRL_Reset_MER           ((uint32_t)0x00003FFB)
#define CTRL_Set_OPTPG           ((uint32_t)0x00000010)
#define CTRL_Reset_OPTPG         ((uint32_t)0x00003FEF)
#define CTRL_Set_OPTER           ((uint32_t)0x00000020)
#define CTRL_Reset_OPTER         ((uint32_t)0x00003FDF)
#define CTRL_Set_START           ((uint32_t)0x00000040)
#define CTRL_Set_LOCK            ((uint32_t)0x00000080)
#define CTRL_Reset_SMPSEL        ((uint32_t)0x00003EFF)
#define CTRL_SMPSEL_SMP1         ((uint32_t)0x00000000)
#define CTRL_SMPSEL_SMP2         ((uint32_t)0x00000100)

/* FLASH Mask */
#define RDPRTL1_MSK              ((uint32_t)0x00000002)
#define RDPRTL2_MSK              ((uint32_t)0x80000000)
#define OBR_USER_MSK             ((uint32_t)0x0000001C)
#define WRP0_MSK                 ((uint32_t)0x000000FF)
#define WRP1_MSK                 ((uint32_t)0x0000FF00)
#define WRP2_MSK                 ((uint32_t)0x00FF0000)
#define WRP3_MSK                 ((uint32_t)0xFF000000)

/* FLASH Keys */
#define L1_RDP_Key               ((uint32_t)0xFFFF00A5)
#define RDP_USER_Key             ((uint32_t)0xFFF000A5)
#define L2_RDP_Key               ((uint32_t)0xFFFF33CC)
#define FLASH_KEY1               ((uint32_t)0x45670123)
#define FLASH_KEY2               ((uint32_t)0xCDEF89AB)

/* Delay definition */
#define EraseTimeout             ((uint32_t)0x000B0000)
#define ProgramTimeout           ((uint32_t)0x00002000)

/* 内部辅助函数 */
static void flash_fsm_set_result(flash_fsm_t* fsm, flash_result_t result)
{
    fsm->result = result;
    if (fsm->callback) {
        fsm->callback(result, fsm->user_data);
    }
}

static bool flash_fsm_check_timeout(flash_fsm_t* fsm)
{
    if (fsm->timeout == 0) {
        flash_fsm_set_result(fsm, FLASH_RESULT_TIMEOUT);
        fsm->state = FLASH_STATE_ERROR;
        return true;
    }
    fsm->timeout--;
    return false;
}

/* 初始化Flash状态机 */
void flash_fsm_init(flash_fsm_t* fsm)
{
    assert_param(fsm != NULL);

    fsm->state     = FLASH_STATE_IDLE;
    fsm->result    = FLASH_RESULT_SUCCESS;
    fsm->callback  = NULL;
    fsm->user_data = NULL;
    fsm->timeout   = 0;
}

/* 启动Flash操作 */
bool flash_fsm_start(flash_fsm_t* fsm, const flash_request_t* request,
                     void (*callback)(flash_result_t result, void* user_data),
                     void* user_data)
{
    assert_param(fsm != NULL);
    assert_param(request != NULL);

    if (fsm->state != FLASH_STATE_IDLE) {
        return false; /* 当前有操作进行中 */
    }

    fsm->request   = *request;
    fsm->callback  = callback;
    fsm->user_data = user_data;
    fsm->result    = FLASH_RESULT_PENDING;

    /* 根据操作类型设置超时时间 */
    switch (request->type) {
        case FLASH_OP_ERASE_PAGE:
        case FLASH_OP_ERASE_OPTION:
            fsm->timeout = FLASH_OP_TIMEOUT_ERASE;
            break;
        case FLASH_OP_PROGRAM_WORD:
        case FLASH_OP_PROGRAM_OPTION:
            fsm->timeout = FLASH_OP_TIMEOUT_PROGRAM;
            break;
        default:
            return false;
    }

    fsm->state = FLASH_STATE_INIT;
    return true;
}

/* 处理Flash状态机 */
void flash_fsm_process(flash_fsm_t* fsm)
{
    assert_param(fsm != NULL);

    if (fsm->state == FLASH_STATE_IDLE || fsm->state == FLASH_STATE_DONE ||
        fsm->state == FLASH_STATE_ERROR) {
        return;
    }

    FLASH_STS status;

    switch (fsm->state) {
        case FLASH_STATE_INIT:
            fsm->state = FLASH_STATE_UNLOCK;
            break;

        case FLASH_STATE_UNLOCK:
            FLASH_Unlock();
            if (fsm->request.type == FLASH_OP_ERASE_OPTION ||
                fsm->request.type == FLASH_OP_PROGRAM_OPTION) {
                /* 选项字节操作需要额外的解锁 */
                FLASH->OPTKEY = FLASH_KEY1;
                FLASH->OPTKEY = FLASH_KEY2;
            }
            fsm->state = FLASH_STATE_CLEAR_FLAG;
            break;

        case FLASH_STATE_CLEAR_FLAG:
            FLASH_ClearFlag(FLASH_STS_CLRFLAG);
            fsm->state = FLASH_STATE_WAIT_READY;
            break;

        case FLASH_STATE_WAIT_READY:
            status = FLASH_GetSTS();
            if (status == FLASH_BUSY) {
                if (flash_fsm_check_timeout(fsm)) {
                    return;
                }
            } else if (status == FLASH_COMPL) {
                fsm->state = FLASH_STATE_SET_OP_TYPE;
            } else {
                flash_fsm_set_result(fsm, FLASH_RESULT_ERROR);
                fsm->state = FLASH_STATE_ERROR;
            }
            break;

        case FLASH_STATE_SET_OP_TYPE:
            switch (fsm->request.type) {
                case FLASH_OP_ERASE_PAGE:
                    FLASH->CTRL |= CTRL_Set_PER;
                    fsm->state = FLASH_STATE_SET_ADDRESS;
                    break;
                case FLASH_OP_PROGRAM_WORD:
                    FLASH->CTRL |= CTRL_Set_PG;
                    fsm->state = FLASH_STATE_SET_ADDRESS;
                    break;
                case FLASH_OP_ERASE_OPTION:
                    FLASH->CTRL |= CTRL_Set_OPTER;
                    fsm->state = FLASH_STATE_START_OP;
                    break;
                case FLASH_OP_PROGRAM_OPTION:
                    FLASH->CTRL |= CTRL_Set_OPTPG;
                    fsm->state = FLASH_STATE_SET_ADDRESS;
                    break;
            }
            break;

        case FLASH_STATE_SET_ADDRESS:
            if (fsm->request.type == FLASH_OP_ERASE_PAGE) {
                FLASH->ADD = fsm->request.address;
                fsm->state = FLASH_STATE_START_OP;
            } else if (fsm->request.type == FLASH_OP_PROGRAM_WORD) {
                *(__IO uint32_t*)fsm->request.address = fsm->request.data;
                fsm->state = FLASH_STATE_START_OP;
            } else if (fsm->request.type == FLASH_OP_PROGRAM_OPTION) {
                uint32_t data = ((uint32_t)fsm->request.option_data & 0xFF) |
                                (((uint32_t)fsm->request.option_data & 0xFF00) << 8);
                *(__IO uint32_t*)fsm->request.address = data;
                fsm->state                            = FLASH_STATE_START_OP;
            }
            break;

        case FLASH_STATE_START_OP:
            FLASH->CTRL |= CTRL_Set_START;
            fsm->state = FLASH_STATE_WAIT_COMPLETE;
            break;

        case FLASH_STATE_WAIT_COMPLETE:
            status = FLASH_GetSTS();
            if (status == FLASH_BUSY) {
                if (flash_fsm_check_timeout(fsm)) {
                    return;
                }
            } else if (status == FLASH_COMPL) {
                fsm->state = FLASH_STATE_LOCK;
            } else {
                flash_fsm_set_result(fsm, FLASH_RESULT_ERROR);
                fsm->state = FLASH_STATE_ERROR;
            }
            break;

        case FLASH_STATE_LOCK:
            /* 清除操作位 */
            switch (fsm->request.type) {
                case FLASH_OP_ERASE_PAGE:
                    FLASH->CTRL &= CTRL_Reset_PER;
                    break;
                case FLASH_OP_PROGRAM_WORD:
                    FLASH->CTRL &= CTRL_Reset_PG;
                    break;
                case FLASH_OP_ERASE_OPTION:
                    FLASH->CTRL &= CTRL_Reset_OPTER;
                    break;
                case FLASH_OP_PROGRAM_OPTION:
                    FLASH->CTRL &= CTRL_Reset_OPTPG;
                    break;
            }

            FLASH_Lock();
            flash_fsm_set_result(fsm, FLASH_RESULT_SUCCESS);
            fsm->state = FLASH_STATE_DONE;
            break;

        default:
            break;
    }
}

/* 取消当前操作 */
void flash_fsm_cancel(flash_fsm_t* fsm)
{
    assert_param(fsm != NULL);

    if (fsm->state != FLASH_STATE_IDLE && fsm->state != FLASH_STATE_DONE &&
        fsm->state != FLASH_STATE_ERROR) {
        /* 恢复到安全状态 */
        FLASH_Lock();
        flash_fsm_set_result(fsm, FLASH_RESULT_ERROR);
        fsm->state = FLASH_STATE_ERROR;
    }
}