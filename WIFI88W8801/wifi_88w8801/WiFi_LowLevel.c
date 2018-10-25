// 定义与单片机寄存器操作相关的函数, 方便在不同平台间移植
#include <stdio.h>
#include <stm32f4xx.h>
#include <string.h>
#include "common.h"
#include "WiFi.h"

#define CMD52_WRITE _BV(31)
#define CMD52_READAFTERWRITE _BV(27)
#define CMD53_WRITE _BV(31)
#define CMD53_BLOCKMODE _BV(27)
#define CMD53_INCREMENTING _BV(26)

// 高速模式下必须使用DMA
#if defined(WIFI_HIGHSPEED) && !defined(WIFI_USEDMA)
#error "DMA must be enabled when SDIO is in high speed mode!"
#endif

static uint8_t WiFi_LowLevel_CheckError(const char *msg_title);
static uint16_t WiFi_LowLevel_GetBlockNum(uint8_t func, uint32_t *psize);
static void WiFi_LowLevel_GPIOInit(void);
static void WiFi_LowLevel_SDIOInit(void);
static void WiFi_LowLevel_SendCMD52(uint8_t func, uint32_t addr, uint8_t data, uint32_t flags);
static void WiFi_LowLevel_SendCMD53(uint8_t func, uint32_t addr, uint16_t count, uint32_t flags);
static void WiFi_LowLevel_SetSDIOBlockSize(uint32_t size);
#ifdef WIFI_FIRMWAREAREA_ADDR
static uint8_t WiFi_LowLevel_VerifyFirmware(void);
#endif
static void WiFi_LowLevel_WaitForResponse(const char *msg_title);

static uint16_t sdio_block_size[2]; // 各功能区的块大小, 保存在此变量中避免每次都去发送CMD52命令读SDIO寄存器
static uint8_t sdio_func_num = 0; // 功能区总数 (0号功能区除外)
static uint16_t sdio_rca; // RCA相对地址: 虽然SDIO标准规定SDIO接口上可以接多张SD卡, 但是STM32的SDIO接口只能接一张卡 (芯片手册上有说明)
static SDIO_CmdInitTypeDef sdio_cmd;
static SDIO_DataInitTypeDef sdio_data;

/* 检查并清除错误标志位 */
static uint8_t WiFi_LowLevel_CheckError(const char *msg_title)
{
  uint8_t err = 0;
  if (SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) == SET)
  {
    SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
    err++;
    printf("%s: CMD%d CRC failed!\n", msg_title, SDIO->CMD & SDIO_CMD_CMDINDEX);
  }
  if (SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) == SET)
  {
    SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
    err++;
    printf("%s: CMD%d timeout!\n", msg_title, SDIO->CMD & SDIO_CMD_CMDINDEX);
  }
  if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) == SET)
  {
    SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
    err++;
    printf("%s: data CRC failed!\n", msg_title);
  }
  if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) == SET)
  {
    SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
    err++;
    printf("%s: data timeout!\n", msg_title);
  }
  if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) == SET)
  {
    SDIO_ClearFlag(SDIO_FLAG_STBITERR);
    err++;
    printf("%s: start bit error!\n", msg_title);
  }
  if (SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) == SET)
  {
    SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);
    err++;
    printf("%s: data underrun!\n", msg_title);
  }
  if (SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) == SET)
  {
    SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
    err++;
    printf("%s: data overrun!\n", msg_title);
  }
#ifdef WIFI_USEDMA
  if (DMA_GetFlagStatus(DMA2_Stream3, DMA_FLAG_TEIF3) == SET)
  {
    DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TEIF3);
    err++;
    printf("%s: DMA transfer error!\n", msg_title);
  }
  if (DMA_GetFlagStatus(DMA2_Stream3, DMA_FLAG_FEIF3) == SET)
  {
    DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_FEIF3);
    err++;
    printf("%s: DMA FIFO error!\n", msg_title);
  }
  if (DMA_GetFlagStatus(DMA2_Stream3, DMA_FLAG_DMEIF3) == SET)
  {
    DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_DMEIF3);
    err++;
    printf("%s: DMA direct mode error!\n", msg_title);
  }
#endif
  return err;
}

