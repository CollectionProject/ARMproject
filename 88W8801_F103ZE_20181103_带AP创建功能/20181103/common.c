#include <stdio.h>
#include <stm32f10x.h>
#include <time.h>
#include "common.h"

#define RTC_USELSI // 因为板上没有LSE晶振, 所以RTC时钟选LSI

#ifdef RTC_USELSI
#define RTC_PRESCALER 40000 // LSI频率
#else
#define RTC_PRESCALER 32768 // LSE频率
#endif

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

/* 获取RTC分频计数器的值 */
uint32_t rtc_divider(void)
{
  uint32_t div[2];
  do
  {
    div[0] = RTC_GetDivider();
    div[1] = RTC_GetDivider();
  } while ((div[0] >> 16) != (div[1] >> 16));
  
  // RTC_GetDivider函数先读取DIVH再读取DIVL
  // 如果更新发生在第一次读取DIVH或DIVL后, 则div[0]和div[1]的高16位不相等, 两个值都会被丢弃
  // 如果更新发生在第二次读取DIVH后, 则div[0]和div[1]的高16位相等, 且div[0]值的高、低16位匹配，div[1]值的高、低16位不匹配
  return div[0]; // 所以应该取第一次读取的值
}

/* 初始化RTC外设 */
// 如果sys_now函数不是用RTC实现的, 则可以删掉这个函数
void rtc_init(void)
{
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  PWR_BackupAccessCmd(ENABLE); // 允许写后备寄存器(如RCC->BDCR)
  
  // 检查宏定义RTC_USELSI是否正确
  if (RCC->BDCR & RCC_BDCR_RTCEN)
  {
#ifdef RTC_USELSI
    if ((RCC->BDCR & RCC_BDCR_RTCSEL) != RCC_RTCCLKSource_LSI)
#else
    if ((RCC->BDCR & RCC_BDCR_RTCSEL) != RCC_RTCCLKSource_LSE)
#endif
    {
      printf("RTC is already running and the clock source doesn't match RTC_USELSI!\n");
      printf("Reset RTC!\n");
      RCC_BackupResetCmd(ENABLE);
      RCC_BackupResetCmd(DISABLE);
    }
  }
  
#ifdef RTC_USELSI
  RCC_LSICmd(ENABLE);
  while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET); // 等待LSI启动
#else
  if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
  {
    RCC_LSEConfig(RCC_LSE_ON);
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET); // 等待LSE启动
  }
#endif
  
  if ((RCC->BDCR & RCC_BDCR_RTCEN) == 0) // 这个操作无法用库函数代码表示
  {
    // 若RTC未打开, 则初始化RTC
    // 必须要先选择时钟, 然后再开启RTC时钟
#ifdef RTC_USELSI
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI); // 选LSI作为RTC时钟
#else
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
#endif
    RCC_RTCCLKCmd(ENABLE); // 开启RTC时钟, RTC开始走时
    
    RTC_SetPrescaler(RTC_PRESCALER - 1); // 设置分频系数: 定时1s
    RTC_WaitForLastTask();
    //RTC_SetCounter(50); // 设置初始时间: STM32F1系列的RTC没有年月日、小时等寄存器, 只有一个32位的计数器, 要想实现日期和时间的功能必须调用C库<time.h>中的mktime函数, 用软件来实现这些功能
    //RTC_WaitForLastTask();
  }
  else
    RTC_WaitForSynchro(); // 等待RTC与APB1时钟同步
}

/* RTC时间转化为毫秒数 (lwip协议栈要求实现的函数) */
// 该函数必须保证: 除非定时器溢出, 否则后获取的时间必须大于先获取的时间
uint32_t sys_now(void)
{
  uint32_t sec[2];
  uint32_t div, milli;
  do
  {
    time(&sec[0]); // 秒
    div = rtc_divider();
    time(&sec[1]);
  } while (sec[0] != sec[1]);
  
  // CNT是在DIV从P-1跳变到P-2瞬间发生更新的 (P=RTC_PRESCALER)
  if (div == RTC_PRESCALER - 1)
    milli = div;
  else
    milli = RTC_PRESCALER - div - 2;
  milli = milli * 1000 / RTC_PRESCALER; // 毫秒
  return sec[0] * 1000 + milli;
}

/* 获取RTC秒数 */
time_t time(time_t *timer)
{
  // CNTH和CNTL是同时更新的
  // 但由于这两个寄存器不是同时读取的, 所以有可能一个读到的是更新前的值, 另一个读到的是更新后的值
  uint32_t now[2];
  do
  {
    now[0] = RTC_GetCounter();
    now[1] = RTC_GetCounter();
  } while ((now[0] >> 16) != (now[1] >> 16)); // 使用循环可以避免中断的影响
  
  // RTC_GetCounter函数先读取CNTL再读取CNTH
  // 如果更新发生在第一次读取CNTL后, 则now[0]和now[1]的高16位相等, 且now[0]值的高、低16位不匹配，now[1]值的高、低16位匹配
  // 如果更新发生在第一次读取CNTH后或第二次读取CNTL后, 则now[0]和now[1]的高16位不相等, 两个值都会被丢弃
  if (timer)
    *timer = now[1];
  return now[1]; // 所以应该取第二次读取的值
}
