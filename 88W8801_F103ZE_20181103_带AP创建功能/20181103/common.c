#include <stdio.h>
#include <stm32f10x.h>
#include <time.h>
#include "common.h"

#define RTC_USELSI // ��Ϊ����û��LSE����, ����RTCʱ��ѡLSI

#ifdef RTC_USELSI
#define RTC_PRESCALER 40000 // LSIƵ��
#else
#define RTC_PRESCALER 32768 // LSEƵ��
#endif

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

/* ��ȡRTC��Ƶ��������ֵ */
uint32_t rtc_divider(void)
{
  uint32_t div[2];
  do
  {
    div[0] = RTC_GetDivider();
    div[1] = RTC_GetDivider();
  } while ((div[0] >> 16) != (div[1] >> 16));
  
  // RTC_GetDivider�����ȶ�ȡDIVH�ٶ�ȡDIVL
  // ������·����ڵ�һ�ζ�ȡDIVH��DIVL��, ��div[0]��div[1]�ĸ�16λ�����, ����ֵ���ᱻ����
  // ������·����ڵڶ��ζ�ȡDIVH��, ��div[0]��div[1]�ĸ�16λ���, ��div[0]ֵ�ĸߡ���16λƥ�䣬div[1]ֵ�ĸߡ���16λ��ƥ��
  return div[0]; // ����Ӧ��ȡ��һ�ζ�ȡ��ֵ
}

/* ��ʼ��RTC���� */
// ���sys_now����������RTCʵ�ֵ�, �����ɾ���������
void rtc_init(void)
{
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  PWR_BackupAccessCmd(ENABLE); // ����д�󱸼Ĵ���(��RCC->BDCR)
  
  // ���궨��RTC_USELSI�Ƿ���ȷ
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
  while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET); // �ȴ�LSI����
#else
  if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
  {
    RCC_LSEConfig(RCC_LSE_ON);
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET); // �ȴ�LSE����
  }
#endif
  
  if ((RCC->BDCR & RCC_BDCR_RTCEN) == 0) // ��������޷��ÿ⺯�������ʾ
  {
    // ��RTCδ��, ���ʼ��RTC
    // ����Ҫ��ѡ��ʱ��, Ȼ���ٿ���RTCʱ��
#ifdef RTC_USELSI
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI); // ѡLSI��ΪRTCʱ��
#else
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
#endif
    RCC_RTCCLKCmd(ENABLE); // ����RTCʱ��, RTC��ʼ��ʱ
    
    RTC_SetPrescaler(RTC_PRESCALER - 1); // ���÷�Ƶϵ��: ��ʱ1s
    RTC_WaitForLastTask();
    //RTC_SetCounter(50); // ���ó�ʼʱ��: STM32F1ϵ�е�RTCû�������ա�Сʱ�ȼĴ���, ֻ��һ��32λ�ļ�����, Ҫ��ʵ�����ں�ʱ��Ĺ��ܱ������C��<time.h>�е�mktime����, �������ʵ����Щ����
    //RTC_WaitForLastTask();
  }
  else
    RTC_WaitForSynchro(); // �ȴ�RTC��APB1ʱ��ͬ��
}

/* RTCʱ��ת��Ϊ������ (lwipЭ��ջҪ��ʵ�ֵĺ���) */
// �ú������뱣֤: ���Ƕ�ʱ�����, ������ȡ��ʱ���������Ȼ�ȡ��ʱ��
uint32_t sys_now(void)
{
  uint32_t sec[2];
  uint32_t div, milli;
  do
  {
    time(&sec[0]); // ��
    div = rtc_divider();
    time(&sec[1]);
  } while (sec[0] != sec[1]);
  
  // CNT����DIV��P-1���䵽P-2˲�䷢�����µ� (P=RTC_PRESCALER)
  if (div == RTC_PRESCALER - 1)
    milli = div;
  else
    milli = RTC_PRESCALER - div - 2;
  milli = milli * 1000 / RTC_PRESCALER; // ����
  return sec[0] * 1000 + milli;
}

/* ��ȡRTC���� */
time_t time(time_t *timer)
{
  // CNTH��CNTL��ͬʱ���µ�
  // �������������Ĵ�������ͬʱ��ȡ��, �����п���һ���������Ǹ���ǰ��ֵ, ��һ���������Ǹ��º��ֵ
  uint32_t now[2];
  do
  {
    now[0] = RTC_GetCounter();
    now[1] = RTC_GetCounter();
  } while ((now[0] >> 16) != (now[1] >> 16)); // ʹ��ѭ�����Ա����жϵ�Ӱ��
  
  // RTC_GetCounter�����ȶ�ȡCNTL�ٶ�ȡCNTH
  // ������·����ڵ�һ�ζ�ȡCNTL��, ��now[0]��now[1]�ĸ�16λ���, ��now[0]ֵ�ĸߡ���16λ��ƥ�䣬now[1]ֵ�ĸߡ���16λƥ��
  // ������·����ڵ�һ�ζ�ȡCNTH���ڶ��ζ�ȡCNTL��, ��now[0]��now[1]�ĸ�16λ�����, ����ֵ���ᱻ����
  if (timer)
    *timer = now[1];
  return now[1]; // ����Ӧ��ȡ�ڶ��ζ�ȡ��ֵ
}
