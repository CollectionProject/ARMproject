#include <stdio.h>
#include <stm32f4xx.h>
#include "common.h"

/* ��ʱn���� (����ȷ) */
// ʵ���ӳٵ�ʱ��t: nms<t<=nms+1
void delay(uint16_t nms)
{
  uint32_t diff;
  uint32_t start = sys_now();
  do
  {
    diff = sys_now() - start;
  } while (diff <= nms);
}

/* ��ʾ�������� */
void dump_data(const void *data, uint32_t len)
{
  const uint8_t *p = data;
  while (len--)
    printf("%02X", *p++);
  printf("\n");
}

/* �ж�һ���ڴ��Ƿ�ȫΪ0 */
uint8_t isempty(const void *data, uint32_t len)
{
  const uint8_t *p = data;
  while (len--)
  {
    if (*p != 0)
      return 0;
    p++;
  }
  return 1;
}

/* ��ȡϵͳʱ������� (lwipЭ��ջҪ��ʵ�ֵĺ���) */
// �ú������뱣֤: ���Ƕ�ʱ�����, ������ȡ��ʱ���������Ȼ�ȡ��ʱ��
uint32_t sys_now(void)
{
  return TIM_GetCounter(TIM5);
}

/* ��ʼ��ϵͳʱ���������� */
void sys_now_init(void)
{
  TIM_TimeBaseInitTypeDef tim;
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4 | RCC_APB1Periph_TIM5, ENABLE);
  
  // TIM4���ڽ�84MHz��ƵΪ1MHz
  TIM_TimeBaseStructInit(&tim);
  tim.TIM_Period = 83; // 84MHz/(83+1)=1MHz -> 1us
  tim.TIM_Prescaler = 0;
  TIM_TimeBaseInit(TIM4, &tim);
  TIM_SelectOutputTrigger(TIM4, TIM_TRGOSource_Update);
  
  // TIM5��TIM4�ṩʱ��, ����ƵΪ1kHz, �γɺ��������
  tim.TIM_Period = 0xffffffff; // 32λ������
  tim.TIM_Prescaler = 999; // 1us*(999+1)=1ms
  TIM_TimeBaseInit(TIM5, &tim);
  TIM_ITRxExternalClockConfig(TIM5, TIM_TS_ITR2); // TIM4_TRGO
  TIM_Cmd(TIM5, ENABLE);
  
  TIM_Cmd(TIM4, ENABLE); // �������붨ʱ��, ��ʽ��ʼ��ʱ
}
