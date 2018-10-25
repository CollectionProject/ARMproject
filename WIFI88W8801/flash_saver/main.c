#include <stdio.h>
#include <stm32f4xx.h>

#ifndef STM32F40_41xxx
#warning "Please check the configuration!"
#endif

#define FLASH_PAGESIZE 131072 // Flash页大小
#define FLASH_SECTOR_START 0x08020000
#define FLASH_SECTOR_NUMBER_START 5

extern const unsigned char firmware_sd8801[255536];

const uint8_t *data_addr[] = {firmware_sd8801};
const uint32_t data_size[] = {255536};

static uint8_t data_count;
static uint32_t data_start;
static uint32_t flash_end;
static uint32_t total_size;

void dump_data(const void *data, uint32_t len)
{
  const uint8_t *p = data;
  while (len--)
    printf("%02X", *p++);
  printf("\n");
}

int fputc(int ch, FILE *fp)
{
  if (fp == stdout)
  {
    if (ch == '\n')
    {
      while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
      USART_SendData(USART1, '\r');
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, ch);
  }
  return ch;
}

uint8_t verify(void)
{
  uint32_t crc;
  printf("Verify...\n");
  
  CRC_ResetDR();
  crc = CRC_CalcBlockCRC((uint32_t *)data_start, total_size / 4);
  if (crc == 0)
  {
    printf("OK!\n");
    return 1;
  }
  else
  {
    printf("Not OK!\n");
    return 0;
  }
}

void program(void)
{
  uint8_t i;
  uint32_t j, addr, data;
  
  FLASH_Unlock();
  if ((FLASH->CR & FLASH_CR_LOCK) == 0)
    printf("Flash is unlocked!\n");
  else
  {
    printf("Unlock Flash failed!\n");
    return;
  }
  
  printf("Erase sectors...\n");
  for (addr = data_start; addr < flash_end; addr += FLASH_PAGESIZE)
  {
    i = (addr - FLASH_SECTOR_START) / FLASH_PAGESIZE + FLASH_SECTOR_NUMBER_START;
    FLASH_EraseSector(8 * i, VoltageRange_3);
  }
  
  printf("Program...\n");
  CRC_ResetDR();
  addr = data_start;
  for (i = 0; i < data_count; i++)
  {
    CRC_CalcCRC(data_size[i]);
    FLASH_ProgramWord(addr, data_size[i]);
    addr += 4;
  }
  
  for (i = 0; i < data_count; i++)
  {
    for (j = 0; j < data_size[i]; j += 4)
    {
      data = *(uint32_t *)(data_addr[i] + j);
      CRC_CalcCRC(data);
      FLASH_ProgramWord(addr, data);
      addr += 4;
    }
  }
  
  data = CRC_GetCRC();
  FLASH_ProgramWord(addr, data);
  
  FLASH_Lock();
  printf("Flash is locked!\n");
}

uint8_t calc_addr(void)
{
  uint8_t i;
  uint16_t flash_size = *(uint16_t *)0x1fff7a22;
  flash_end = 0x8000000 + flash_size * 1024 - 1;
  
  printf("Device ID: ");
  dump_data((void *)0x1fff7a10, 12);
  printf("Flash size: %dKB\n", flash_size);
  
  data_count = sizeof(data_size) / sizeof(data_size[0]);
  total_size = (data_count + 1) * sizeof(uint32_t);
  for (i = 0; i < data_count; i++)
    total_size += data_size[i];
  printf("Count: %d\n", data_count);
  printf("Total size: %d\n", total_size);

  data_start = ((flash_end | (FLASH_PAGESIZE - 1)) - total_size + 1) & ~(FLASH_PAGESIZE - 1);
  printf("Start address: 0x%08x (WIFI_FIRMWAREAREA_ADDR in WiFi.h)\n", data_start);
  printf("End address: 0x%08x\n", data_start + total_size - 1);
  
#ifdef FLASH_SECTOR_START
  if (data_start < FLASH_SECTOR_START)
  {
    printf("Error: start address must be greater than or equal to 0x%08x!\n", FLASH_SECTOR_START);
    return 0;
  }
#endif
  
  // 检查芯片Flash大小是否足够
  for (i = 0; i < data_count; i++)
  {
    if ((uint32_t)data_addr[i] + data_size[i] > data_start)
    {
      printf("Error: flash size is too small!\n");
      return 0;
    }
  }
  return 1;
}

int main(void)
{
  GPIO_InitTypeDef gpio;
  USART_InitTypeDef usart;
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC | RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
  
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
  
  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
  gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
  gpio.GPIO_Speed = GPIO_High_Speed;
  GPIO_Init(GPIOA, &gpio);
  
  USART_StructInit(&usart);
  usart.USART_BaudRate = 115200;
  USART_Init(USART1, &usart);
  USART_Cmd(USART1, ENABLE);
  printf("Marvell 88W8801 firmware saver for STM32F40_41xxx\n");
  
  if (calc_addr())
  {
    if (*(uint32_t *)data_start != data_size[0])
    {
      printf("Data haven't been programmed!\n");
      program();
      verify();
    }
    else
    {
      printf("Data have been programmed!\n");
      if (!verify())
      {
        program();
        verify();
      }
    }
  }
  
  while (1);
}

void HardFault_Handler(void)
{
  printf("Hard Error!\n");
  while (1);
}
