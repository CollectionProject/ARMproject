/******************** (C) COPYRIGHT 2013 www.armjishu.com  ********************
 * �ļ���  ��main.c
 * ����    ��ʵ��STM32F107VC����IV�ſ������TFT������ʾһ��Ӣ��ʵ��
 * ʵ��ƽ̨��STM32���ۿ�����
 * ��׼��  ��STM32F10x_StdPeriph_Driver V3.5.0
 * ����    ��www.armjishu.com 
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
    /* ע�⴮��2ʹ��Printfʱ"SZ_STM32F107VC_LIB.c"�ļ���fputc�������豸��ΪSZ_STM32_COM2 */
    SZ_STM32_COMInit(COM2, 115200);

    /* ��ʼ��ϵͳ��ʱ��SysTick,ÿ���ж�1000�� */
    SZ_STM32_SysTickInit(1000);

    /* TFT-LCD��ʼ�� */	
    SZ_STM32_LCDInit();
	
	  UG_Init(&gui,draw_point,320,240);
		UG_FontSelect(&FONT_6X8);
		UG_FillScreen(C_YELLOW);
		UG_SetForecolor(C_WHITE);
		UG_SetBackcolor(C_BLUE);
		UG_PutString(0,0,"hello world!!!");
}


/**-------------------------------------------------------
  * @������ SysTick_Handler_User
  * @����   ϵͳ���Ķ�ʱ�����������û�������
  * @����   ��
  * @����ֵ ��
***------------------------------------------------------*/
void SysTick_Handler_User(void)
{
    static uint32_t TimeIncrease = 0;

    if((TimeIncrease%100) == 0)
    {
        if((TimeIncrease%2000) == 0) //ÿ2����100����
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
