#include <stdio.h>
#include <stm32f4xx.h>
#include "common.h"

/* 延时n毫秒 (不精确) */
// 实际延迟的时间t: nms<t<=nms+1
void delay(uint16_t nms)
{
  uint32_t diff;
  uint32_t start = sys_now();
  do
  {
    diff = sys_now() - start;
  } while (diff <= nms);
}

/* 显示数据内容 */
void dump_data(const void *data, uint32_t len)
{
  const uint8_t *p = data;
  while (len--)
    printf("%02X", *p++);
  printf("\n");
}

/* 判断一段内存是否全为0 */
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

/* 获取系统时间毫秒数 (lwip协议栈要求实现的函数) */
// 该函数必须保证: 除非定时器溢出, 否则后获取的时间必须大于先获取的时间
uint32_t sys_now(void)
{
  return TIM_GetCounter(TIM5);
}

/* 初始化系统时间毫秒计数器 */
void sys_now_init(void)
{
  TIM_TimeBaseInitTypeDef tim;
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4 | RCC_APB1Periph_TIM5, ENABLE);
  
  // TIM4用于将84MHz分频为1MHz
  TIM_TimeBaseStructInit(&tim);
  tim.TIM_Period = 83; // 84MHz/(83+1)=1MHz -> 1us
  tim.TIM_Prescaler = 0;
  TIM_TimeBaseInit(TIM4, &tim);
  TIM_SelectOutputTrigger(TIM4, TIM_TRGOSource_Update);
  
  // TIM5由TIM4提供时钟, 并分频为1kHz, 形成毫秒计数器
  tim.TIM_Period = 0xffffffff; // 32位计数器
  tim.TIM_Prescaler = 999; // 1us*(999+1)=1ms
  TIM_TimeBaseInit(TIM5, &tim);
  TIM_ITRxExternalClockConfig(TIM5, TIM_TS_ITR2); // TIM4_TRGO
  TIM_Cmd(TIM5, ENABLE);
  
  TIM_Cmd(TIM4, ENABLE); // 启动毫秒定时器, 正式开始计时
}
