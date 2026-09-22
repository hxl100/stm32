# STM32 三色 LED 轮流闪烁工程 (Keil MDK5, 寄存器版)

芯片: **STM32F103C8T6**, 主频 72MHz, 开发环境: Keil MDK 5.24a (MDK-Lite)

## 功能说明

红、绿、蓝 3 只 LED 分别接在 **GPIOA、GPIOB、GPIOC** 三个端口的引脚上,
按 `红 -> 绿 -> 蓝` 的顺序轮流点亮, 每只保持 **1 秒**, 循环往复。

纯寄存器方式编程 (直接操作 RCC / GPIO / SysTick 寄存器), 不调用任何库函数。

## 工程文件

| 文件 | 说明 |
|------|------|
| `led_flow.uvprojx` | Keil 工程文件（双击打开） |
| `main.c` | 主程序（寄存器版, 详细注释） |
| `startup_stm32f10x_md.s` | 启动文件（来自 STM32F1xx_DFP 2.2.0） |
| `system_stm32f10x.c` | 系统时钟初始化（72MHz） |
| `Objects\led_flow.hex` | 编译产物，串口下载用这个文件 |
| `Objects\led_flow.axf` | 编译产物，Keil 调试/ST-LINK 下载用 |

## 硬件连接 (面包板)

| LED | 引脚 | 板子排针位置 |
|-----|------|--------------|
| 红灯 | PA0  | 排针标 PA0 |
| 绿灯 | PB0  | 排针标 PB0 |
| 蓝灯 | PC14 | 排针标 PC14 |

接线方式（共阳 / 低电平点亮）:

```
3.3V ──► LED阳极 ──► LED阴极 ──► 220Ω~1kΩ电阻 ──► PA0 / PB0 / PC14
```

- 每只 LED 串联一只限流电阻 (220Ω ~ 1kΩ)
- 蓝/绿灯导通压降较高, 若亮度偏暗可换 100Ω~220Ω 电阻
- 核心板 GND 与供电电源共地
- PC14 与 PC15 是 32.768kHz 晶振脚, 核心板上未焊该晶振, 可当普通 IO 用
- 如果你的板子 PC13 上自带指示灯, 与本工程无关（本工程用的是 PC14）

## 使用方法

1. 双击 `led_flow.uvprojx` 用 Keil 打开工程
2. 按 **F7** 编译（应 0 Error / 0 Warning）
3. 有 ST-LINK 时：按 **F8（Load）** 直接下载运行

## 修改方法

- **换引脚**: 修改 `main.c` 顶部 `LED_RED_PIN` 等宏定义, 同时改 `LED_Init()`
  里的时钟使能位和 CRL/CRH 配置（Pin0~7 用 CRL, Pin8~15 用 CRH）
- **改间隔**: 修改 `main()` 中 `Delay_ms(1000)` 的数值, 单位毫秒
- **高电平点亮**: 若 LED 阴极接地、阳极经电阻接引脚, 把 `LED_On()` 中
  BSRR/BRR 互换即可（写 BRR 灭、写 BSRR 亮）
- **串口下载** (无 ST-LINK 时): 用 USB-TTL 接 PA9/PA10, BOOT0 接 1 复位后
  用 FlyMcu 烧录 `Objects\led_flow.hex`, 烧完 BOOT0 接回 0

## 常见问题

- **中文注释乱码**: Edit → Configuration → Editor → Encoding 选 UTF-8
- **编译报错找不到头文件**: 工程 Include Path 已配好 (C/C++ 选项卡可查看),
  依赖 `Keil.STM32F1xx_DFP` 和 `ARM.CMSIS` 两个包, 不要卸载
- **代码超 32KB 报 L6047U**: MDK-Lite 限制, 需升级注册 MDK Community 版
