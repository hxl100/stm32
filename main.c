/**
 *******************************************************************************
 * @file    main.c
 * @brief   三色 LED 轮流闪烁实验（纯寄存器编程, 不依赖任何库函数）
 * ----------------------------------------------------------------------------
 *  硬件:  STM32F103C8T6 最小系统核心板 + 面包板 + 红/绿/蓝 LED 各 1 只
 *  接线:  红灯 -> PA0,  绿灯 -> PB0,  蓝灯 -> PC14
 *         （共阳接法: LED 阳极接 3.3V, 阴极经限流电阻接引脚, 低电平点亮）
 *  效果:  红亮 1s -> 绿亮 1s -> 蓝亮 1s -> 循环往复, 任意时刻只有 1 只亮
 *  环境:  Keil MDK 5.24a (ARMCC V5.06), 系统主频 72MHz
 *******************************************************************************
 */

#include "stm32f10x.h"

/* -------------------------------------------------------------------------- */
/* 引脚宏定义: 换引脚时只需要改这里                                          */
/* -------------------------------------------------------------------------- */
#define LED_RED_PIN     0       /* 红灯接 PA0 */
#define LED_GREEN_PIN   0       /* 绿灯接 PB0 */
#define LED_BLUE_PIN    14      /* 蓝灯接 PC14 */

/* LED 编号, 便于 main 里阅读 */
#define LED_RED         0
#define LED_GREEN       1
#define LED_BLUE        2

/**
 * @brief  毫秒级延时（基于 SysTick 系统滴答定时器, 主频 72MHz）
 * @param  ms: 要延时的毫秒数
 * @note   SysTick 是 Cortex-M3 内核自带的 24 位递减计数器
 *         72MHz 下计 72000 个时钟 = 1ms, 所以重装载值写 72000-1
 */
void Delay_ms(uint32_t ms)
{
    SysTick->LOAD = 72000 - 1;          /* 重装载值: 数 72000 次 = 1ms */
    SysTick->VAL  = 0;                  /* 清空当前计数值 */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |  /* 时钟源 = 内核时钟 72MHz */
                    SysTick_CTRL_ENABLE_Msk;      /* 使能计数器, 开始递减 */

    while (ms--)
    {
        /* COUNTFLAG 位在计数减到 0 时自动置 1,
         * 读它相当于轮询"1ms 到了没有" */
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0)
            ;
    }

    SysTick->CTRL = 0;                  /* 延时结束, 关闭 SysTick */
}

/**
 * @brief  LED 引脚初始化: PA0 / PB0 / PC14 配置为通用推挽输出, 50MHz
 */
void LED_Init(void)
{
    /* ---- 第 1 步: 使能 3 个 GPIO 端口的时钟 ----
     * STM32 上电后所有外设时钟默认关闭, 不使能时钟寄存器写了也不生效。
     * GPIOA/B/C 都挂在 APB2 总线上, 对应 APB2ENR 寄存器的 IOPAEN/IOPBEN/IOPCEN 位 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN |   /* 使能 GPIOA 时钟 */
                    RCC_APB2ENR_IOPBEN |   /* 使能 GPIOB 时钟 */
                    RCC_APB2ENR_IOPCEN;    /* 使能 GPIOC 时钟 */

    /* ---- 第 2 步: 配置引脚模式 ----
     * 每个引脚在 CRL/CRH 寄存器中占 4 位:
     *   高 2 位 CNF  = 输出类型   低 2 位 MODE = 速度
     *   CRL 管 Pin0~Pin7,  CRH 管 Pin8~Pin15
     * 写入 0011b:  CNF=00 通用推挽输出 + MODE=11 输出速度 50MHz */

    /* PA0 在 CRL 的 bit[3:0]: 先清 0, 再写入 0011 */
    GPIOA->CRL &= ~0x0F;
    GPIOA->CRL |=  0x03;

    /* PB0 同样在 CRL 的 bit[3:0] */
    GPIOB->CRL &= ~0x0F;
    GPIOB->CRL |=  0x03;

    /* PC14 在 CRH 的 bit[27:24]  (计算公式: (14-8)*4 = 24) */
    GPIOC->CRH &= ~(0x0F << 24);
    GPIOC->CRH |=  (0x03 << 24);

    /* ---- 第 3 步: 初始状态 ----
     * 共阳接法低电平点亮, 所以上电先输出高电平 = 全部熄灭 */
    GPIOA->ODR |= (1U << LED_RED_PIN);
    GPIOB->ODR |= (1U << LED_GREEN_PIN);
    GPIOC->ODR |= (1U << LED_BLUE_PIN);
}

/**
 * @brief  只点亮指定的一只 LED, 其余全部熄灭
 * @param  led: LED_RED / LED_GREEN / LED_BLUE
 * @note   用 BSRR/BRR 位操作寄存器控制引脚:
 *         - 写 BSRR 某位为 1 -> 对应引脚输出高电平（共阳接法 = 熄灭）
 *         - 写 BRR  某位为 1 -> 对应引脚输出低电平（共阳接法 = 点亮）
 *         位操作只改指定的引脚, 不影响同一端口的其他引脚,
 *         比直接读写 ODR 更安全（不会发生读-改-写被打断的问题）
 */
void LED_On(uint8_t led)
{
    /* 先把 3 只 LED 全部熄灭 */
    GPIOA->BSRR = (1U << LED_RED_PIN);      /* PA0 输出高 -> 红灯灭 */
    GPIOB->BSRR = (1U << LED_GREEN_PIN);    /* PB0 输出高 -> 绿灯灭 */
    GPIOC->BSRR = (1U << LED_BLUE_PIN);     /* PC14 输出高 -> 蓝灯灭 */

    /* 再点亮指定的一只 */
    switch (led)
    {
        case LED_RED:                       /* PA0 输出低 -> 红灯亮 */
            GPIOA->BRR = (1U << LED_RED_PIN);
            break;

        case LED_GREEN:                     /* PB0 输出低 -> 绿灯亮 */
            GPIOB->BRR = (1U << LED_GREEN_PIN);
            break;

        case LED_BLUE:                      /* PC14 输出低 -> 蓝灯亮 */
            GPIOC->BRR = (1U << LED_BLUE_PIN);
            break;

        default:                            /* 非法编号: 全部熄灭 */
            break;
    }
}

/**
 * @brief  主函数: 红 -> 绿 -> 蓝 轮流点亮, 每只保持 1 秒
 */
int main(void)
{
    LED_Init();                 /* 初始化 3 个端口的 LED 引脚 */

    while (1)
    {
        LED_On(LED_RED);        /* 只亮红灯 */
        Delay_ms(1000);         /* 保持 1 秒 */

        LED_On(LED_GREEN);      /* 只亮绿灯 */
        Delay_ms(1000);         /* 保持 1 秒 */

        LED_On(LED_BLUE);       /* 只亮蓝灯 */
        Delay_ms(1000);         /* 保持 1 秒 */
    }
}
