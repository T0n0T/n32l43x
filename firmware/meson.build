# 固件特定编译选项
firmware_c_args = [
  '-DN32L43X',
  '-DUSE_STDPERIPH_DRIVER',
]

# 获取选项值
TARGET_PLATFORM = get_option('TARGET_PLATFORM')

# 包含目录
firmware_incdirs = include_directories(
  'CMSIS/core',
  'CMSIS/device',
  TARGET_PLATFORM + '_std_periph_driver/inc',
)

if get_option('N32_ALGO_LIB')
  firmware_incdirs += include_directories(TARGET_PLATFORM + '_algo_lib/inc')
endif

if get_option('N32_USBFS_DRV')
  firmware_incdirs += include_directories(TARGET_PLATFORM + '_usbfs_driver/inc')
endif

# 源文件
firmware_sources = files(
  'CMSIS/device/system_' + TARGET_PLATFORM + '.c',
  TARGET_PLATFORM + '_std_periph_driver/src/misc.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_adc.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_can.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_comp.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_crc.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_dac.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_dbg.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_dma.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_exti.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_flash.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_gpio.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_i2c.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_iwdg.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_lcd.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_lprcnt.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_lptim.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_lpuart.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_opamp.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_pwr.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_rcc.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_rtc.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_spi.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_tim.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_usart.c',
  TARGET_PLATFORM + '_std_periph_driver/src/n32l43x_wwdg.c',
  'CMSIS/device/startup/startup_' + TARGET_PLATFORM + '_gcc.s',
)

if get_option('N32_USBFS_DRV')
  firmware_sources += files(
    TARGET_PLATFORM + '_usbfs_driver/src/' + 'usb_core.c',
    TARGET_PLATFORM + '_usbfs_driver/src/' + 'usb_init.c',
    TARGET_PLATFORM + '_usbfs_driver/src/' + 'usb_int.c',
    TARGET_PLATFORM + '_usbfs_driver/src/' + 'usb_mem.c',
    TARGET_PLATFORM + '_usbfs_driver/src/' + 'usb_regs.c',
    TARGET_PLATFORM + '_usbfs_driver/src/' + 'usb_sil.c',
  )
endif

# 链接预编译的静态库
prebuilt_libs = [
  'n32l43x_periph_lib/lib/RCC_MSI_Trimming.lib',
]

if get_option('N32_ALGO_LIB')
  prebuilt_libs += 'n32l43x_algo_lib/lib/n32l43x_aes.lib'
  prebuilt_libs += 'n32l43x_algo_lib/lib/n32l43x_algo_common.lib'
  prebuilt_libs += 'n32l43x_algo_lib/lib/n32l43x_des.lib'
  prebuilt_libs += 'n32l43x_algo_lib/lib/n32l43x_hash.lib'
  prebuilt_libs += 'n32l43x_algo_lib/lib/n32l43x_rng.lib'
endif

# 编译为静态库(包含源码和预编译库)
firmware_lib = static_library(
  'firmware',
  sources: firmware_sources,
  objects: prebuilt_libs,
  include_directories: firmware_incdirs,
  c_args: c_args + firmware_c_args,
  pic: false,
)