/* 判断应该采用哪种方式传输数据 */
// 返回值: 0为多字节模式, 否则表示块传输模式的数据块数
// *psize的值会做适当调整, 有可能大于原值
static uint16_t WiFi_LowLevel_GetBlockNum(uint8_t func, uint32_t *psize)
{
  // 88W8801除了下载固件外, 只能使用块传输模式(DTMODE=0)读写数据
  uint16_t block_num;
  WiFi_LowLevel_SetSDIOBlockSize(sdio_block_size[func]);
  
  block_num = *psize / sdio_block_size[func];
  if (*psize % sdio_block_size[func] != 0)
    block_num++;
  *psize = block_num * sdio_block_size[func]; // 块数*块大小
  return block_num;
}

/* 获取WiFi模块支持SDIO功能区个数 (0号功能区除外) */
uint8_t WiFi_LowLevel_GetFunctionNum(void)
{
  return sdio_func_num;
}

/* 初始化WiFi模块有关的所有GPIO引脚 */
static void WiFi_LowLevel_GPIOInit(void)
{
  GPIO_InitTypeDef gpio;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);
  
  // 使Wi-Fi模块复位信号(PDN)有效
  gpio.GPIO_Mode = GPIO_Mode_OUT;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_Pin = GPIO_Pin_14;
  gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
  gpio.GPIO_Speed = GPIO_Low_Speed;
  GPIO_Init(GPIOD, &gpio);
  GPIO_WriteBit(GPIOD, GPIO_Pin_14, Bit_RESET);
  
  // PA15为MOS管控制的Wi-Fi模块电源开关, 低电平时Wi-Fi模块通电
  gpio.GPIO_Pin = GPIO_Pin_15;
  GPIO_Init(GPIOA, &gpio);
  GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_RESET);
  
  // 撤销Wi-Fi模块的复位信号
  delay(100); // 延时一段时间, 使WiFi模块能够正确复位
  GPIO_WriteBit(GPIOD, GPIO_Pin_14, Bit_SET);
  
  // SDIO相关引脚
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_SDIO);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_SDIO);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SDIO);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SDIO);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SDIO);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource2, GPIO_AF_SDIO);
  
  // PC8~11: SDIO_D0~3, PC12: SDIO_CK, 设为复用推挽输出
  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
  gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
  gpio.GPIO_Speed = GPIO_High_Speed;
  GPIO_Init(GPIOC, &gpio);
  
  // PD2: SDIO_CMD, 设为复用推挽输出
  gpio.GPIO_Pin = GPIO_Pin_2;
  GPIO_Init(GPIOD, &gpio);
}

void WiFi_LowLevel_Init(void)
{
  // 在此处打开WiFi模块所需要的除GPIO和SDIO外所有其他外设的时钟
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);
  
  // 检查Flash中保存的固件内容是否已被破坏
#ifdef WIFI_FIRMWAREAREA_ADDR
  if (!WiFi_LowLevel_VerifyFirmware())
  {
    printf("Error: The firmware stored in flash memory is corrupted!\n");
    while (1);
  }
#endif
  
  WiFi_LowLevel_GPIOInit();
  WiFi_LowLevel_SDIOInit();
}

