# 项目名称

该项目是一个嵌入式系统 `RTOS` 项目, 使用 `meson` 构建系统。

## 目录结构

- `docs/` - 项目的文档，包括设计文档和API文档。
- `drivers/` - 硬件驱动层代码。
- `rtos/` - 实时操作系统相关代码。
- `middleware/` - 中间件组件。
- `applications/` - 应用层代码。
- `utilities/` - 实用工具和库。
- `tools/` - 开发工具和脚本。

## 使用
```sh
meson setup build --cross-file tools/toolchains/arm-none-eabi-gcc.txt # 配置项目
meson compile -C build # 编译默认目标
meson compile -C build flash #执行烧录动作
```

## 许可证

该项目遵循 [许可证名称]。