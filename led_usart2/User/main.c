/* 文件名: main.c
 * 功能: 实验任务5之任务2——USART串口通信
 *   单片机连续发送"hello windows!"，上位机发'#'暂停发送，发'*'恢复发送
 * 串口参数: 115200波特率, 8位数据位, 1位停止位, 无校验位(8N1)
 * 使用USART2: PA2=TX发送  PA3=RX接收
 * 硬件接线: PA2→USB转TTL模块RX  PA3→模块TX  GND→GND(共地)
 * 工程环境: 江协科技Keil5教学工程结构(Start/Library/System/User), 标准外设库
 */
#include "stm32f10x.h"                  // Device header
#include "Delay.h"

/*串口发送一个字符串:
  逐字节写入"发送数据寄存器DR",必须等上一字节发完(TXE标志=1)才能写下一字节,
  否则上一字节还没发出去就被新数据覆盖——这就是"不加延时会丢数据"的底层原因*/
void USART_SendString(char *str)
{
	uint8_t i;
	for (i = 0; str[i] != '\0'; i++)		//字符串以'\0'结尾,循环到结尾停止
	{
		USART_SendData(USART2, str[i]);					//把一字节写入发送数据寄存器DR
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);	//等待发送完成(TXE=1)
	}
}

void USART2_Init(void)
{
	/*开启时钟: USART2挂在APB1总线上,GPIOA挂在APB2总线上,使用前都要开时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);	//开启USART2时钟(APB1)
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA时钟(APB2)

	/*GPIO初始化:
	  PA2=USART2_TX发送引脚→复用推挽输出(引脚控制权交给串口硬件)
	  PA3=USART2_RX接收引脚→浮空输入*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//复用推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;		//PA2发送
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//输出速度50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//浮空输入
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;		//PA3接收
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*USART初始化: 任务书要求的参数——115200波特率、8位数据、1位停止位、无校验*/
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;			//波特率115200
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//8位数据位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;		//1位停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;		//无校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//无硬件流控
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;	//发送和接收都使能
	USART_Init(USART2, &USART_InitStructure);

	USART_Cmd(USART2, ENABLE);	//启动USART2
}

int main(void)
{
	uint8_t rx_data;		//存放接收到的字节
	uint8_t send_enable = 1;	//发送使能标志: 1=发送  0=暂停
	USART2_Init();
	USART_SendString("TASK2-READY!\r\n");	//开机标志: 串口助手看到这句=程序已运行
	while (1)
	{
		if (send_enable == 1)			//允许发送时才发送
		{
			USART_SendString("hello windows!\r\n");
			Delay_ms(500);			//延时500ms再发下一条
		}
		if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET)	//RXNE=1表示收到数据
		{
			rx_data = USART_ReceiveData(USART2);	//读出数据(读的同时自动清除RXNE标志)
			if (rx_data == '#')			//收到'#' → 暂停发送
			{
				send_enable = 0;
				USART_SendString("PAUSED!\r\n");	//回显: 告诉电脑已暂停(验证用,可删)
			}
			else if (rx_data == '*')		//收到'*' → 恢复发送
			{
				send_enable = 1;
				USART_SendString("RESUMED!\r\n");	//回显: 告诉电脑已恢复(验证用,可删)
			}
		}
	}
}