/* 接收数据, 自动判断采用哪种传输模式 */
// size为要接收的字节数, bufsize为data缓冲区的大小
// 若bufsize=0, 则只读取数据, 但不保存到data中, 此时data可以为NULL
uint8_t WiFi_LowLevel_ReadData(uint8_t func, uint32_t addr, void *data, uint32_t size, uint32_t bufsize)
{
  uint16_t block_num; // 数据块个数
#ifdef WIFI_USEDMA
  DMA_InitTypeDef dma;
  uint32_t temp; // 丢弃数据用的变量
#else
  uint32_t *p = data;
#endif
  if ((uintptr_t)data & 3)
    printf("WiFi_LowLevel_ReadData: data must be 4-byte aligned!\n"); // DMA每次传输多个字节时, 内存和外设地址必须要对齐, 否则将不能正确传输且不会提示错误
  if (size == 0)
  {
    printf("WiFi_LowLevel_ReadData: size cannot be 0!\n");
    return 0;
  }
  
  block_num = WiFi_LowLevel_GetBlockNum(func, &size);
  if (bufsize > 0 && bufsize < size)
  {
    printf("WiFi_LowLevel_ReadData: a buffer of at least %d bytes is required! bufsize=%d\n", size, bufsize);
    WiFi_LowLevel_ReadData(func, addr, NULL, size, 0); // 丢弃数据
    return 0;
  }
  
#ifdef WIFI_USEDMA
  dma.DMA_BufferSize = 0;
  dma.DMA_Channel = DMA_Channel_4;
  dma.DMA_DIR = DMA_DIR_PeripheralToMemory;
  dma.DMA_FIFOMode = DMA_FIFOMode_Enable;
  dma.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  dma.DMA_MemoryBurst = DMA_MemoryBurst_INC4;
  dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  dma.DMA_Mode = DMA_Mode_Normal;
  dma.DMA_PeripheralBaseAddr = (uint32_t)&SDIO->FIFO;
  dma.DMA_PeripheralBurst = DMA_PeripheralBurst_INC4;
  dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  dma.DMA_Priority = DMA_Priority_VeryHigh;
  if (bufsize > 0)
  {
    dma.DMA_Memory0BaseAddr = (uint32_t)data;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
  }
  else
  {
    // 数据丢弃模式
    dma.DMA_Memory0BaseAddr = (uint32_t)&temp;
    dma.DMA_MemoryInc = DMA_MemoryInc_Disable;
  }
  DMA_Init(DMA2_Stream3, &dma);
  DMA_Cmd(DMA2_Stream3, ENABLE);
#endif
  
  if (block_num)
  {
    sdio_data.SDIO_TransferMode = SDIO_TransferMode_Block;
    WiFi_LowLevel_SendCMD53(func, addr, block_num, CMD53_BLOCKMODE);
  }
  else
  {
    sdio_data.SDIO_TransferMode = SDIO_TransferMode_Stream;
    WiFi_LowLevel_SendCMD53(func, addr, size, 0);
  }
  
  sdio_data.SDIO_DataLength = size;
  sdio_data.SDIO_DPSM = SDIO_DPSM_Enable;
  sdio_data.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
  SDIO_DataConfig(&sdio_data);
  
  while (SDIO_GetFlagStatus(SDIO_FLAG_RXACT) == RESET); // 等待开始接收数据
#ifdef WIFI_USEDMA
  while (DMA_GetFlagStatus(DMA2_Stream3, DMA_FLAG_TCIF3) == RESET) // 等待DMA读取数据完毕
  {
    if (SDIO_GetFlagStatus(SDIO_FLAG_RXACT) == RESET || SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) == SET)
      break; // 如果出现错误, 则退出循环
  }
  DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3); // 清除DMA传输完成标志位
  DMA_Cmd(DMA2_Stream3, DISABLE); // 关闭DMA
#else
  while (size && SDIO_GetFlagStatus(SDIO_FLAG_RXACT) == SET && SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) == RESET)
  {
    // 如果有数据到来就读取数据
    if (SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) == SET)
    {
      size -= 4;
      if (bufsize > 0)
        *p++ = SDIO_ReadData();
      else
        SDIO_ReadData(); // 读寄存器, 但不保存数据
    }
  }
#endif
  
  while ((SDIO_GetFlagStatus(SDIO_FLAG_CMDACT) == SET || SDIO_GetFlagStatus(SDIO_FLAG_RXACT) == SET) && SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) == RESET); // 等待接收完毕
  sdio_data.SDIO_DPSM = SDIO_DPSM_Disable;
  SDIO_DataConfig(&sdio_data);
  
  // 清除相关标志位
  SDIO_ClearFlag(SDIO_FLAG_CMDREND | SDIO_FLAG_DATAEND | SDIO_FLAG_DBCKEND);
  return WiFi_LowLevel_CheckError(__func__) == 0;
}

/* 读SDIO寄存器 */
uint8_t WiFi_LowLevel_ReadReg(uint8_t func, uint32_t addr)
{
  WiFi_LowLevel_SendCMD52(func, addr, 0, 0);
  WiFi_LowLevel_WaitForResponse(__func__);
  return SDIO_GetResponse(SDIO_RESP1) & 0xff;
}

/* 初始化SDIO外设并完成WiFi模块的枚举 */
// SDIO Simplified Specification Version 3.00: 3. SDIO Card Initialization
static void WiFi_LowLevel_SDIOInit(void)
{
  SDIO_InitTypeDef sdio;
  uint32_t resp;
  
  // SDIO外设拥有两个时钟: SDIOCLK=48MHz(分频后用于产生SDIO_CK=PC12引脚时钟), APB2 bus clock=PCLK2=84MHz(用于访问寄存器)
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SDIO, ENABLE);
#ifdef WIFI_USEDMA
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
#endif
  
  SDIO_SetPowerState(SDIO_PowerState_ON); // 打开SDIO外设
  SDIO_StructInit(&sdio);
  sdio.SDIO_ClockDiv = 118; // 初始化时最高允许的频率: 48MHz/(118+2)=400kHz
  SDIO_Init(&sdio);
  SDIO_ClockCmd(ENABLE);
  
