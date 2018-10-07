/******************** (C) COPYRIGHT 2013 www.armjishu.com  ********************
 * 文件名  ：main.c
 * 描述    ：实现STM32F107VC神舟IV号开发板的TFT彩屏显示一个英文实验
 * 实验平台：STM32神舟开发板
 * 标准库  ：STM32F10x_StdPeriph_Driver V3.5.0
 * 作者    ：www.armjishu.com 
*******************************************************************************/
#include "SZ_STM32F107VC_LIB.h"
#include "SZ_STM32F107VC_LCD.h"
#include "ugui.h"
UG_GUI gui;

void draw_point(short x,short y,UG_COLOR color)
{
	LCD_SetPoint(y,x,RGB_888toRGB_565(color));
}

int main(void)
{ 
    /* 注意串口2使用Printf时"SZ_STM32F107VC_LIB.c"文件中fputc定义中设备改为SZ_STM32_COM2 */
    SZ_STM32_COMInit(COM2, 115200);

    /* 初始化系统定时器SysTick,每秒中断1000次 */
    SZ_STM32_SysTickInit(1000);

    /* TFT-LCD初始化 */	
    SZ_STM32_LCDInit();
	
	  UG_Init(&gui,draw_point,320,240);
		UG_FontSelect(&FONT_6X8);
		UG_FillScreen(C_YELLOW);
		UG_SetForecolor(C_WHITE);
		UG_SetBackcolor(C_BLUE);
		UG_PutString(0,0,"hello world!!!");
}


/**-------------------------------------------------------
  * @函数名 SysTick_Handler_User
  * @功能   系统节拍定时器服务请求用户处理函数
  * @参数   无
  * @返回值 无
***------------------------------------------------------*/
void SysTick_Handler_User(void)
{
    static uint32_t TimeIncrease = 0;

    if((TimeIncrease%100) == 0)
    {
        if((TimeIncrease%2000) == 0) //每2秒亮100毫秒
        {
            LED4OBB = 0;
        }
        else
        {
            LED4OBB = 1;
        }
    }
    TimeIncrease++;
	
}

							


/******************* (C) COPYRIGHT 2013 www.armjishu.com *****END OF FILE****/
