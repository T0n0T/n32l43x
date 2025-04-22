# 固件特定编译选项
firmware_c_args = [
  '-DN32L40X',
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
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_adc.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_can.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_comp.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_crc.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_dac.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_dbg.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_dma.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_exti.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_flash.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_gpio.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_i2c.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_iwdg.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_lcd.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_lptim.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_lpuart.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_opamp.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_pwr.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_rcc.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_rtc.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_spi.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_tim.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_usart.c',
  TARGET_PLATFORM + '_std_periph_driver/src/' + TARGET_PLATFORM + '_wwdg.c',
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
  TARGET_PLATFORM + '_periph_lib/lib/RCC_MSI_Trimming.lib',
]

if get_option('N32_ALGO_LIB')
  prebuilt_libs += TARGET_PLATFORM + '_algo_lib/lib/' + TARGET_PLATFORM + '_aes.lib'
  prebuilt_libs += TARGET_PLATFORM + '_algo_lib/lib/' + TARGET_PLATFORM + '_algo_common.lib'
  prebuilt_libs += TARGET_PLATFORM + '_algo_lib/lib/' + TARGET_PLATFORM + '_des.lib'
  prebuilt_libs += TARGET_PLATFORM + '_algo_lib/lib/' + TARGET_PLATFORM + '_hash.lib'
  prebuilt_libs += TARGET_PLATFORM + '_algo_lib/lib/' + TARGET_PLATFORM + '_rng.lib'
endif

# 创建预编译库的依赖
firmware_deps = []
foreach lib : prebuilt_libs
    firmware_deps += declare_dependency(
        link_args: ['-L' + meson.current_source_dir() / TARGET_PLATFORM + '_periph_lib/lib',
                   '-l:' + lib.split('/')[-1]],
        include_directories: firmware_incdirs
    )
endforeach

# 导出变量供根目录使用
meson.override_dependency('firmware', firmware_deps)

# 编译为静态库(包含源码和预编译库)
firmware_lib = static_library(
  'firmware',
  sources: firmware_sources,
  objects: prebuilt_libs,
  include_directories: firmware_incdirs,
  c_args: c_args + firmware_c_args,
  pic: false,
)