#ifdef WIFI_USEDMA
  DMA_FlowControllerConfig(DMA2_Stream3, DMA_FlowCtrl_Peripheral); // 必须由SDIO控制DMA传输的数据量
  SDIO_DMACmd(ENABLE); // 设为DMA传输模式 (在STM32F4上, DMACmd必须在SetSDIOOperation前执行, 否则有可能执行失败)
#endif
  SDIO_SetSDIOOperation(ENABLE); // 设为SDIO模式
  
  // 不需要发送CMD0, 因为SD I/O card的初始化命令是CMD52
  // An I/O only card or the I/O portion of a combo card is NOT reset by CMD0. (See 4.4 Reset for SDIO)
  delay(10); // 延时可防止CMD5重发
  
  /* 发送CMD5: IO_SEND_OP_COND */
  sdio_cmd.SDIO_Argument = 0;
  sdio_cmd.SDIO_CmdIndex = 5;
  sdio_cmd.SDIO_CPSM = SDIO_CPSM_Enable;
  sdio_cmd.SDIO_Response = SDIO_Response_Short; // 接收短回应
  sdio_cmd.SDIO_Wait = SDIO_Wait_No;
  SDIO_SendCommand(&sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__func__);
  printf("RESPCMD%d, RESP1_%08x\n", SDIO_GetCommandResponse(), SDIO_GetResponse(SDIO_RESP1));
  
  /* 设置参数VDD Voltage Window: 3.2~3.4V, 并再次发送CMD5 */
  sdio_cmd.SDIO_Argument = 0x300000;
  SDIO_SendCommand(&sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__func__);
  resp = SDIO_GetResponse(SDIO_RESP1);
  printf("RESPCMD%d, RESP1_%08x\n", SDIO_GetCommandResponse(), resp);
  if (resp & _BV(31))
  {
    // Card is ready to operate after initialization
    sdio_func_num = (resp >> 28) & 7;
    printf("Number of I/O Functions: %d\n", sdio_func_num);
    printf("Memory Present: %d\n", (resp & _BV(27)) != 0);
  }
  
  /* 获取WiFi模块地址 (CMD3: SEND_RELATIVE_ADDR, Ask the card to publish a new relative address (RCA)) */
  sdio_cmd.SDIO_Argument = 0;
  sdio_cmd.SDIO_CmdIndex = 3;
  SDIO_SendCommand(&sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__func__);
  sdio_rca = SDIO_GetResponse(SDIO_RESP1) >> 16;
  printf("Relative Card Address: 0x%04x\n", sdio_rca);
  
  /* 选中WiFi模块 (CMD7: SELECT/DESELECT_CARD) */
  sdio_cmd.SDIO_Argument = sdio_rca << 16;
  sdio_cmd.SDIO_CmdIndex = 7;
  SDIO_SendCommand(&sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__func__);
  printf("Card selected! RESP1_%08x\n", SDIO_GetResponse(SDIO_RESP1));
  
  /* 提高时钟频率, 并设置数据超时时间为0.1s */
#ifdef WIFI_HIGHSPEED
  sdio.SDIO_ClockDiv = 0; // 48MHz/(0+2)=24MHz
  sdio_data.SDIO_DataTimeOut = 2400000;
  printf("SDIO Clock: 24MHz\n");
#else
  sdio.SDIO_ClockDiv = 46; // 48MHz/(46+2)=1MHz
  sdio_data.SDIO_DataTimeOut = 100000;
  printf("SDIO Clock: 1MHz\n");
#endif
  
  /* SDIO外设的总线宽度设为4位 */
  sdio.SDIO_BusWide = SDIO_BusWide_4b;
  SDIO_Init(&sdio);
  WiFi_LowLevel_WriteReg(0, SDIO_CCCR_BUSIFCTRL, WiFi_LowLevel_ReadReg(0, SDIO_CCCR_BUSIFCTRL) | SDIO_CCCR_BUSIFCTRL_BUSWID_4Bit);
}

