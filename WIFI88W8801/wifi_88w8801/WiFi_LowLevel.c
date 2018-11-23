// �����뵥Ƭ���Ĵ���������صĺ���, �����ڲ�ͬƽ̨����ֲ
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

// ����ģʽ�±���ʹ��DMA
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

static uint16_t sdio_block_size[2]; // ���������Ŀ��С, �����ڴ˱����б���ÿ�ζ�ȥ����CMD52�����SDIO�Ĵ���
static uint8_t sdio_func_num = 0; // ���������� (0�Ź���������)
static uint16_t sdio_rca; // RCA��Ե�ַ: ��ȻSDIO��׼�涨SDIO�ӿ��Ͽ��ԽӶ���SD��, ����STM32��SDIO�ӿ�ֻ�ܽ�һ�ſ� (оƬ�ֲ�����˵��)
static SDIO_CmdInitTypeDef sdio_cmd;
static SDIO_DataInitTypeDef sdio_data;

/* ��鲢��������־λ */
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

/* �ж�Ӧ�ò������ַ�ʽ�������� */
// ����ֵ: 0Ϊ���ֽ�ģʽ, �����ʾ�鴫��ģʽ�����ݿ���
// *psize��ֵ�����ʵ�����, �п��ܴ���ԭֵ
static uint16_t WiFi_LowLevel_GetBlockNum(uint8_t func, uint32_t *psize)
{
  // 88W8801�������ع̼���, ֻ��ʹ�ÿ鴫��ģʽ(DTMODE=0)��д����
  uint16_t block_num;
  WiFi_LowLevel_SetSDIOBlockSize(sdio_block_size[func]);
  
  block_num = *psize / sdio_block_size[func];
  if (*psize % sdio_block_size[func] != 0)
    block_num++;
  *psize = block_num * sdio_block_size[func]; // ����*���С
  return block_num;
}

/* ��ȡWiFiģ��֧��SDIO���������� (0�Ź���������) */
uint8_t WiFi_LowLevel_GetFunctionNum(void)
{
  return sdio_func_num;
}

/* ��ʼ��WiFiģ���йص�����GPIO���� */
static void WiFi_LowLevel_GPIOInit(void)
{
  GPIO_InitTypeDef gpio;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);
  
  // ʹWi-Fiģ�鸴λ�ź�(PDN)��Ч
  gpio.GPIO_Mode = GPIO_Mode_OUT;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_Pin = GPIO_Pin_14;
  gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
  gpio.GPIO_Speed = GPIO_Low_Speed;
  GPIO_Init(GPIOD, &gpio);
  GPIO_WriteBit(GPIOD, GPIO_Pin_14, Bit_RESET);
  
  // PA15ΪMOS�ܿ��Ƶ�Wi-Fiģ���Դ����, �͵�ƽʱWi-Fiģ��ͨ��
  gpio.GPIO_Pin = GPIO_Pin_15;
  GPIO_Init(GPIOA, &gpio);
  GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_RESET);
  
  // ����Wi-Fiģ��ĸ�λ�ź�
  delay(100); // ��ʱһ��ʱ��, ʹWiFiģ���ܹ���ȷ��λ
  GPIO_WriteBit(GPIOD, GPIO_Pin_14, Bit_SET);
  
  // SDIO�������
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_SDIO);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_SDIO);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SDIO);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SDIO);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SDIO);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource2, GPIO_AF_SDIO);
  
  // PC8~11: SDIO_D0~3, PC12: SDIO_CK, ��Ϊ�����������
  gpio.GPIO_Mode = GPIO_Mode_AF;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
  gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
  gpio.GPIO_Speed = GPIO_High_Speed;
  GPIO_Init(GPIOC, &gpio);
  
  // PD2: SDIO_CMD, ��Ϊ�����������
  gpio.GPIO_Pin = GPIO_Pin_2;
  GPIO_Init(GPIOD, &gpio);
}

void WiFi_LowLevel_Init(void)
{
  // �ڴ˴���WiFiģ������Ҫ�ĳ�GPIO��SDIO���������������ʱ��
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);
  
  // ���Flash�б���Ĺ̼������Ƿ��ѱ��ƻ�
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

