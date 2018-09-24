#include "beep.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//蜂鸣器驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/2
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	   

//初始化PB8为输出口.并使能这个口的时钟		    
//蜂鸣器初始化
void BEEP_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /* Enable the GPIO_LED Clock */
    /* 使能蜂鸣器对应GPIO的Clock时钟 */
    RCC_APB2PeriphClockCmd(BEEP_GPIO_CLK, ENABLE);

    /* Configure the GPIO_LED pin */
    /* 初始化蜂鸣器的GPIO管脚，配置为推挽输出 */
    GPIO_InitStructure.GPIO_Pin = BEEP_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(BEEP_GPIO_PORT, &GPIO_InitStructure);

    BEEPOBB = 1;//初始化时关闭蜂鸣器
}

/**-------------------------------------------------------
  * @函数名 SZ_STM32_BEEPOn
  * @功能   使蜂鸣器开始鸣响
  * @参数   无
  * @返回值 无
***------------------------------------------------------*/
void BEEP_On(void)
{
    /* 指定管脚输出低电平，使蜂鸣器开始鸣响 */
    //BEEP_GPIO_PORT->BRR = BEEP_PIN;  
    BEEPOBB = 0;
}

/**-------------------------------------------------------
  * @函数名 SZ_STM32_BEEPOff
  * @功能   使蜂鸣器停止鸣响
  * @参数   无
  * @返回值 无
***------------------------------------------------------*/
void BEEP_Off(void)
{
    /* 指定管脚输出高电平，使蜂鸣器停止鸣响 */
    //BEEP_GPIO_PORT->BSRR = BEEP_PIN;   
    BEEPOBB = 1;
}

