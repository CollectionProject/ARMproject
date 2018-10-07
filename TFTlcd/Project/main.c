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

#define MAX_OBJECTS 10

void window_1_callback(UG_MESSAGE *msg)
{
	
}

int main(void)
{ 
		UG_WINDOW window_1;
		UG_BUTTON button_1;
		UG_BUTTON button_2;
		UG_BUTTON button_3;
		UG_TEXTBOX textbox_1;
		UG_OBJECT obj_buff_wnd_1[MAX_OBJECTS];
	
    /* 注意串口2使用Printf时"SZ_STM32F107VC_LIB.c"文件中fputc定义中设备改为SZ_STM32_COM2 */
    SZ_STM32_COMInit(COM2, 115200);

    

    /* TFT-LCD初始化 */	
    SZ_STM32_LCDInit();
	
	  UG_Init(&gui,draw_point,320,240);
	
		/* 初始化系统定时器SysTick,每秒中断1000次 */
    
		// . . .
		/* Create the window */
		UG_WindowCreate ( &window_1 , obj_buff_wnd_1 , MAX_OBJECTS, window_1_callback ) ;
		/* Modify the window t i t l e */
		UG_WindowSetTitleText ( &window_1 , "uGUI Demo Window" ) ;
		UG_WindowSetTitleTextFont ( &window_1 , &FONT_12X20 ) ;
		/* Create some buttons */
		UG_ButtonCreate ( &window_1 , &button_1 , BTN_ID_0 , 10 , 10 , 110 , 60 ) ;
		UG_ButtonCreate ( &window_1 , &button_2 , BTN_ID_1 , 10 , 80 , 110 , 130 ) ;
		UG_ButtonCreate ( &window_1 , &button_3 , BTN_ID_2 , 10 , 150 , 110 , 200 ) ;
		/* Label the buttons */
		UG_ButtonSetFont ( &window_1 , BTN_ID_0 , &FONT_12X20 ) ;
		UG_ButtonSetText ( &window_1 , BTN_ID_0 , "Button nnA" ) ;
		UG_ButtonSetFont ( &window_1 , BTN_ID_1 , &FONT_12X20 ) ;
		UG_ButtonSetText ( &window_1 , BTN_ID_1 , "Button nnB" ) ;
		UG_ButtonSetFont ( &window_1 , BTN_ID_2 , &FONT_12X20 ) ;
		UG_ButtonSetText ( &window_1 , BTN_ID_2 , "Button nnC" ) ;
		/* Create a Textbox */
		UG_TextboxCreate ( &window_1 , &textbox_1 , TXB_ID_0 , 120 , 10 , 310 , 200 ) ;
		UG_TextboxSetFont ( &window_1 , TXB_ID_0 , &FONT_12X16 ) ;
		UG_TextboxSetText ( &window_1 , TXB_ID_0 ,"Hello world!!!" ) ;
		UG_TextboxSetForeColor ( &window_1 , TXB_ID_0 , C_BLACK ) ;
		UG_TextboxSetAlignment ( &window_1 , TXB_ID_0 , ALIGN_CENTER ) ;
		/* Fin ally , show the window */
		UG_WindowShow( &window_1 ) ;
		// . . .
		//UG_Update();
		SZ_STM32_SysTickInit(1000);
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

    if((TimeIncrease%1000) == 0)
    {
        UG_Update();
    }
    TimeIncrease++;
	
}

							


/******************* (C) COPYRIGHT 2013 www.armjishu.com *****END OF FILE****/