/* ��������, �Զ��жϲ������ִ���ģʽ */
// sizeΪҪ���յ��ֽ���, bufsizeΪdata�������Ĵ�С
// ��bufsize=0, ��ֻ��ȡ����, �������浽data��, ��ʱdata����ΪNULL
uint8_t WiFi_LowLevel_ReadData(uint8_t func, uint32_t addr, void *data, uint32_t size, uint32_t bufsize)
{
  uint16_t block_num; // ���ݿ����
#ifdef WIFI_USEDMA
  DMA_InitTypeDef dma;
  uint32_t temp; // ���������õı���
#else
  uint32_t *p = data;
#endif
  if ((uintptr_t)data & 3)
    printf("WiFi_LowLevel_ReadData: data must be 4-byte aligned!\n"); // DMAÿ�δ������ֽ�ʱ, �ڴ�������ַ����Ҫ����, ���򽫲�����ȷ�����Ҳ�����ʾ����
  if (size == 0)
  {
    printf("WiFi_LowLevel_ReadData: size cannot be 0!\n");
    return 0;
  }
  
  block_num = WiFi_LowLevel_GetBlockNum(func, &size);
  if (bufsize > 0 && bufsize < size)
  {
    printf("WiFi_LowLevel_ReadData: a buffer of at least %d bytes is required! bufsize=%d\n", size, bufsize);
    WiFi_LowLevel_ReadData(func, addr, NULL, size, 0); // ��������
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
    // ���ݶ���ģʽ
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
  
  while (SDIO_GetFlagStatus(SDIO_FLAG_RXACT) == RESET); // �ȴ���ʼ��������
#ifdef WIFI_USEDMA
  while (DMA_GetFlagStatus(DMA2_Stream3, DMA_FLAG_TCIF3) == RESET) // �ȴ�DMA��ȡ�������
  {
    if (SDIO_GetFlagStatus(SDIO_FLAG_RXACT) == RESET || SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) == SET)
      break; // ������ִ���, ���˳�ѭ��
  }
  DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3); // ���DMA������ɱ�־λ
  DMA_Cmd(DMA2_Stream3, DISABLE); // �ر�DMA
#else
  while (size && SDIO_GetFlagStatus(SDIO_FLAG_RXACT) == SET && SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) == RESET)
  {
    // ��������ݵ����Ͷ�ȡ����
    if (SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) == SET)
    {
      size -= 4;
      if (bufsize > 0)
        *p++ = SDIO_ReadData();
      else
        SDIO_ReadData(); // ���Ĵ���, ������������
    }
  }
#endif
  
  while ((SDIO_GetFlagStatus(SDIO_FLAG_CMDACT) == SET || SDIO_GetFlagStatus(SDIO_FLAG_RXACT) == SET) && SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) == RESET); // �ȴ��������
  sdio_data.SDIO_DPSM = SDIO_DPSM_Disable;
  SDIO_DataConfig(&sdio_data);
  
  // �����ر�־λ
  SDIO_ClearFlag(SDIO_FLAG_CMDREND | SDIO_FLAG_DATAEND | SDIO_FLAG_DBCKEND);
  return WiFi_LowLevel_CheckError(__func__) == 0;
}

/* ��SDIO�Ĵ��� */
uint8_t WiFi_LowLevel_ReadReg(uint8_t func, uint32_t addr)
{
  WiFi_LowLevel_SendCMD52(func, addr, 0, 0);
  WiFi_LowLevel_WaitForResponse(__func__);
  return SDIO_GetResponse(SDIO_RESP1) & 0xff;
}

/* ��ʼ��SDIO���貢���WiFiģ���ö�� */
// SDIO Simplified Specification Version 3.00: 3. SDIO Card Initialization
static void WiFi_LowLevel_SDIOInit(void)
{
  SDIO_InitTypeDef sdio;
  uint32_t resp;
  
  // SDIO����ӵ������ʱ��: SDIOCLK=48MHz(��Ƶ�����ڲ���SDIO_CK=PC12����ʱ��), APB2 bus clock=PCLK2=84MHz(���ڷ��ʼĴ���)
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SDIO, ENABLE);
#ifdef WIFI_USEDMA
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
#endif
  
  SDIO_SetPowerState(SDIO_PowerState_ON); // ��SDIO����
  SDIO_StructInit(&sdio);
  sdio.SDIO_ClockDiv = 118; // ��ʼ��ʱ��������Ƶ��: 48MHz/(118+2)=400kHz
  SDIO_Init(&sdio);
  SDIO_ClockCmd(ENABLE);
  