static void WiFi_LowLevel_SendCMD52(uint8_t func, uint32_t addr, uint8_t data, uint32_t flags)
{
  sdio_cmd.SDIO_Argument = (func << 28) | (addr << 9) | data | flags;
  sdio_cmd.SDIO_CmdIndex = 52;
  sdio_cmd.SDIO_CPSM = SDIO_CPSM_Enable;
  sdio_cmd.SDIO_Response = SDIO_Response_Short;
  sdio_cmd.SDIO_Wait = SDIO_Wait_No;
  SDIO_SendCommand(&sdio_cmd);
}

static void WiFi_LowLevel_SendCMD53(uint8_t func, uint32_t addr, uint16_t count, uint32_t flags)
{
  // 当count=512时, 和0x1ff相与后为0, 符合SDIO标准
  sdio_cmd.SDIO_Argument = (func << 28) | (addr << 9) | (count & 0x1ff) | flags;
  sdio_cmd.SDIO_CmdIndex = 53;
  sdio_cmd.SDIO_CPSM = SDIO_CPSM_Enable;
  sdio_cmd.SDIO_Response = SDIO_Response_Short;
  sdio_cmd.SDIO_Wait = SDIO_Wait_No;
  SDIO_SendCommand(&sdio_cmd);
}

/* 设置WiFi模块功能区的数据块大小 */
void WiFi_LowLevel_SetBlockSize(uint8_t func, uint32_t size)
{
  sdio_block_size[func] = size;
  WiFi_LowLevel_WriteReg(0, (func << 8) | 0x10, size & 0xff);
  WiFi_LowLevel_WriteReg(0, (func << 8) | 0x11, size >> 8);
}

/* 设置SDIO外设的数据块大小 */
static void WiFi_LowLevel_SetSDIOBlockSize(uint32_t size)
{
  switch (size)
  {
    case 1:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_1b;
      break;
    case 2:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_2b;
      break;
    case 4:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_4b;
      break;
    case 8:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_8b;
      break;
    case 16:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_16b;
      break;
    case 32:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_32b;
      break;
    case 64:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_64b;
      break;
    case 128:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_128b;
      break;
    case 256:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_256b;
      break;
    case 512:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_512b;
      break;
    case 1024:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_1024b;
      break;
    case 2048:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_2048b;
      break;
    case 4096:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_4096b;
      break;
    case 8192:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_8192b;
      break;
    case 16384:
      sdio_data.SDIO_DataBlockSize = SDIO_DataBlockSize_16384b;
      break;
  }
}

/* 检查Flash中保存的固件内容是否完整 */
#ifdef WIFI_FIRMWAREAREA_ADDR
static uint8_t WiFi_LowLevel_VerifyFirmware(void)
{
  uint32_t crc, len;
  if (WIFI_FIRMWARE_SIZE != 255536)
    return 0;
  
  len = WIFI_FIRMWARE_SIZE / 4 + 2; // 固件区(包括CRC)总大小的1/4
  CRC_ResetDR();
  crc = CRC_CalcBlockCRC((uint32_t *)WIFI_FIRMWAREAREA_ADDR, len);
  return crc == 0;
}
#endif

/* 等待SDIO命令回应 */
static void WiFi_LowLevel_WaitForResponse(const char *msg_title)
{
  uint8_t first = 1;
  do
  {
    if (!first)
      SDIO_SendCommand(&sdio_cmd); // 重发命令
    else
      first = 0;
    while (SDIO_GetFlagStatus(SDIO_FLAG_CMDACT) == SET); // 等待命令发送完毕
    WiFi_LowLevel_CheckError(msg_title);
  } while (SDIO_GetFlagStatus(SDIO_FLAG_CMDREND) == RESET); // 如果没有收到回应, 则重试
  SDIO_ClearFlag(SDIO_FLAG_CMDREND);
}

