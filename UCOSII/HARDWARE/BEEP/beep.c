#include "beep.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEKս��STM32������
//��������������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/2
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	   

//��ʼ��PB8Ϊ�����.��ʹ������ڵ�ʱ��		    
//��������ʼ��
void BEEP_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /* Enable the GPIO_LED Clock */
    /* ʹ�ܷ�������ӦGPIO��Clockʱ�� */
    RCC_APB2PeriphClockCmd(BEEP_GPIO_CLK, ENABLE);

    /* Configure the GPIO_LED pin */
    /* ��ʼ����������GPIO�ܽţ�����Ϊ������� */
    GPIO_InitStructure.GPIO_Pin = BEEP_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(BEEP_GPIO_PORT, &GPIO_InitStructure);

    BEEPOBB = 1;//��ʼ��ʱ�رշ�����
}

/**-------------------------------------------------------
  * @������ SZ_STM32_BEEPOn
  * @����   ʹ��������ʼ����
  * @����   ��
  * @����ֵ ��
***------------------------------------------------------*/
void BEEP_On(void)
{
    /* ָ���ܽ�����͵�ƽ��ʹ��������ʼ���� */
    //BEEP_GPIO_PORT->BRR = BEEP_PIN;  
    BEEPOBB = 0;
}

/**-------------------------------------------------------
  * @������ SZ_STM32_BEEPOff
  * @����   ʹ������ֹͣ����
  * @����   ��
  * @����ֵ ��
***------------------------------------------------------*/
void BEEP_Off(void)
{
    /* ָ���ܽ�����ߵ�ƽ��ʹ������ֹͣ���� */
    //BEEP_GPIO_PORT->BSRR = BEEP_PIN;   
    BEEPOBB = 1;
}

