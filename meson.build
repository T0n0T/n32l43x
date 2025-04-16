project('n32l40x_bin', 'c',
  version: '1.0.0',
  default_options: [
    'buildtype=debugoptimized',
    'c_std=gnu11'
  ],
  meson_version: '>=0.56.0')

# 设置交叉编译工具链配置文件
cross_file = 'tools/arm-none-eabi-gcc.txt'

# 全局编译参数
c_args = [
  '-mcpu=cortex-m4',
  '-mthumb',
  '-mfpu=fpv4-sp-d16',
  '-mfloat-abi=hard',
  '-Wall',
  '-Os',
  '-ffunction-sections',
  '-fdata-sections',
  '-specs=picolibc.specs',
  '-D__FPU_PRESENT',
]

# 全局链接参数
link_args = [
  '-mcpu=cortex-m4',
  '-mthumb',
  '-mfpu=fpv4-sp-d16',
  '-mfloat-abi=hard',
  '-Wl,--gc-sections',
  '-specs=picolibc.specs',
  '-Wl,-eentry',
  '-Xlinker',
  '-Map=' + meson.current_build_dir() + '/n32l40x.map',
  '-T', meson.current_source_dir() + '/n32l40x.ld',
]

# 包含子目录构建脚本
subdir('firmware')
subdir('rtos')
subdir('applications')
subdir('drivers')

# 创建最终可执行文件
executable('n32l40x',
  sources: [ 
    applications_sources,
    drivers_sources,
  ],
  include_directories: [
    applications_incdirs,
    drivers_incdirs,
    firmware_incdirs, 
    rtos_incdirs,
  ],
  link_with: [
    firmware_lib,
    rtos_lib,
  ],
  c_args: c_args,
  link_args: link_args,
  name_suffix: 'elf'
)