#ifdef WIFI_USEDMA
  DMA_FlowControllerConfig(DMA2_Stream3, DMA_FlowCtrl_Peripheral); // ������SDIO����DMA�����������
  SDIO_DMACmd(ENABLE); // ��ΪDMA����ģʽ (��STM32F4��, DMACmd������SetSDIOOperationǰִ��, �����п���ִ��ʧ��)
#endif
  SDIO_SetSDIOOperation(ENABLE); // ��ΪSDIOģʽ
  
  // ����Ҫ����CMD0, ��ΪSD I/O card�ĳ�ʼ��������CMD52
  // An I/O only card or the I/O portion of a combo card is NOT reset by CMD0. (See 4.4 Reset for SDIO)
  delay(10); // ��ʱ�ɷ�ֹCMD5�ط�
  
  /* ����CMD5: IO_SEND_OP_COND */
  sdio_cmd.SDIO_Argument = 0;
  sdio_cmd.SDIO_CmdIndex = 5;
  sdio_cmd.SDIO_CPSM = SDIO_CPSM_Enable;
  sdio_cmd.SDIO_Response = SDIO_Response_Short; // ���ն̻�Ӧ
  sdio_cmd.SDIO_Wait = SDIO_Wait_No;
  SDIO_SendCommand(&sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__func__);
  printf("RESPCMD%d, RESP1_%08x\n", SDIO_GetCommandResponse(), SDIO_GetResponse(SDIO_RESP1));
  
  /* ���ò���VDD Voltage Window: 3.2~3.4V, ���ٴη���CMD5 */
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
  
  /* ��ȡWiFiģ���ַ (CMD3: SEND_RELATIVE_ADDR, Ask the card to publish a new relative address (RCA)) */
  sdio_cmd.SDIO_Argument = 0;
  sdio_cmd.SDIO_CmdIndex = 3;
  SDIO_SendCommand(&sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__func__);
  sdio_rca = SDIO_GetResponse(SDIO_RESP1) >> 16;
  printf("Relative Card Address: 0x%04x\n", sdio_rca);
  
  /* ѡ��WiFiģ�� (CMD7: SELECT/DESELECT_CARD) */
  sdio_cmd.SDIO_Argument = sdio_rca << 16;
  sdio_cmd.SDIO_CmdIndex = 7;
  SDIO_SendCommand(&sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__func__);
  printf("Card selected! RESP1_%08x\n", SDIO_GetResponse(SDIO_RESP1));
  
  /* ���ʱ��Ƶ��, ���������ݳ�ʱʱ��Ϊ0.1s */
#ifdef WIFI_HIGHSPEED
  sdio.SDIO_ClockDiv = 0; // 48MHz/(0+2)=24MHz
  sdio_data.SDIO_DataTimeOut = 2400000;
  printf("SDIO Clock: 24MHz\n");
#else
  sdio.SDIO_ClockDiv = 46; // 48MHz/(46+2)=1MHz
  sdio_data.SDIO_DataTimeOut = 100000;
  printf("SDIO Clock: 1MHz\n");
#endif
  
  /* SDIO��������߿����Ϊ4λ */
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
  // ��count=512ʱ, ��0x1ff�����Ϊ0, ����SDIO��׼
  sdio_cmd.SDIO_Argument = (func << 28) | (addr << 9) | (count & 0x1ff) | flags;
  sdio_cmd.SDIO_CmdIndex = 53;
  sdio_cmd.SDIO_CPSM = SDIO_CPSM_Enable;
  sdio_cmd.SDIO_Response = SDIO_Response_Short;
  sdio_cmd.SDIO_Wait = SDIO_Wait_No;
  SDIO_SendCommand(&sdio_cmd);
}

/* ����WiFiģ�鹦���������ݿ��С */
void WiFi_LowLevel_SetBlockSize(uint8_t func, uint32_t size)
{
  sdio_block_size[func] = size;
  WiFi_LowLevel_WriteReg(0, (func << 8) | 0x10, size & 0xff);
  WiFi_LowLevel_WriteReg(0, (func << 8) | 0x11, size >> 8);
}

/* ����SDIO��������ݿ��С */
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

