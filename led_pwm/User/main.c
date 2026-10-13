/* 文件名: main.c
 * 功能: 实验任务4——定时器和PWM实验
 *   TASK=1: 定时器计数方式——TIM2每1s产生一次更新中断,翻转PA0,LED以2s为周期亮灭
 *   TASK=2: PWM模式——TIM2_CH1(PA0)输出1kHz PWM,呼吸灯渐亮渐灭,呼吸周期约2s
 * 工程环境: 江协科技Keil5教学工程结构(Start/Library/System/User),标准外设库
 * 切换方法: 修改下面 TASK 的值(1或2),重新编译下载
 */
#include "stm32f10x.h"                  // Device header
#include "Delay.h"

#define TASK 2		//1=任务1定时闪灯  2=任务2PWM呼吸灯

#if TASK == 1	/*==================== 任务1: 定时器计数方式控制LED闪灯 ====================*/

void Timer2_Init(void)
{
	/*开启时钟: TIM2挂在APB1总线上,GPIOA挂在APB2总线上,使用前都要开时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);	//开启TIM2时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA时钟

	/*GPIO初始化: PA0推挽输出接LED(任务1定时器只负责计时,引脚还是普通输出)*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;		//推挽输出模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;			//LED接PA0
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//输出速度50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*时基单元初始化(定时器最核心的三个参数):
	  定时器时钟 = 72MHz(APB1=36MHz,定时器自动×2)
	  PSC预分频: 72MHz / 7200 = 10kHz(计数器每1/10000秒加1)
	  ARR自动重装: 数满10000个数产生一次更新事件 → 10kHz / 10000 = 1Hz,即每1秒一次*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;	//时钟分频(滤波采样用),不分频
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//向上计数模式
	TIM_TimeBaseInitStructure.TIM_Period = 10000 - 1;		//自动重装值ARR=9999,数10000个数
	TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;		//预分频值PSC=7199,分7200份
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;		//重复计数器(高级定时器专用),普通定时器填0
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

	/*更新中断配置*/
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);			//先清一次更新标志,防止启动瞬间误进中断
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);		//使能更新中断:每1s触发一次

	/*NVIC配置: 优先级分组2(2位抢占+2位子优先级),TIM2中断通道使能*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;			//中断通道:TIM2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//使能该通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//子优先级1
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM2, ENABLE);	//启动TIM2,开始计数
}

int main(void)
{
	Timer2_Init();
	while (1)
	{
		//主循环什么都不用做——LED的翻转完全由TIM2定时中断完成
		//(每1秒进一次中断翻转一次PA0 → 亮1s灭1s,周期2s)
	}
}

#elif TASK == 2	/*==================== 任务2: PWM模式呼吸灯 ====================*/

void PWM_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);	//开启TIM2时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA时钟

	/*GPIO初始化: PA0接TIM2的CH1通道,必须配置为"复用推挽输出"!
	  这是PWM实验与普通点灯的唯一区别——引脚的控制权交给定时器硬件*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//复用推挽输出(AF=Alternate Function)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;		//TIM2_CH1对应PA0(查引脚定义表)
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//输出速度50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*时基单元初始化(决定PWM频率):
	  72MHz / 720 / 100 = 1kHz,即PWM波的周期为1ms*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;	//不分频
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;		//ARR=99:计100个数一个PWM周期
	TIM_TimeBaseInitStructure.TIM_Prescaler = 720 - 1;		//PSC=719:72MHz/720=100kHz
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

	/*输出比较单元初始化(CH1): 配置为PWM模式1,占空比初值0(灯全灭)*/
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;		//PWM模式1:计数值CNT<CCR时输出有效电平
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;	//有效电平为高
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;	//使能CH1输出到引脚
	TIM_OCInitStructure.TIM_Pulse = 0;				//CCR初值0 → 占空比0%
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);

	TIM_Cmd(TIM2, ENABLE);	//启动TIM2,PWM立即开始输出
}

int main(void)
{
	uint8_t i;
	PWM_Init();
	while (1)
	{
		/*呼吸灯: CCR从0走到99(渐亮)再走回0(渐灭)
		  占空比 = CCR / (ARR+1),改CCR就是改占空比
		  每步停留10ms → 199步×10ms ≈ 2s 一个完整呼吸周期*/
		for (i = 0; i <= 99; i++)	//渐亮: 占空比0%→99%
		{
			TIM_SetCompare1(TIM2, i);	//修改CH1的比较值CCR=修改占空比
			Delay_ms(10);
		}
		for (i = 99; i > 0; i--)	//渐灭: 占空比99%→1%
		{
			TIM_SetCompare1(TIM2, i);
			Delay_ms(10);
		}
	}
}

#endif