/* 发送数据, 自动判断采用哪种传输模式 */
// size为要发送的字节数, bufsize为data缓冲区的大小
uint8_t WiFi_LowLevel_WriteData(uint8_t func, uint32_t addr, const void *data, uint32_t size, uint32_t bufsize)
{
  uint16_t block_num; // 数据块个数
#ifdef WIFI_USEDMA
  DMA_InitTypeDef dma;
#else
  const uint32_t *p = data;
#endif
  if ((uintptr_t)data & 3)
    printf("WiFi_LowLevel_WriteData: data must be 4-byte aligned!\n");
  if (size == 0)
  {
    printf("WiFi_LowLevel_WriteData: size cannot be 0!\n");
    return 0;
  }

  block_num = WiFi_LowLevel_GetBlockNum(func, &size);
  if (bufsize < size)
    printf("WiFi_LowLevel_WriteData: a buffer of at least %d bytes is required! bufsize=%d\n", size, bufsize); // 只读缓冲区越界不会影响数据传输, 所以这只是一个警告
  
  if (block_num)
  {
    sdio_data.SDIO_TransferMode = SDIO_TransferMode_Block;
    WiFi_LowLevel_SendCMD53(func, addr, block_num, CMD53_WRITE | CMD53_BLOCKMODE);
  }
  else
  {
    sdio_data.SDIO_TransferMode = SDIO_TransferMode_Stream;
    WiFi_LowLevel_SendCMD53(func, addr, size, CMD53_WRITE);
  }
  WiFi_LowLevel_WaitForResponse(__func__); // 必须要等到CMD53收到回应后才能开始发送数据
  
  // 开始发送数据
#ifdef WIFI_USEDMA
  dma.DMA_BufferSize = 0; // 由SDIO控制流量时, 该选项无效
  dma.DMA_Channel = DMA_Channel_4;
  dma.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  dma.DMA_FIFOMode = DMA_FIFOMode_Enable; // 必须使用FIFO模式
  dma.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  dma.DMA_Memory0BaseAddr = (uint32_t)data;
  dma.DMA_MemoryBurst = DMA_MemoryBurst_INC4;
  dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
  dma.DMA_Mode = DMA_Mode_Normal;
  dma.DMA_PeripheralBaseAddr = (uint32_t)&SDIO->FIFO;
  dma.DMA_PeripheralBurst = DMA_PeripheralBurst_INC4;
  dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  dma.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_Init(DMA2_Stream3, &dma);
  DMA_Cmd(DMA2_Stream3, ENABLE);
#endif
  
  sdio_data.SDIO_DataLength = size;
  sdio_data.SDIO_DPSM = SDIO_DPSM_Enable;
  sdio_data.SDIO_TransferDir = SDIO_TransferDir_ToCard;
  SDIO_DataConfig(&sdio_data);
  
  while (SDIO_GetFlagStatus(SDIO_FLAG_TXACT) == RESET); // 等待开始发送数据
#ifdef WIFI_USEDMA
  while (DMA_GetFlagStatus(DMA2_Stream3, DMA_FLAG_TCIF3) == RESET) // 等待DMA送入数据完毕
  {
    if (SDIO_GetFlagStatus(SDIO_FLAG_TXACT) == RESET || SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) == SET)
      break; // 如果出现错误, 则退出循环
  }
  DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3 | DMA_FLAG_FEIF3); // 清除DMA传输完成标志位, 忽略FIFO error错误
  DMA_Cmd(DMA2_Stream3, DISABLE); // 关闭DMA
#else
  while (size && SDIO_GetFlagStatus(SDIO_FLAG_TXACT) == SET && SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) == RESET)
  {
    size -= 4;
    SDIO_WriteData(*p++); // 向FIFO送入4字节数据
    while (SDIO_GetFlagStatus(SDIO_FLAG_TXFIFOF) == SET); // 如果FIFO已满则等待
  }
#endif
  
  while (SDIO_GetFlagStatus(SDIO_FLAG_TXACT) == SET && SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) == RESET); // 等待发送完毕
  sdio_data.SDIO_DPSM = SDIO_DPSM_Disable;
  SDIO_DataConfig(&sdio_data);
  
  // 清除相关标志位
  SDIO_ClearFlag(SDIO_FLAG_DATAEND | SDIO_FLAG_DBCKEND);
  return WiFi_LowLevel_CheckError(__func__) == 0;
}

/* 写寄存器, 返回写入后寄存器的实际内容 */
uint8_t WiFi_LowLevel_WriteReg(uint8_t func, uint32_t addr, uint8_t value)
{
  WiFi_LowLevel_SendCMD52(func, addr, value, CMD52_WRITE | CMD52_READAFTERWRITE);
  WiFi_LowLevel_WaitForResponse(__func__);
  return SDIO_GetResponse(SDIO_RESP1) & 0xff;
}