/* ���Flash�б���Ĺ̼������Ƿ����� */
#ifdef WIFI_FIRMWAREAREA_ADDR
static uint8_t WiFi_LowLevel_VerifyFirmware(void)
{
  uint32_t crc, len;
  if (WIFI_FIRMWARE_SIZE != 255536)
    return 0;
  
  len = WIFI_FIRMWARE_SIZE / 4 + 2; // �̼���(����CRC)�ܴ�С��1/4
  CRC_ResetDR();
  crc = CRC_CalcBlockCRC((uint32_t *)WIFI_FIRMWAREAREA_ADDR, len);
  return crc == 0;
}
#endif

/* �ȴ�SDIO�����Ӧ */
static void WiFi_LowLevel_WaitForResponse(const char *msg_title)
{
  uint8_t first = 1;
  do
  {
    if (!first)
      SDIO_SendCommand(&sdio_cmd); // �ط�����
    else
      first = 0;
    while (SDIO_GetFlagStatus(SDIO_FLAG_CMDACT) == SET); // �ȴ���������
    WiFi_LowLevel_CheckError(msg_title);
  } while (SDIO_GetFlagStatus(SDIO_FLAG_CMDREND) == RESET); // ���û���յ���Ӧ, ������
  SDIO_ClearFlag(SDIO_FLAG_CMDREND);
}

/* ��������, �Զ��жϲ������ִ���ģʽ */
// sizeΪҪ���͵��ֽ���, bufsizeΪdata�������Ĵ�С
uint8_t WiFi_LowLevel_WriteData(uint8_t func, uint32_t addr, const void *data, uint32_t size, uint32_t bufsize)
{
  uint16_t block_num; // ���ݿ����
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
    printf("WiFi_LowLevel_WriteData: a buffer of at least %d bytes is required! bufsize=%d\n", size, bufsize); // ֻ��������Խ�粻��Ӱ�����ݴ���, ������ֻ��һ������
  
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
  WiFi_LowLevel_WaitForResponse(__func__); // ����Ҫ�ȵ�CMD53�յ���Ӧ����ܿ�ʼ��������
  
  // ��ʼ��������
#ifdef WIFI_USEDMA
  dma.DMA_BufferSize = 0; // ��SDIO��������ʱ, ��ѡ����Ч
  dma.DMA_Channel = DMA_Channel_4;
  dma.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  dma.DMA_FIFOMode = DMA_FIFOMode_Enable; // ����ʹ��FIFOģʽ
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
  
  while (SDIO_GetFlagStatus(SDIO_FLAG_TXACT) == RESET); // �ȴ���ʼ��������
#ifdef WIFI_USEDMA
  while (DMA_GetFlagStatus(DMA2_Stream3, DMA_FLAG_TCIF3) == RESET) // �ȴ�DMA�����������
  {
    if (SDIO_GetFlagStatus(SDIO_FLAG_TXACT) == RESET || SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) == SET)
      break; // ������ִ���, ���˳�ѭ��
  }
  DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3 | DMA_FLAG_FEIF3); // ���DMA������ɱ�־λ, ����FIFO error����
  DMA_Cmd(DMA2_Stream3, DISABLE); // �ر�DMA
#else
  while (size && SDIO_GetFlagStatus(SDIO_FLAG_TXACT) == SET && SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) == RESET)
  {
    size -= 4;
    SDIO_WriteData(*p++); // ��FIFO����4�ֽ�����
    while (SDIO_GetFlagStatus(SDIO_FLAG_TXFIFOF) == SET); // ���FIFO������ȴ�
  }
#endif
  
  while (SDIO_GetFlagStatus(SDIO_FLAG_TXACT) == SET && SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) == RESET); // �ȴ��������
  sdio_data.SDIO_DPSM = SDIO_DPSM_Disable;
  SDIO_DataConfig(&sdio_data);
  
  // �����ر�־λ
  SDIO_ClearFlag(SDIO_FLAG_DATAEND | SDIO_FLAG_DBCKEND);
  return WiFi_LowLevel_CheckError(__func__) == 0;
}

/* д�Ĵ���, ����д���Ĵ�����ʵ������ */
uint8_t WiFi_LowLevel_WriteReg(uint8_t func, uint32_t addr, uint8_t value)
{
  WiFi_LowLevel_SendCMD52(func, addr, value, CMD52_WRITE | CMD52_READAFTERWRITE);
  WiFi_LowLevel_WaitForResponse(__func__);
  return SDIO_GetResponse(SDIO_RESP1) & 0xff;
}
