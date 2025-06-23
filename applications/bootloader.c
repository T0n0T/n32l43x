#include "bootloader.h"
#include "stdio.h"

/**@brief Function for booting an app as if the chip was reset.
 *
 * @param[in]  vector_table_addr  The address of the app's vector table.
 */
static inline void jump_to_application(uint32_t vector_table_addr)
{
    const uint32_t current_isr_num = (__get_IPSR() & IPSR_ISR_Msk);
    const uint32_t new_msp         = *((uint32_t*)(vector_table_addr));                    // The app's Stack Pointer is found as the first word of the vector table.
    const uint32_t reset_handler   = *((uint32_t*)(vector_table_addr + sizeof(uint32_t))); // The app's Reset Handler is found as the second word of the vector table.

    __set_CONTROL(0x00000000);   // Set CONTROL to its reset value 0.
    __set_PRIMASK(0x00000000);   // Set PRIMASK to its reset value 0.
    __set_BASEPRI(0x00000000);   // Set BASEPRI to its reset value 0.
    __set_FAULTMASK(0x00000000); // Set FAULTMASK to its reset value 0.

    assert_param(current_isr_num == 0); // If this is triggered, the CPU is currently in an interrupt.

    __set_MSP(new_msp);
    ((void (*)(void))reset_handler)();
}

void app_run(uint32_t app_addr)
{
    BOOT_LOG_INFO("Jump to application running ...");

    NVIC->ICER[0] = 0xFFFFFFFF;
    NVIC->ICPR[0] = 0xFFFFFFFF;

    jump_to_application(app_addr);

    while (1);
}

bool app_is_valid(uint32_t app_addr)
{
    if ((((uint32_t)(*((__IO uint32_t*)(app_addr + 4))) & 0xff000000) != 0x08000000) ||
        (((*((__IO uint32_t*)app_addr) & 0x2ff00000) != 0x20000000))) {
        BOOT_LOG_ERROR("No legitimate application.");
        return false;
    }
    return true;
}
