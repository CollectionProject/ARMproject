#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "WiFi.h"

static void WiFi_Associate_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_AssociateEx_Callback(void *arg, void *data, WiFi_Status status);
static uint8_t WiFi_CheckTxBufferTimeout(WiFi_TxBuffer *tbuf);
static void WiFi_Deauthenticate_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_DownloadFirmware(void);
static void WiFi_EventProcess(const WiFi_Event *event);
static void WiFi_GetMACAddress_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_JoinADHOC_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_JoinADHOCEx_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_PacketComplete(int8_t port, WiFi_Status status);
static void WiFi_PrintTxPacketStatus_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_TxBufferComplete(WiFi_TxBuffer *tbuf, void *data, WiFi_Status status);
static void WiFi_Scan_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_ScanSSID_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_SetWEPEx_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_ShowKeyMaterials_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_StartADHOCEx_Callback(void *arg, void *data, WiFi_Status status);
static void WiFi_StartMicroAP_Callback(void *arg, void *data, WiFi_Status status);

static const float wifi_ratelist[] = {1.0, 2.0, 5.5, 11.0, 0.0, 6.0, 9.0, 12.0, 18.0, 24.0, 36.0, 48.0, 54.0};
static uint8_t wifi_buffer_command[256]; // ����֡���ͻ�����
static uint8_t wifi_buffer_packet[1792]; // ����֡���ͻ�����
static uint8_t wifi_buffer_rx[2048]; // ֡���ջ�����
static uint32_t wifi_port;
static uint32_t wifi_input_time = 0; // ���һ�ε���SDIO�жϴ�������ʱ��
static WiFi_SSIDInfo wifi_ssid_info = {0}; // ��ǰ���ӵ��ȵ���Ϣ
static WiFi_TxBuffer wifi_tx_command = {0}; // ����֡���ͻ�����������
static WiFi_TxBuffer wifi_tx_packet[WIFI_DATAPORTS_NUM] = {0}; // ����֡���ͻ�����������

/* ����һ���ȵ� */
void WiFi_Associate(const char *ssid, WiFi_AuthenticationType auth_type, WiFi_Callback callback, void *arg)
{
  void **p;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  p = malloc(2 * sizeof(void *) + sizeof(WiFi_AuthenticationType)); // ���һ����Ա����ָ��, ����ʵ������
  if (p == NULL)
  {
    printf("WiFi_Associate: malloc failed!\n");
    if (callback)
      callback(arg, NULL, WIFI_STATUS_MEM);
    return;
  }
  p[0] = arg;
  p[1] = callback;
  *(WiFi_AuthenticationType *)(p + 2) = auth_type;
  
  WiFi_ScanSSID(ssid, &wifi_ssid_info, WiFi_Associate_Callback, p);
}

static void WiFi_Associate_Callback(void *arg, void *data, WiFi_Status status)
{
  void **p = (void **)arg; // ֮ǰ������ڴ�
  void *app_arg = p[0]; // �û�ָ���Ĳ���
  WiFi_Callback app_callback = (WiFi_Callback)p[1]; // �û�ָ���Ļص�����
  WiFi_AuthenticationType *pauth = (WiFi_AuthenticationType *)(p + 2);
  
  uint16_t cmd_size;
  WiFi_CmdRequest_Associate *cmd;
  WiFi_CmdResponse_Associate *resp;
  WiFi_SecurityType security;
  MrvlIETypes_PhyParamDSSet_t *ds;
  MrvlIETypes_CfParamSet_t *cf;
  MrvlIETypes_AuthType_t *auth;
  MrvlIETypes_VendorParamSet_t *vendor;
  MrvlIETypes_RsnParamSet_t *rsn;
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_Associate error!\n");
    free(arg);
    if (app_callback)
      app_callback(app_arg, data, status);
    return;
  }
  
  security = WiFi_GetSecurityType(&wifi_ssid_info);
  switch (WiFi_GetCommandCode(data))
  {
    case CMD_802_11_SCAN:
      // WiFi_ScanSSID����ִ�����
      cmd = (WiFi_CmdRequest_Associate *)wifi_buffer_command;
      memcpy(cmd->peer_sta_addr, wifi_ssid_info.mac_addr, sizeof(wifi_ssid_info.mac_addr));
      cmd->cap_info = wifi_ssid_info.cap_info;
      cmd->listen_interval = 10;
      cmd->bcn_period = wifi_ssid_info.bcn_period;
      cmd->dtim_period = 1;
      memcpy(cmd + 1, &wifi_ssid_info.ssid, TLV_STRUCTLEN(wifi_ssid_info.ssid));
      
      ds = (MrvlIETypes_PhyParamDSSet_t *)((uint8_t *)(cmd + 1) + TLV_STRUCTLEN(wifi_ssid_info.ssid));
      ds->header.type = WIFI_MRVLIETYPES_PHYPARAMDSSET;
      ds->header.length = 1;
      ds->channel = wifi_ssid_info.channel;
      
      cf = (MrvlIETypes_CfParamSet_t *)(ds + 1);
      memset(cf, 0, sizeof(MrvlIETypes_CfParamSet_t));
      cf->header.type = WIFI_MRVLIETYPES_CFPARAMSET;
      cf->header.length = TLV_PAYLOADLEN(*cf);
      
      memcpy(cf + 1, &wifi_ssid_info.rates, TLV_STRUCTLEN(wifi_ssid_info.rates));
      auth = (MrvlIETypes_AuthType_t *)((uint8_t *)(cf + 1) + TLV_STRUCTLEN(wifi_ssid_info.rates));
      auth->header.type = WIFI_MRVLIETYPES_AUTHTYPE;
      auth->header.length = TLV_PAYLOADLEN(*auth);
      auth->auth_type = *pauth;
      
      cmd_size = (uint8_t *)(auth + 1) - wifi_buffer_command;
      if (security == WIFI_SECURITYTYPE_WPA)
      {
        // WPA��������������м���Vendor�������ܳɹ�����
        vendor = (MrvlIETypes_VendorParamSet_t *)(auth + 1);
        memcpy(vendor, &wifi_ssid_info.wpa, TLV_STRUCTLEN(wifi_ssid_info.wpa));
        cmd_size += TLV_STRUCTLEN(wifi_ssid_info.wpa);
      }
      else if (security == WIFI_SECURITYTYPE_WPA2)
      {
        // WPA2��������������м���RSN�������ܳɹ�����
        rsn = (MrvlIETypes_RsnParamSet_t *)(auth + 1);
        memcpy(rsn, &wifi_ssid_info.rsn, TLV_STRUCTLEN(wifi_ssid_info.rsn));
        cmd_size += TLV_STRUCTLEN(wifi_ssid_info.rsn);
      }
      
      WiFi_SendCommand(CMD_802_11_ASSOCIATE, wifi_buffer_command, cmd_size, WIFI_BSS_STA, WiFi_Associate_Callback, arg, WIFI_TIMEOUT_SCAN, WIFI_DEFAULT_MAXRETRY);
      // ����arg�ڴ�, �ȹ����ɹ������ͷ�
      break;
    case CMD_802_11_ASSOCIATE:
      // ��������ִ����ϲ��յ��˻�Ӧ
      // ������Ҫ����Ƿ�����ɹ�
      free(arg);
      resp = (WiFi_CmdResponse_Associate *)data;
      //printf("capability=0x%04x, status_code=0x%04x, aid=0x%04x\n", resp->capability, resp->status_code, resp->association_id);
      if (app_callback)
      {
        if (resp->association_id == 0xffff)
          app_callback(app_arg, data, WIFI_STATUS_FAIL); // ����ʧ�� (�ڻص�������data�м��resp->capability��resp->status_code��ֵ�ɻ����ϸԭ��)
        else if (security == WIFI_SECURITYTYPE_WPA || security == WIFI_SECURITYTYPE_WPA2)
          app_callback(app_arg, data, WIFI_STATUS_INPROGRESS); // �ȴ���֤
        else
          app_callback(app_arg, data, WIFI_STATUS_OK); // �����ɹ�
      }
      break;
  }
}

/* ����һ���ȵ㲢�������� */
// ����WPA�͵��ȵ�ʱ, security��Աֱ�Ӹ�ֵWIFI_SECURITYTYPE_WPA����, ����Ҫ��ȷָ��WPA�汾��
void WiFi_AssociateEx(const WiFi_Connection *conn, WiFi_AuthenticationType auth_type, int8_t max_retry, WiFi_Callback callback, void *arg)
{
  int8_t *pmax_retry;
  uint16_t ssid_len;
  void **p;
  WiFi_AuthenticationType *pauth;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  ssid_len = strlen(conn->ssid);
  p = malloc(2 * sizeof(void *) + sizeof(WiFi_AuthenticationType) + sizeof(int8_t) + ssid_len + 1);
  if (p == NULL)
  {
    printf("WiFi_AssociateEx: malloc failed!\n");
    if (callback)
      callback(arg, NULL, WIFI_STATUS_MEM);
    return;
  }
  p[0] = arg;
  p[1] = callback;
  pauth = (WiFi_AuthenticationType *)(p + 2);
  *pauth = auth_type;
  pmax_retry = (int8_t *)(pauth + 1);
  *pmax_retry = max_retry; // ������������ӵĴ���, -1��ʾ���޴���, 0��ʾ������
  memcpy(pmax_retry + 1, conn->ssid, ssid_len + 1);
  
  if (conn->security == WIFI_SECURITYTYPE_WEP)
    WiFi_SetWEPEx(conn->password, WiFi_AssociateEx_Callback, p);
  else if (conn->security == WIFI_SECURITYTYPE_WPA || conn->security == WIFI_SECURITYTYPE_WPA2)
    WiFi_SetWPA(conn->password, WiFi_AssociateEx_Callback, p);
  else
    WiFi_MACControl(WIFI_MACCTRL_ETHERNET2 | WIFI_MACCTRL_TX | WIFI_MACCTRL_RX, WiFi_AssociateEx_Callback, p);
}

static void WiFi_AssociateEx_Callback(void *arg, void *data, WiFi_Status status)
{
  void **p = (void **)arg;
  void *app_arg = p[0];
  WiFi_Callback app_callback = (WiFi_Callback)p[1];
  WiFi_AuthenticationType *pauth = (WiFi_AuthenticationType *)(p + 2);
  int8_t *pmax_retry = (int8_t *)(pauth + 1);
  
  char *ssid = (char *)(pmax_retry + 1);
  uint16_t cmd_code = WiFi_GetCommandCode(data);
  
  if (cmd_code == CMD_802_11_ASSOCIATE || cmd_code == CMD_802_11_SCAN)
  {
    if (cmd_code == CMD_802_11_ASSOCIATE && (status == WIFI_STATUS_OK || status == WIFI_STATUS_INPROGRESS))
    {
      // �����ɹ�
      free(arg);
      if (app_callback)
        app_callback(app_arg, data, status);
      return;
    }
    else
    {
      // ����ʧ��, ����
      if (*pmax_retry != 0)
      {
        if (*pmax_retry != -1)
          (*pmax_retry)--;
        cmd_code = CMD_MAC_CONTROL;
        status = WIFI_STATUS_OK;
      }
    }
  }
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_AssociateEx error!\n");
    free(arg);
    if (app_callback)
      app_callback(app_arg, data, status);
    return;
  }
  
  switch (cmd_code)
  {
    case CMD_802_11_KEY_MATERIAL:
      WiFi_MACControl(WIFI_MACCTRL_ETHERNET2 | WIFI_MACCTRL_WEP | WIFI_MACCTRL_TX | WIFI_MACCTRL_RX, WiFi_AssociateEx_Callback, arg);
      break;
    case CMD_SUPPLICANT_PMK:
      WiFi_MACControl(WIFI_MACCTRL_ETHERNET2 | WIFI_MACCTRL_TX | WIFI_MACCTRL_RX, WiFi_AssociateEx_Callback, arg);
      break;
    case CMD_MAC_CONTROL:
      WiFi_Associate(ssid, *pauth, WiFi_AssociateEx_Callback, arg);
  }
}

/* ���֮ǰ��������δִ���������ִ���µ�����, ��ֱ�ӵ��ûص������������ */
uint8_t WiFi_CheckCommandBusy(WiFi_Callback callback, void *arg)
{
  // ����������ǰ����ȷ��֮ǰ�������Ѿ�������ϲ��յ���Ӧ
  // See 4.2 Protocol: The command exchange protocol is serialized, the host driver must wait until 
  // it has received a command response for the current command request before it can send the next command request.
  if (WiFi_IsCommandBusy())
  {
    printf("Warning: The previous command is in progress!\n");
    if (callback)
      callback(arg, NULL, WIFI_STATUS_BUSY);
    return 1;
  }
  return 0;
}

/* ����֡������֡���ͳ�ʱ���� */
void WiFi_CheckTimeout(void)
{
  int8_t i;
  uint8_t ret;
  uint32_t diff;
  WiFi_CommandHeader *cmd = (WiFi_CommandHeader *)wifi_buffer_command;
  
  // ��ʱ���INTSTATUS�Ĵ���, ������Ϊû�м�⵽SDIOIT�ж϶����³�����
  diff = sys_now() - wifi_input_time;
  if (diff > 2000)
    WiFi_Input();
  
  // �ص������е�data����: ʧ��ʱΪִ��ʧ�ܵ�����, �ɹ�ʱΪ�յ��Ļ�Ӧ
  ret = WiFi_CheckTxBufferTimeout(&wifi_tx_command);
  if (ret == 1)
  {
    printf("Command 0x%04x timeout!\n", cmd->cmd_code);
    WiFi_TxBufferComplete(&wifi_tx_command, wifi_buffer_command, WIFI_STATUS_NORESP);
  }
  else if (ret == 2)
  {
    WiFi_SendCommand(0, NULL, 0, 0, wifi_tx_command.callback, wifi_tx_command.arg, wifi_tx_command.timeout, wifi_tx_command.retry - 1);
    printf("Command 0x%04x timeout! Resend...\n", cmd->cmd_code);
  }
  
  // ��������֡ʱ, �ص������е�data���������˷���ʱʹ�õ�ͨ����
  for (i = 0; i < WIFI_DATAPORTS_NUM; i++)
  {
    ret = WiFi_CheckTxBufferTimeout(&wifi_tx_packet[i]);
    if (ret == 1)
      WiFi_PacketComplete(i + 1, WIFI_STATUS_NORESP); // ͨ���Ŵ�1��ʼ, �����±��0��ʼ
  }
}

/* ��鷢�ͻ������Ƿ�ʱ */
static uint8_t WiFi_CheckTxBufferTimeout(WiFi_TxBuffer *tbuf)
{
  uint32_t diff;
  if (tbuf->busy)
  {
    diff = sys_now() - tbuf->start_time;
    if (diff > tbuf->timeout) // ����ʱʱ�䵽��
    {
      if (tbuf->retry != 0)
      {
        tbuf->busy = 0; // busy�����ſ����ش�
        return 2; // ��ʱ, ��û����������Դ���
      }
      else
        return 1; // ��ʱ, �ҳ�����������Դ��� (busy������ſ��Ե���WiFi_TxBufferComplete)
    }
  }
  return 0; // û�г�ʱ
}

/* ��������WEP��Կ�Ƿ�Ϸ�, ����ʮ�������ַ�����Կת��Ϊ��������Կ */
// dest������\0����, ����ֵΪ��Կ����������
int8_t WiFi_CheckWEPKey(uint8_t *dest, const char *src)
{
  uint8_t i;
  uint16_t len = 0;
  uint32_t temp;
  if (src != NULL)
  {
    len = strlen(src);
    if (len == 5 || len == 13)
      memcpy(dest, src, len); // 5����13��ASCII��Կ�ַ�
    else if (len == 10 || len == 26)
    {
      // 10����26��16������Կ�ַ�
      for (i = 0; i < len; i += 2)
      {
        if (!isxdigit(src[i]) || !isxdigit(src[i + 1]))
          return -1; // ��Կ�к��зǷ��ַ�
        
        sscanf(src + i, "%02x", &temp);
        dest[i / 2] = temp;
      }
      len /= 2;
    }
    else
      len = 0; // ���Ȳ��Ϸ�
  }
  return len;
}

/* ���ȵ�Ͽ����� */
void WiFi_Deauthenticate(uint16_t reason, WiFi_Callback callback, void *arg)
{
  uint8_t ret;
  void **p;
  WiFi_Cmd_Deauthenticate *cmd = (WiFi_Cmd_Deauthenticate *)wifi_buffer_command;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  ret = WiFi_GetBSSID(cmd->peer_sta_addr);
  if (!ret)
  {
    printf("WiFi_Deauthenticate: WiFi is not connected!\n");
    if (callback)
      callback(arg, NULL, WIFI_STATUS_FAIL);
    return;
  }
  
  p = malloc(2 * sizeof(void *));
  if (p == NULL)
  {
    printf("WiFi_Deauthenticate: malloc failed!\n");
    if (callback)
      callback(arg, NULL, WIFI_STATUS_MEM);
    return;
  }
  p[0] = arg;
  p[1] = callback;
  
  cmd->reason_code = reason;
  WiFi_SendCommand(CMD_802_11_DEAUTHENTICATE, wifi_buffer_command, sizeof(WiFi_Cmd_Deauthenticate), WIFI_BSS_STA, WiFi_Deauthenticate_Callback, p, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);
}

static void WiFi_Deauthenticate_Callback(void *arg, void *data, WiFi_Status status)
{
  void **p = (void **)arg;
  void *app_arg = p[0];
  WiFi_Callback app_callback = (WiFi_Callback)p[1];
  free(arg);

  if (status == WIFI_STATUS_OK)
    memset(&wifi_ssid_info, 0, sizeof(wifi_ssid_info)); // ���SSID��Ϣ
  else
    printf("WiFi_Deauthenticate failed!\n");
  
  if (app_callback)
    app_callback(app_arg, data, status);
}

/* �̼����� */
static void WiFi_DownloadFirmware(void)
{
  uint8_t helper_buf[32];
  const uint8_t *data;
  uint16_t curr;
  uint32_t len;
  
  // ���ع̼�
  data = WIFI_FIRMWARE_ADDR; // �õ�ַ������4�ֽڶ����, ���򽫻����������
  len = WIFI_FIRMWARE_SIZE;
  while (len)
  {
    // ��ȡ����Ӧ���ص��ֽ���
    // ÿ�ο��Է���n>=curr�ֽڵ�����, ֻ����һ��CMD53�����, WiFiģ��ֻ��ǰcurr�ֽڵ�����
    if (len != WIFI_FIRMWARE_SIZE) // ��һ�η�������ǰ����Ҫ�ȴ�
      WiFi_Wait(WIFI_INTSTATUS_DNLD, 0); // ������ǰ����ȴ�Download Ready
    // 88W8801�������curr=0�����, �������ﲻ��Ҫwhile���
    curr = WiFi_LowLevel_ReadReg(1, WIFI_SQREADBASEADDR0) | (WiFi_LowLevel_ReadReg(1, WIFI_SQREADBASEADDR1) << 8);
    //printf("Required: %d bytes, Remaining: %d bytes\n", curr, len);
    
    if (curr & 1)
    {
      // ��currΪ����(��17), ��˵�����ն˳�����CRCУ�����, Ӧ���´�����һ�ε�����(�ⲿ�ִ���ʡ��)
      printf("Error: an odd size is invalid!\n");
      while (1);
    }
    if (curr > len)
      curr = len;
    
    // ���͹̼�����
    if (len < 32) // lenΪ������ʣ���С
    {
      // ���������ռ䲻��һ�����ݿ�, �����helper_buf
      memcpy(helper_buf, data, curr);
      WiFi_LowLevel_WriteData(1, wifi_port, helper_buf, curr, sizeof(helper_buf));
    }
    else
      WiFi_LowLevel_WriteData(1, wifi_port, data, curr, len);
    
    len -= curr;
    data += curr;
  }
  
  // �ȴ�Firmware����
  WiFi_Wait(WIFI_INTSTATUS_DNLD, 0);
  while (WiFi_GetFirmwareStatus() != 0xfedc);
  printf("Firmware is successfully downloaded!\n");
}

/* �����յ�����������һֱδ��������ݻ����� */
void WiFi_DiscardData(void)
{
  uint8_t i, ret;
  uint16_t len;
  for (i = 0; i < 16; i++)
  {
    len = WiFi_GetDataLength(i);
    if (len != 0)
    {
      ret = WiFi_LowLevel_ReadData(1, wifi_port + i, NULL, len, 0);
      if (ret)
        printf("Discarded %d bytes of data on port %d\n", len, i);
    }
  }
}

/* ����WiFi�¼� */
static void WiFi_EventProcess(const WiFi_Event *event)
{
  printf("[Event %d] size=%d, bss=0x%02x\n", event->event_id, event->header.length, WIFI_BSS(event->bss_type, event->bss_num));
  switch (event->event_id)
  {
    case 3:
    case 8:
    case 9:
      // WiFi����
      memset(&wifi_ssid_info, 0, sizeof(wifi_ssid_info)); // ���SSID��Ϣ
      break;
    case 43:
      // �ɹ������ȵ㲢���WPA/WPA2��֤
      // WEPģʽ��������ģʽ������֤, ֻҪ�����Ͼͻ�������¼�
      WiFi_AuthenticationCompleteHandler();
      break;
  }
}

/* ��ȡ���õ�ͨ���� */
int8_t WiFi_GetAvailablePort(uint16_t bitmap)
{
  int8_t i;
  for (i = 0; i < 16; i++)
  {
    if (bitmap & 1)
      return i;
    bitmap >>= 1;
  }
  return -1;
}

/* ��ȡ�����ȵ��MAC��ַ */
uint8_t WiFi_GetBSSID(uint8_t mac_addr[6])
{
  if (mac_addr != NULL)
    memcpy(mac_addr, wifi_ssid_info.mac_addr, 6);
  return !isempty(wifi_ssid_info.mac_addr, 6); // ����ֵ��ʾMAC��ַ�Ƿ�ȫΪ0
}

/* ��ȡ��Ҫ���յ����ݴ�С */
// ��port=0ʱ, ��ȡ�����յ������Ӧ֡���¼�֡�Ĵ�С
// ��port=1~15ʱ, ��ȡ�����յ�����֡�Ĵ�С, portΪ����ͨ����
uint16_t WiFi_GetDataLength(uint8_t port)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_RDLENP0L + 2 * port) | (WiFi_LowLevel_ReadReg(1, WIFI_RDLENP0U + 2 * port) << 8);
}

/* ��ȡ�̼�״̬ */
uint16_t WiFi_GetFirmwareStatus(void)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_SCRATCH0_0) | (WiFi_LowLevel_ReadReg(1, WIFI_SCRATCH0_1) << 8);
}

/* ��ȡMAC��ַ */
// callback����ΪNULL
void WiFi_GetMACAddress(WiFi_Callback callback, void *arg)
{
  void **p;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  p = malloc(2 * sizeof(void *));
  if (p == NULL)
  {
    printf("WiFi_GetMACAddress: malloc failed!\n");
    callback(arg, NULL, WIFI_STATUS_MEM);
    return;
  }
  p[0] = arg;
  p[1] = callback;
  
  WiFi_MACAddr(NULL, WIFI_ACT_GET, WiFi_GetMACAddress_Callback, p);
}

static void WiFi_GetMACAddress_Callback(void *arg, void *data, WiFi_Status status)
{
  void **p = (void **)arg;
  void *app_arg = p[0];
  WiFi_Callback app_callback = (WiFi_Callback)p[1];
  WiFi_Cmd_MACAddr *cmd = (WiFi_Cmd_MACAddr *)data;
  free(arg);
  
  if (status == WIFI_STATUS_OK)
    app_callback(app_arg, cmd->mac_addr, status);
  else
  {
    printf("WiFi_GetMACAddress error!\n");
    app_callback(app_arg, NULL, status);
  }
}

/* �������µ�����֡ */
uint8_t *WiFi_GetPacketBuffer(void)
{
  WiFi_DataTx *data = (WiFi_DataTx *)wifi_buffer_packet;
  return data->payload;
}

/* ��ȡ����ͨ����״̬ */
uint16_t WiFi_GetReadPortStatus(void)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_RDBITMAPL) | (WiFi_LowLevel_ReadReg(1, WIFI_RDBITMAPU) << 8);
}

/* ��ȡ�յ�������֡ */
const WiFi_DataRx *WiFi_GetReceivedPacket(void)
{
  WiFi_DataRx *data = (WiFi_DataRx *)wifi_buffer_rx;
  if (data->header.type == WIFI_SDIOFRAME_DATA)
    return data;
  else
    return NULL;
}

/* ��ȡ�ȵ����֤���� */
WiFi_SecurityType WiFi_GetSecurityType(const WiFi_SSIDInfo *info)
{
  if (info->cap_info & WIFI_CAPABILITY_PRIVACY)
  {
    if (info->rsn.header.type)
      return WIFI_SECURITYTYPE_WPA2;
    else if (info->wpa.header.type)
      return WIFI_SECURITYTYPE_WPA;
    else
      return WIFI_SECURITYTYPE_WEP;
  }
  else
    return WIFI_SECURITYTYPE_NONE;
}

/* ��ȡ����ͨ����״̬ */
uint16_t WiFi_GetWritePortStatus(void)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_WRBITMAPL) | (WiFi_LowLevel_ReadReg(1, WIFI_WRBITMAPU) << 8);
}

/* ��ʼ��WiFiģ�� */
void WiFi_Init(void)
{
  // ��ʼ���ײ�Ĵ���
  WiFi_LowLevel_Init();
  WiFi_ShowCIS();
  
  // ��ʼ��Function 1
  WiFi_LowLevel_WriteReg(0, SDIO_CCCR_IOEN, SDIO_CCCR_IOEN_IOE1); // IOE1=1 (Enable Function)
  while ((WiFi_LowLevel_ReadReg(0, SDIO_CCCR_IORDY) & SDIO_CCCR_IORDY_IOR1) == 0); // �ȴ�IOR1=1 (I/O Function Ready)
  WiFi_LowLevel_WriteReg(0, SDIO_CCCR_INTEN, SDIO_CCCR_INTEN_IENM | SDIO_CCCR_INTEN_IEN1); // ��SDIO�ж�����
  WiFi_LowLevel_WriteReg(1, WIFI_INTMASK, WIFI_INTMASK_HOSTINTMASK); // �����жϱ�־λ���ж��Ƿ�������Ҫ��ȡ, �ɿ��Ը���
  
  // ���ع̼�
  wifi_port = WiFi_LowLevel_ReadReg(1, WIFI_IOPORT0) | (WiFi_LowLevel_ReadReg(1, WIFI_IOPORT1) << 8) | (WiFi_LowLevel_ReadReg(1, WIFI_IOPORT2) << 16);
  WiFi_LowLevel_SetBlockSize(1, 32);
  WiFi_DownloadFirmware();
  WiFi_LowLevel_SetBlockSize(1, 256);
}

void WiFi_Input(void)
{
  int8_t port;
  uint8_t ret, status;
  uint16_t bitmap, len;
  WiFi_SDIOFrameHeader *rx_header = (WiFi_SDIOFrameHeader *)wifi_buffer_rx;
  WiFi_DataRx *rx_packet = (WiFi_DataRx *)wifi_buffer_rx;
  WiFi_CommandHeader *rx_cmd = (WiFi_CommandHeader *)wifi_buffer_rx;
  WiFi_CommandHeader *tx_cmd = (WiFi_CommandHeader *)wifi_buffer_command;
  
  wifi_input_time = sys_now();
  status = WiFi_LowLevel_ReadReg(1, WIFI_INTSTATUS); // ��ȡ��Ҫ������жϱ�־λ
  if (status == 0)
    return;
  WiFi_LowLevel_WriteReg(1, WIFI_INTSTATUS, WIFI_INTSTATUS_ALL & ~status); // �����������Щ��־λ, Ȼ���ٽ��д���, �������Ա������������������������ж�
  
  if (status & WIFI_INTSTATUS_DNLD)
  {
    // �������з��������ݵ���û���յ�ȷ�ϵ�ͨ��
    for (port = 1; port <= WIFI_DATAPORTS_NUM; port++) // ��88W8801��, ����ͨ���ͷ�ʱ��������ж�, ���Բ���Ҫ��ͨ��0
    {
      if (wifi_tx_packet[port - 1].busy)
      {
        bitmap = WiFi_GetWritePortStatus(); // ����֡�ص��������п��ܷ���������֡����ĳЩͨ���ٴα�ռ��, ����ÿ�ζ�Ҫ���¶��Ĵ���
        if (bitmap & _BV(port))
          WiFi_PacketComplete(port, WIFI_STATUS_OK); // ͨ������Ϊ����, ������ͨ���յ���ȷ��
      }
    }
  }
  
  if (status & WIFI_INTSTATUS_UPLD)
  {
    bitmap = WiFi_GetReadPortStatus();
    while ((port = WiFi_GetAvailablePort(bitmap)) != -1)
    {
      bitmap &= ~_BV(port);
      len = WiFi_GetDataLength(port);
      ret = WiFi_LowLevel_ReadData(1, wifi_port + port, wifi_buffer_rx, len, sizeof(wifi_buffer_rx));
      if (!ret)
        continue;
      
      switch (rx_header->type)
      {
        case WIFI_SDIOFRAME_DATA:
          // �յ���̫������֡
          if (rx_packet->rx_packet_type == 2) // Ethernet version 2 frame
            WiFi_PacketHandler(rx_packet, port);
          else
            printf("[Recv] type=%d\n", rx_packet->rx_packet_type);
          break;
        case WIFI_SDIOFRAME_COMMAND:
          // �յ������Ӧ֡
          if (rx_cmd->seq_num == tx_cmd->seq_num) // ������
          {
#ifdef WIFI_DISPLAY_RESPTIME
            printf("CMDRESP 0x%04x at %dms\n", rx_cmd->cmd_code, sys_now() - wifi_tx_command.start_time);
#endif
            if (rx_cmd->result == CMD_STATUS_SUCCESS)
              status = WIFI_STATUS_OK;
            else if (rx_cmd->result == CMD_STATUS_UNSUPPORTED)
            {
              status = WIFI_STATUS_UNSUPPORTED;
              printf("Command 0x%04x is unsupported!\n", tx_cmd->cmd_code);
            }
            else
              status = WIFI_STATUS_FAIL;
            WiFi_TxBufferComplete(&wifi_tx_command, wifi_buffer_rx, (WiFi_Status)status);
          }
          break;
        case WIFI_SDIOFRAME_EVENT:
          // �յ��¼�֡
          WiFi_EventProcess((WiFi_Event *)wifi_buffer_rx);
          WiFi_EventHandler((WiFi_Event *)wifi_buffer_rx); // �����¼�����ص�����
          break;
      }
    }
  }
}

/* ��������֡ǰ, ���뱣֤����ͻ�����Ϊ�� */
uint8_t WiFi_IsCommandBusy(void)
{
  return wifi_tx_command.busy;
}

/* ����Ad-Hoc���� */
void WiFi_JoinADHOC(const char *ssid, WiFi_Callback callback, void *arg)
{
  void **p;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  p = malloc(2 * sizeof(void *));
  if (p == NULL)
  {
    printf("WiFi_JoinADHOC: malloc failed!\n");
    if (callback)
      callback(arg, NULL, WIFI_STATUS_MEM);
    return;
  }
  p[0] = arg;
  p[1] = callback;
  
  WiFi_ScanSSID(ssid, &wifi_ssid_info, WiFi_JoinADHOC_Callback, p);
}

static void WiFi_JoinADHOC_Callback(void *arg, void *data, WiFi_Status status)
{
  void **p = (void **)arg;
  void *app_arg = p[0];
  WiFi_Callback app_callback = (WiFi_Callback)p[1];
  WiFi_Cmd_ADHOCJoin *cmd;
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_JoinADHOC error!\n");
    free(arg);
    if (app_callback)
      app_callback(app_arg, data, status);
    return;
  }
  
  switch (WiFi_GetCommandCode(data))
  {
    case CMD_802_11_SCAN:
      cmd = (WiFi_Cmd_ADHOCJoin *)wifi_buffer_command;
      memcpy(cmd->bssid, wifi_ssid_info.mac_addr, sizeof(cmd->bssid));
      strncpy((char *)cmd->ssid, (char *)wifi_ssid_info.ssid.ssid, sizeof(cmd->ssid)); // strncpy�Ὣδʹ�õ��������Ϊ0
      cmd->bss_type = WIFI_BSS_ANY; // recommended for use when joining Ad-Hoc networks
      cmd->bcn_period = wifi_ssid_info.bcn_period;
      cmd->dtim_period = 1;
      memset(cmd->timestamp, 0, sizeof(cmd->timestamp) + sizeof(cmd->start_ts));
      cmd->ds_param_set.header.type = WIFI_MRVLIETYPES_PHYPARAMDSSET;
      cmd->ds_param_set.header.length = TLV_PAYLOADLEN(cmd->ds_param_set);
      cmd->ds_param_set.channel = wifi_ssid_info.channel;
      cmd->reserved1 = 0;
      cmd->ibss_param_set.header.type = WIFI_MRVLIETYPES_IBSSPARAMSET;
      cmd->ibss_param_set.header.length = TLV_PAYLOADLEN(cmd->ibss_param_set);
      cmd->ibss_param_set.atim_window = 0;
      cmd->reserved2 = 0;
      cmd->cap_info = wifi_ssid_info.cap_info;
      memcpy(cmd->data_rates, wifi_ssid_info.rates.rates, sizeof(cmd->data_rates));
      cmd->reserved3 = 0;
      WiFi_SendCommand(CMD_802_11_AD_HOC_JOIN, wifi_buffer_command, sizeof(WiFi_Cmd_ADHOCJoin), WIFI_BSS_STA, WiFi_JoinADHOC_Callback, arg, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);
      break;
    case CMD_802_11_AD_HOC_JOIN:
      free(arg);
      cmd = (WiFi_Cmd_ADHOCJoin *)data;
      if (app_callback)
        app_callback(app_arg, data, status);
      break;
  }
}

/* ������������Ad-Hoc���� */
void WiFi_JoinADHOCEx(const WiFi_Connection *conn, int8_t max_retry, WiFi_Callback callback, void *arg)
{
  int8_t *pmax_retry;
  uint16_t ssid_len;
  void **p;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  if (conn->security == WIFI_SECURITYTYPE_WPA || conn->security == WIFI_SECURITYTYPE_WPA2)
  {
    // 88W8686��Ȼ�ܹ����ϵ��Դ�����WPA2��֤���͵�Ad-Hoc�ȵ�, Ҳ�����EAPOL��֤
    // ����ͬʱҲ���Լ�����һ��WEP�͵�ͬ��Ad-Hoc�ȵ�, ʹͨ���޷���������
    printf("WiFi_JoinADHOCEx: WPA is not supported!\n");
    if (callback)
      callback(arg, NULL, WIFI_STATUS_INVALID);
    return;
  }
  
  ssid_len = strlen(conn->ssid);
  p = malloc(2 * sizeof(void *) + sizeof(int8_t) + ssid_len + 1);
  if (p == NULL)
  {
    printf("WiFi_JoinADHOCEx: malloc failed!\n");
    if (callback)
      callback(arg, NULL, WIFI_STATUS_MEM);
    return;
  }
  p[0] = arg;
  p[1] = callback;
  pmax_retry = (int8_t *)(p + 2);
  *pmax_retry = max_retry;
  memcpy(pmax_retry + 1, conn->ssid, ssid_len + 1);
  
  if (conn->security == WIFI_SECURITYTYPE_WEP)
    WiFi_SetWEP(WIFI_ACT_ADD, conn->password, WiFi_JoinADHOCEx_Callback, p);
  else
    WiFi_MACControl(WIFI_MACCTRL_ETHERNET2 | WIFI_MACCTRL_TX | WIFI_MACCTRL_RX, WiFi_JoinADHOCEx_Callback, p);
}

static void WiFi_JoinADHOCEx_Callback(void *arg, void *data, WiFi_Status status)
{
  void **p = (void **)arg;
  void *app_arg = p[0];
  WiFi_Callback app_callback = (WiFi_Callback)p[1];
  int8_t *pmax_retry = (int8_t *)(p + 2);
  char *ssid = (char *)(pmax_retry + 1);
  uint16_t cmd_code = WiFi_GetCommandCode(data);
  
  if (cmd_code == CMD_802_11_AD_HOC_JOIN || cmd_code == CMD_802_11_SCAN)
  {
    if (cmd_code == CMD_802_11_AD_HOC_JOIN && status == WIFI_STATUS_OK)
    {
      free(arg);
      if (app_callback)
        app_callback(app_arg, data, status);
      return;
    }
    else
    {
      if (*pmax_retry != 0)
      {
        if (*pmax_retry != -1)
          (*pmax_retry)--;
        cmd_code = CMD_MAC_CONTROL;
        status = WIFI_STATUS_OK;
      }
    }
  }
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_JoinADHOCEx error!\n");
    free(arg);
    if (app_callback)
      app_callback(app_arg, data, status);
    return;
  }
  
  switch (cmd_code)
  {
    case CMD_802_11_SET_WEP:
      WiFi_MACControl(WIFI_MACCTRL_ETHERNET2 | WIFI_MACCTRL_WEP | WIFI_MACCTRL_TX | WIFI_MACCTRL_RX, WiFi_JoinADHOCEx_Callback, arg);
      break;
    case CMD_MAC_CONTROL:
      WiFi_JoinADHOC(ssid, WiFi_JoinADHOCEx_Callback, arg);
  }
}

/* ��ȡ������MAC��ַ */
void WiFi_MACAddr(const uint8_t newaddr[6], WiFi_CommandAction action, WiFi_Callback callback, void *arg)
{
  WiFi_Cmd_MACAddr *cmd = (WiFi_Cmd_MACAddr *)wifi_buffer_command;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  cmd->action = action;
  if (action == WIFI_ACT_SET)
    memcpy(cmd->mac_addr, newaddr, 6);
  else
    memset(cmd->mac_addr, 0, 6);
  WiFi_SendCommand(CMD_802_11_MAC_ADDR, wifi_buffer_command, sizeof(WiFi_Cmd_MACAddr), WIFI_BSS_STA, callback, arg, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);
}

/* ����MAC */
void WiFi_MACControl(uint16_t action, WiFi_Callback callback, void *arg)
{
  WiFi_Cmd_MACCtrl *cmd = (WiFi_Cmd_MACCtrl *)wifi_buffer_command;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  cmd->action = action;
  cmd->reserved = 0;
  WiFi_SendCommand(CMD_MAC_CONTROL, wifi_buffer_command, sizeof(WiFi_Cmd_MACCtrl), WIFI_BSS_STA, callback, arg, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);
}

/* �ͷ�����֡���ͻ����������ûص����� */
static void WiFi_PacketComplete(int8_t port, WiFi_Status status)
{
  WiFi_TxCallbackData callback_data;
#ifdef WIFI_DISPLAY_RESPTIME
  if (status == WIFI_STATUS_OK)
    printf("Packet on port %d ACK at %dms\n", port, sys_now() - wifi_tx_packet[port - 1].start_time);
  else if (status == WIFI_STATUS_NORESP)
    printf("Packet on port %d timeout!\n", port);
#endif
  callback_data.header.length = sizeof(callback_data);
  callback_data.header.type = WIFI_SDIOFRAME_DATA;
  callback_data.port = port;
  WiFi_TxBufferComplete(&wifi_tx_packet[port - 1], &callback_data, status);
}

/* ��ʾ�����б� */
void WiFi_PrintRates(uint16_t rates)
{
  // �����б��ڹ̼�API�ֲ��3.1 Receive Packet Descriptorһ�������RxRate�ֶ���������
  uint8_t i;
  uint8_t space = 0;
  for (i = 0; rates != 0; i++)
  {
    if (i == sizeof(wifi_ratelist) / sizeof(wifi_ratelist[0]))
      break;
    if (rates & 1)
    {
      if (space)
        printf(" ");
      else
        space = 1;
      printf("%.1fMbps", wifi_ratelist[i]);
    }
    rates >>= 1;
  }
}

/* ��ʾ����֡�ķ������ */
void WiFi_PrintTxPacketStatus(void)
{
  WiFi_SendCommand(CMD_TX_PKT_STATS, wifi_buffer_command, sizeof(WiFi_CommandHeader), WIFI_BSS_STA, WiFi_PrintTxPacketStatus_Callback, NULL, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);
}

static void WiFi_PrintTxPacketStatus_Callback(void *arg, void *data, WiFi_Status status)
{
  uint8_t i;
  uint8_t none = 1;
  WiFi_CommandHeader *header = (WiFi_CommandHeader *)data;
  WiFi_TxPktStatEntry *entries = (WiFi_TxPktStatEntry *)(header + 1);
  
  if (status == WIFI_STATUS_OK)
  {
    printf("[Tx packet status]\n");
    for (i = 0; i < sizeof(wifi_ratelist) / sizeof(wifi_ratelist[0]); i++)
    {
      if (wifi_ratelist[i] != 0.0 && !isempty(&entries[i], sizeof(WiFi_TxPktStatEntry)))
      {
        none = 0;
        printf("%.1fMbps: PktInitCnt=%d, PktSuccessCnt=%d, TxAttempts=%d\n", wifi_ratelist[i], entries[i].pkt_init_cnt, entries[i].pkt_success_cnt, entries[i].tx_attempts);
        printf("  RetryFailure=%d, ExpiryFailure=%d\n", entries[i].retry_failure, entries[i].expiry_failure);
      }
    }
    if (none)
      printf("(None)\n");
  }
  else
    printf("WiFi_PrintTxRates failed!\n");
}

/* ɨ��ȫ���ȵ� (����ʾ) */
void WiFi_Scan(WiFi_Callback callback, void *arg)
{
  uint8_t i;
  void **p;
  WiFi_CmdRequest_Scan *cmd = (WiFi_CmdRequest_Scan *)wifi_buffer_command; // Ҫ���͵�����
  MrvlIETypes_ChanListParamSet_t *chanlist = (MrvlIETypes_ChanListParamSet_t *)(cmd + 1); // �����+1ָ����ǰ��sizeof(ָ������)����ַ��Ԫ, ����ֻǰ��1����ַ��Ԫ
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  p = malloc(2 * sizeof(void *));
  if (p == NULL)
  {
    printf("WiFi_Scan: malloc failed!\n");
    if (callback)
      callback(arg, NULL, WIFI_STATUS_MEM);
    return;
  }
  p[0] = arg;
  p[1] = callback;
  
  cmd->bss_type = WIFI_BSS_ANY;
  memset(cmd->bss_id, 0, sizeof(cmd->bss_id));
  
  // ͨ���Ļ�������
  chanlist->header.type = WIFI_MRVLIETYPES_CHANLISTPARAMSET;
  chanlist->header.length = 4 * sizeof(chanlist->channels);
  for (i = 0; i < 4; i++) // ��ɨ��ǰ4��ͨ�� (i���±�, i+1����ͨ����)
  {
    chanlist->channels[i].band_config_type = 0; // 2.4GHz band, 20MHz channel width
    chanlist->channels[i].chan_number = i + 1; // ͨ����
    chanlist->channels[i].scan_type = 0;
    chanlist->channels[i].min_scan_time = 0;
    chanlist->channels[i].max_scan_time = 100;
  }
  
  WiFi_SendCommand(CMD_802_11_SCAN, wifi_buffer_command, sizeof(WiFi_CmdRequest_Scan) + TLV_STRUCTLEN(*chanlist), WIFI_BSS_STA, WiFi_Scan_Callback, p, WIFI_TIMEOUT_SCAN, WIFI_DEFAULT_MAXRETRY);
}

static void WiFi_Scan_Callback(void *arg, void *data, WiFi_Status status)
{
  void **p = (void **)arg;
  void *app_arg = p[0];
  WiFi_Callback app_callback = (WiFi_Callback)p[1];
  
  uint8_t i, j, n;
  WiFi_CmdRequest_Scan *cmd = (WiFi_CmdRequest_Scan *)wifi_buffer_command;
  MrvlIETypes_ChanListParamSet_t *chanlist = (MrvlIETypes_ChanListParamSet_t *)(cmd + 1);
  
  uint8_t ssid[33], channel;
  uint16_t ie_size;
  WiFi_CmdResponse_Scan *resp = (WiFi_CmdResponse_Scan *)data;
  WiFi_BssDescSet *bss_desc_set;
  WiFi_SecurityType security;
  WiFi_Vendor *vendor;
  IEEEType *ie_params;
  IEEEType *rates;
  //MrvlIETypes_TsfTimestamp_t *tft_table;
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_Scan error!\n");
    free(arg);
    if (app_callback)
      app_callback(app_arg, data, status);
    return;
  }
  
  // ����ɨ���������4��ͨ��������
  j = chanlist->channels[0].chan_number + 4;
  if (j < 14)
  {
    if (j == 13)
      n = 2;
    else
      n = 4;
    
    chanlist->header.length = n * sizeof(chanlist->channels);
    for (i = 0; i < n; i++)
      chanlist->channels[i].chan_number = i + j;
    WiFi_SendCommand(CMD_802_11_SCAN, wifi_buffer_command, sizeof(WiFi_CmdRequest_Scan) + TLV_STRUCTLEN(*chanlist), WIFI_BSS_STA, WiFi_Scan_Callback, arg, WIFI_TIMEOUT_SCAN, WIFI_DEFAULT_MAXRETRY);
  }
  else
    n = 0;
  
  // ��ʾ����ɨ����, num_of_setΪ�ȵ���
  if (resp->num_of_set > 0)
  {
    bss_desc_set = (WiFi_BssDescSet *)(resp + 1);
    for (i = 0; i < resp->num_of_set; i++)
    {
      security = WIFI_SECURITYTYPE_WEP;
      rates = NULL;
      
      ie_params = &bss_desc_set->ie_parameters;
      ie_size = bss_desc_set->ie_length - (sizeof(WiFi_BssDescSet) - sizeof(bss_desc_set->ie_length) - sizeof(bss_desc_set->ie_parameters));
      while (ie_size > 0)
      {
        switch (ie_params->header.type)
        {
          case WIFI_MRVLIETYPES_SSIDPARAMSET:
            // SSID����
            memcpy(ssid, ie_params->data, ie_params->header.length);
            ssid[ie_params->header.length] = '\0';
            break;
          case WIFI_MRVLIETYPES_RATESPARAMSET:
            // ����
            rates = ie_params;
            break;
          case WIFI_MRVLIETYPES_PHYPARAMDSSET:
            // ͨ����
            channel = ie_params->data[0];
            break;
          case WIFI_MRVLIETYPES_RSNPARAMSET:
            security = WIFI_SECURITYTYPE_WPA2;
            break;
          case WIFI_MRVLIETYPES_VENDORPARAMSET:
            if (security != WIFI_SECURITYTYPE_WPA2)
            {
              vendor = (WiFi_Vendor *)ie_params->data;
              if (vendor->oui[0] == 0x00 && vendor->oui[1] == 0x50 && vendor->oui[2] == 0xf2 && vendor->oui_type == 0x01)
                security = WIFI_SECURITYTYPE_WPA;
            }
            break;
        }
        ie_size -= TLV_STRUCTLEN(*ie_params);
        ie_params = (IEEEType *)TLV_NEXT(ie_params);
      }
      if ((bss_desc_set->cap_info & WIFI_CAPABILITY_PRIVACY) == 0)
        security = WIFI_SECURITYTYPE_NONE;
      
      printf("SSID '%s', ", ssid); // �ȵ�����
      printf("MAC %02X:%02X:%02X:%02X:%02X:%02X, ", bss_desc_set->bssid[0], bss_desc_set->bssid[1], bss_desc_set->bssid[2], bss_desc_set->bssid[3], bss_desc_set->bssid[4], bss_desc_set->bssid[5]); // MAC��ַ
      printf("RSSI %d, Channel %d\n", bss_desc_set->rssi, channel); // �ź�ǿ�Ⱥ�ͨ����
      //printf("  Timestamp %lld, Beacon Interval %d\n", bss_desc_set->pkt_time_stamp, bss_desc_set->bcn_interval);
      
      printf("  Capability: 0x%04x (Security: ", bss_desc_set->cap_info);
      switch (security)
      {
        case WIFI_SECURITYTYPE_NONE:
          printf("Unsecured");
          break;
        case WIFI_SECURITYTYPE_WEP:
          printf("WEP");
          break;
        case WIFI_SECURITYTYPE_WPA:
          printf("WPA");
          break;
        case WIFI_SECURITYTYPE_WPA2:
          printf("WPA2");
          break;
      }
      printf(", Mode: ");
      if (bss_desc_set->cap_info & WIFI_CAPABILITY_IBSS)
        printf("Ad-Hoc");
      else
        printf("Infrastructure");
      printf(")\n");
      
      if (rates != NULL)
      {
        // ��������ֵ�ı�ʾ����, �����802.11-2016.pdf��9.4.2.3 Supported Rates and BSS Membership Selectors elementһ�ڵ�����
        printf("  Rates:");
        for (j = 0; j < rates->header.length; j++)
          printf(" %.1fMbps", (rates->data[j] & 0x7f) * 0.5);
        printf("\n");
      }
      
      // ת����һ���ȵ���Ϣ
      bss_desc_set = (WiFi_BssDescSet *)((uint8_t *)bss_desc_set + sizeof(bss_desc_set->ie_length) + bss_desc_set->ie_length);
    }
    
    // resp->buf_size����bss_desc_set���ܴ�С
    // ���tft_table == buffer + sizeof(WiFi_CmdResponse_Scan) + resp->buf_size
    /*
    tft_table = (MrvlIETypes_TsfTimestamp_t *)bss_desc_set;
    if (tft_table->header.type == WIFI_MRVLIETYPES_TSFTIMESTAMP && tft_table->header.length == resp->num_of_set * sizeof(uint64_t))
    {
      printf("Timestamps: ");
      for (i = 0; i < resp->num_of_set; i++)
        printf("%lld ", tft_table->tsf_table[i]);
      printf("\n");
    }
    */
    
    // TSF timestamp table���������ݵ�ĩβ, ����û��Channel/band table
    //if (((uint8_t *)tft_table - (uint8_t *)data) + TLV_STRUCTLEN(*tft_table) == resp->header.frame_header.length)
    //  printf("data end!\n");
  }
  
  // ɨ�����ʱ���ûص�����
  if (n == 0)
  {
    free(arg);
    if (app_callback)
      app_callback(app_arg, data, status);
  }
}

/* ��ȡָ�����Ƶ��ȵ����ϸ��Ϣ */
void WiFi_ScanSSID(const char *ssid, WiFi_SSIDInfo *info, WiFi_Callback callback, void *arg)
{
  uint8_t i;
  void **p;
  MrvlIETypes_ChanListParamSet_t *chan_list;
  WiFi_CmdRequest_Scan *cmd = (WiFi_CmdRequest_Scan *)wifi_buffer_command;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  p = malloc(3 * sizeof(void *));
  if (p == NULL)
  {
    printf("WiFi_ScanSSID: malloc failed!\n");
    if (callback)
      callback(arg, NULL, WIFI_STATUS_MEM);
    return;
  }
  p[0] = arg;
  p[1] = callback;
  p[2] = info;
  memset(info, 0, sizeof(WiFi_SSIDInfo)); // ������info�ṹ������
  
  cmd->bss_type = WIFI_BSS_ANY;
  memset(cmd->bss_id, 0, sizeof(cmd->bss_id));
  
  // ��info->ssid��Ա��ֵ
  info->ssid.header.type = WIFI_MRVLIETYPES_SSIDPARAMSET;
  info->ssid.header.length = strlen(ssid);
  memcpy(info->ssid.ssid, ssid, info->ssid.header.length);
  memcpy(cmd + 1, &info->ssid, TLV_STRUCTLEN(info->ssid)); // ��info->ssid���Ƶ������͵�����������
  
  chan_list = (MrvlIETypes_ChanListParamSet_t *)((uint8_t *)(cmd + 1) + TLV_STRUCTLEN(info->ssid));
  chan_list->header.type = WIFI_MRVLIETYPES_CHANLISTPARAMSET;
  chan_list->header.length = 14 * sizeof(chan_list->channels); // һ����ɨ��14��ͨ��
  for (i = 0; i < 14; i++) // i���±�, i+1����ͨ����
  {
    chan_list->channels[i].band_config_type = 0;
    chan_list->channels[i].chan_number = i + 1;
    chan_list->channels[i].scan_type = 0;
    chan_list->channels[i].min_scan_time = 0;
    chan_list->channels[i].max_scan_time = 100;
  }
  
  WiFi_SendCommand(CMD_802_11_SCAN, wifi_buffer_command, ((uint8_t *)chan_list - wifi_buffer_command) + TLV_STRUCTLEN(*chan_list), WIFI_BSS_STA, WiFi_ScanSSID_Callback, p, WIFI_TIMEOUT_SCAN, WIFI_DEFAULT_MAXRETRY);
}

static void WiFi_ScanSSID_Callback(void *arg, void *data, WiFi_Status status)
{
  void **p = (void **)arg;
  void *app_arg = p[0];
  WiFi_Callback app_callback = (WiFi_Callback)p[1];
  WiFi_SSIDInfo *info = p[2];
  
  uint16_t ie_size;
  WiFi_CmdResponse_Scan *resp = (WiFi_CmdResponse_Scan *)data;
  WiFi_BssDescSet *bss_desc_set = (WiFi_BssDescSet *)(resp + 1);
  IEEEType *ie_params;
  WiFi_Vendor *vendor;
  
  free(arg);
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_ScanSSID error!\n");
    if (app_callback)
      app_callback(app_arg, data, status);
    return;
  }
  
  if (resp->num_of_set == 0)
  {
    // δ�ҵ�ָ����AP�ȵ�, ��ʱinfo�ṹ���г���ssid��Ա��, ����ĳ�Ա��Ϊ0
    // resp�е����ݵ���num_of_set��Ա���û����
    printf("SSID not found!\n");
    if (app_callback)
      app_callback(app_arg, data, WIFI_STATUS_NOTFOUND);
    return;
  }
  
  // bss_desc_set��ɨ�赽�ĵ�һ����Ϣ��Ϊ׼
  memcpy(info->mac_addr, bss_desc_set->bssid, sizeof(info->mac_addr));
  info->cap_info = bss_desc_set->cap_info;
  info->bcn_period = bss_desc_set->bcn_interval;
  
  // ��info->xxx.header.type=0, �����û�и������Ϣ (��SSID�ṹ����, ��ΪSSID��type=WIFI_MRVLIETYPES_SSIDPARAMSET=0)
  ie_params = &bss_desc_set->ie_parameters;
  ie_size = bss_desc_set->ie_length - (sizeof(WiFi_BssDescSet) - sizeof(bss_desc_set->ie_length) - sizeof(bss_desc_set->ie_parameters)); // ����IEEE_Type���ݵ��ܴ�С
  while (ie_size > 0)
  {
    switch (ie_params->header.type)
    {
      case WIFI_MRVLIETYPES_RATESPARAMSET:
        // ����
        WiFi_TranslateTLV((MrvlIEType *)&info->rates, ie_params, sizeof(info->rates.rates));
        break;
      case WIFI_MRVLIETYPES_PHYPARAMDSSET:
        // ͨ����
        info->channel = ie_params->data[0];
        break;
      case WIFI_MRVLIETYPES_RSNPARAMSET:
        // ͨ��ֻ��һ��RSN��Ϣ (��WPA2���)
        WiFi_TranslateTLV((MrvlIEType *)&info->rsn, ie_params, sizeof(info->rsn.rsn));
        break;
      case WIFI_MRVLIETYPES_VENDORPARAMSET:
        // ͨ�����ж���VENDOR��Ϣ (��WPA���)
        vendor = (WiFi_Vendor *)ie_params->data;
        if (vendor->oui[0] == 0x00 && vendor->oui[1] == 0x50 && vendor->oui[2] == 0xf2)
        {
          switch (vendor->oui_type)
          {
            case 0x01:
              // wpa_oui
              WiFi_TranslateTLV((MrvlIEType *)&info->wpa, ie_params, sizeof(info->wpa.vendor));
              break;
            case 0x02:
              // wmm_oui
              if (ie_params->header.length == 24) // �Ϸ���С
                WiFi_TranslateTLV((MrvlIEType *)&info->wwm, ie_params, sizeof(info->wwm.vendor));
              break;
            case 0x04:
              // wps_oui
              WiFi_TranslateTLV((MrvlIEType *)&info->wps, ie_params, sizeof(info->wps.vendor));
              break;
          }
        }
        break;
    }
    
    // ת����һ��TLV
    ie_size -= TLV_STRUCTLEN(*ie_params);
    ie_params = (IEEEType *)TLV_NEXT(ie_params);
  }
  
  if (app_callback)
    app_callback(app_arg, data, status);
}

/* ����WiFi����, �յ���Ӧ��ʱʱ����callback�ص����� */
// size=0��ʾdata������������������, ��code, size��bss��ֱ�Ӵ�data�л�ȡ(�����ط�)
// retry����Ϊ0(��һ��ʧ��ʱ��ֱ�ӵ��ûص�����, ��������), ��timeout����Ϊ0(�����յ���Ӧǰ������Ϊ��ʱ�����ûص�����)
//
// �޲���ϵͳ�Ļ�����ֻ��ʹ�÷�������ʽִ��WiFi����, ��ͨ���ص�����֪ͨ����ִ�еĽ�� (�ص�����Ӧ��֤�ܹ������ò�ֻ����һ��)
// ����в���ϵͳ, ĳ��������Ҫ��������ʽִ��WiFi����, �����ڸú���������ӷ�������ǰ�����ȴ���ʾ����ͨ���Ƿ���õ�0-1�ź����Ĵ���
// ������ͨ������ʱ, ʹ�ź�����ֵΪ1����������һ���ȴ��������������, �����������������ȴ���Ӧ, ���ûص���������������ִ�н��(�ɹ�����ʧ��)���������ķ���ֵ
void WiFi_SendCommand(uint16_t code, const void *data, uint16_t size, uint8_t bss, WiFi_Callback callback, void *arg, uint32_t timeout, uint8_t max_retry)
{
  static uint8_t seq_num = 0; // 88W8801���������к�Ϊ8λ
  WiFi_CommandHeader *cmdhdr = (WiFi_CommandHeader *)wifi_buffer_command;
  
  if (WiFi_CheckCommandBusy(callback, arg)) // ��������ǰ����ȷ��֮ǰ�������Ѿ��������
    return;
  
  // ֱ�ӷ�������: WiFi_SendCommand(0, data, 0, ...)
  // �������ͷ����������: WiFi_SendCommand(code, data, size, ...)
  // �����ϴ�����: WiFi_SendCommand(0, NULL, 0, ...)
  if (data != NULL && data != wifi_buffer_command)
    memmove(wifi_buffer_command, data, (size != 0) ? size : ((WiFi_SDIOFrameHeader *)data)->length); // ��Ҫ���͵��������ݸ��Ƶ���������, �Ա����ʱ�ط�
  
  //printf("seq_num=%d\n", seq_num);
  if (size != 0)
  {
    cmdhdr->frame_header.length = size;
    cmdhdr->frame_header.type = WIFI_SDIOFRAME_COMMAND;
    cmdhdr->cmd_code = code;
    cmdhdr->size = size - sizeof(WiFi_SDIOFrameHeader); // �����С��������ͷ��, ��������SDIO֡ͷ��
    cmdhdr->seq_num = seq_num++;
    cmdhdr->bss = bss; // 88W8801�����ֶ�
    cmdhdr->result = 0;
  }
  else
    size = cmdhdr->frame_header.length; // �ط�����ʱ����дcmdhdr
  WiFi_LowLevel_WriteData(1, wifi_port, wifi_buffer_command, size, sizeof(wifi_buffer_command));
  // WriteData��������ĸ��ʺ�С, ���������Ͳ�ȥ�ж����ķ���ֵ��
  // ��ʹ������(��CRCУ�����), �����ղ��������Ӧ, WiFi_CheckTimeout����Ҳ���ش�������
  
  wifi_tx_command.arg = arg;
  wifi_tx_command.busy = 1;
  wifi_tx_command.callback = callback;
  wifi_tx_command.retry = max_retry;
  wifi_tx_command.start_time = sys_now();
  wifi_tx_command.timeout = timeout;
}

/* ��������֡ */
// dataָ�����WiFi_DataTx.payload
// size=0��ʾ�ٴη�����һ֡, ��ʱdata������������
// ����ֵ��ʾ�����������õ�ͨ����
int8_t WiFi_SendPacket(void *data, uint16_t size, uint8_t bss, WiFi_Callback callback, void *arg, uint32_t timeout)
{
  static int8_t port = 0;
  int8_t newport;
  uint16_t bitmap;
  WiFi_DataTx *packet = (WiFi_DataTx *)wifi_buffer_packet;
  
  // ����������֡: WiFi_SendPacket(data, size, ...)
  // �ش�ǰһ����֡: WiFi_SendPacket(NULL, 0, ...)
  if (size == 0)
    data = packet->payload;
  
  do
  {
    // ����ͨ���ű�������ʹ��, ���ܿ�Խʹ��, ���򽫻ᵼ������֡����ʧ��
    // ����, ʹ����ͨ��4��, �´α���ʹ��ͨ��5, ����ʹ��ͨ��6
    if (port == WIFI_DATAPORTS_NUM)
      newport = 1;
    else
      newport = port + 1;
    
    // �ȴ�����ͨ������
    do
    {
      bitmap = WiFi_GetWritePortStatus();
    } while ((bitmap & _BV(newport)) == 0);
    
    // ͨ���ͷų�����, ���ø�ͨ����Ӧ�Ļص�����
    if (wifi_tx_packet[newport - 1].busy)
      WiFi_PacketComplete(newport, WIFI_STATUS_OK);
  } while (wifi_tx_packet[newport - 1].busy); // ����ڻص������з�����������֡, ����ݾ�̬����port����ֵ���¼���ͨ����
  // �ص������е�������֡ʹ������ͬ�Ķ˿ں�, ����һ���ܷ��ͳɹ�, �����������ٴε���WiFi_PacketComplete�γɶ��ݹ�
  port = newport;
  
  if (data != packet->payload)
    memmove(packet->payload, data, size); // ��Ҫ���͵��������ݸ��Ƶ���������
  
  if (size != 0)
  {
    // �йط������ݰ���ϸ��, ��ο�Firmware Specification PDF��Chapter 3: Data Path
    packet->header.length = sizeof(WiFi_DataTx) - sizeof(packet->payload) + size;
    packet->header.type = WIFI_SDIOFRAME_DATA;
    
    packet->bss_type = WIFI_BSSTYPE(bss);
    packet->bss_num = WIFI_BSSNUM(bss);
    packet->tx_packet_length = size;
    packet->tx_packet_offset = sizeof(WiFi_DataTx) - sizeof(packet->payload) - sizeof(packet->header); // ������SDIOFrameHeader
    packet->tx_packet_type = 0; // normal 802.3
    packet->tx_control = 0;
    packet->priority = 0;
    packet->flags = 0;
    packet->pkt_delay_2ms = 0;
    packet->reserved1 = 0;
    packet->tx_token_id = 0;
    packet->reserved2 = 0;
  }
  WiFi_LowLevel_WriteData(1, wifi_port + port, wifi_buffer_packet, packet->header.length, sizeof(wifi_buffer_packet));
  
  // ��������Ҫ�ȴ�Download Readyλ��1, ��������֡���ͳɹ�
  wifi_tx_packet[port - 1].arg = arg;
  wifi_tx_packet[port - 1].busy = 1;
  wifi_tx_packet[port - 1].callback = callback;
  wifi_tx_packet[port - 1].retry = 0;
  wifi_tx_packet[port - 1].start_time = sys_now();
  wifi_tx_packet[port - 1].timeout = timeout;
  return port;
}

/* ����WEP��Կ (���ȱ���Ϊ5��13���ַ�) */
// action: WIFI_ACT_ADD / WIFI_ACT_REMOVE, �Ƴ���Կʱ����key������ΪNULL
void WiFi_SetWEP(WiFi_CommandAction action, const WiFi_WEPKey *key, WiFi_Callback callback, void *arg)
{
  int16_t len;
  uint8_t i;
  uint16_t cmd_size;
  WiFi_Cmd_SetWEP *cmd = (WiFi_Cmd_SetWEP *)wifi_buffer_command;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  cmd->action = action;
  cmd->tx_key_index = key->index;
  if (action == WIFI_ACT_ADD)
  {
    memset(cmd->wep_types, 0, sizeof(cmd->wep_types) + sizeof(cmd->keys));
    for (i = 0; i < 4; i++)
    {
      if (key->keys[i] == NULL)
        continue;
      
      len = WiFi_CheckWEPKey(cmd->keys[i], key->keys[i]);
      switch (len)
      {
        case 5:
          cmd->wep_types[i] = WIFI_WEPKEYTYPE_40BIT;
          break;
        case 13:
          cmd->wep_types[i] = WIFI_WEPKEYTYPE_104BIT;
          break;
        case -1:
          printf("WiFi_SetWEP: The hex key %d contains invalid characters!\n", i);
          break;
        case 0:
          printf("WiFi_SetWEP: The length of key %d is invalid!\n", i);
          break;
      }
      
      if (len == -1 || len == 0)
      {
        if (callback)
          callback(arg, NULL, WIFI_STATUS_INVALID);
        return;
      }
    }
    cmd_size = sizeof(WiFi_Cmd_SetWEP);
  }
  else if (action == WIFI_ACT_REMOVE)
    cmd_size = cmd->wep_types - wifi_buffer_command;
  else
  {
    if (callback)
      callback(arg, NULL, WIFI_STATUS_INVALID);
    return;
  }
  WiFi_SendCommand(CMD_802_11_SET_WEP, wifi_buffer_command, cmd_size, WIFI_BSS_STA, callback, arg, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);
}

/* ʹ��CMD_802_11_KEY_MATERIAL��������WEP��Կ */
// ��֧�ֵ�����Կ
void WiFi_SetWEPEx(const WiFi_WEPKey *key, WiFi_Callback callback, void *arg)
{
  static int8_t last = -1;
  int8_t len;
  int8_t *plast;
  WiFi_Cmd_KeyMaterial *cmd = (WiFi_Cmd_KeyMaterial *)wifi_buffer_command;
  MrvlIETypes_KeyParamSet_v2_t *keyparamset = (MrvlIETypes_KeyParamSet_v2_t *)(cmd + 1); // ������Կʱ����ʹ��V2�汾��TLV����
  WiFi_KeyParam_WEP_v2 *keyparam = (WiFi_KeyParam_WEP_v2 *)(keyparamset + 1);
  WiFi_Callback *pcallback;
  
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  cmd->action = WIFI_ACT_SET;
  keyparamset->header.type = WIFI_MRVLIETYPES_KEYPARAMSETV2;
  keyparamset->header.length = TLV_PAYLOADLEN(*keyparamset);
  memset(keyparamset->mac_address, 0, sizeof(keyparamset->mac_address)); // ���MAC��ַ��Ϊ0, �����Ҫ�������ȵ�֮��������óɹ�
  keyparamset->key_idx = key->index;
  keyparamset->key_type = WIFI_KEYTYPE_WEP;
  keyparamset->key_info = WIFI_KEYINFO_DEFAULTKEY | WIFI_KEYINFO_KEYENABLED | WIFI_KEYTYPE_WEP;
  
  len = WiFi_CheckWEPKey(keyparam->wep_key, key->keys[key->index]);
  if (len == -1 || len == 0)
  {
    if (len == -1)
      printf("WiFi_SetWEPEx: The hex key contains invalid characters!\n");
    else
      printf("WiFi_SetWEPEx: The length of key is invalid!\n");
    if (callback)
      callback(arg, NULL, WIFI_STATUS_INVALID);
    return;
  }
  
  keyparam->key_len = len;
  keyparamset->header.length += sizeof(keyparam->key_len) + len;
  
  // ��ɾ����Կ����� (�ŵ�����ͻ�������ŵ�����ĩβ, ���ص�����ʹ��)
  plast = (int8_t *)TLV_NEXT(keyparamset);
  if (last != key->index)
  {
    *plast = last;
    last = key->index;
    
    if (*plast != -1)
    {
      pcallback = (WiFi_Callback *)(plast + 1);
      *pcallback = callback;
      callback = WiFi_SetWEPEx_Callback;
      
      *(void **)(pcallback + 1) = arg;
      arg = plast;
    }
  }
  
  WiFi_SendCommand(CMD_802_11_KEY_MATERIAL, wifi_buffer_command, (uint8_t *)plast - wifi_buffer_command, WIFI_BSS_STA, callback, arg, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);
}

static void WiFi_SetWEPEx_Callback(void *arg, void *data, WiFi_Status status)
{
  // �Ƴ�����Կ
  WiFi_Cmd_KeyMaterial *cmd = (WiFi_Cmd_KeyMaterial *)wifi_buffer_command;
  MrvlIETypes_KeyParamSet_v2_t *keyparamset = (MrvlIETypes_KeyParamSet_v2_t *)(cmd + 1);
  
  int8_t *plast = (int8_t *)arg;
  WiFi_Callback *pcallback = (WiFi_Callback *)(plast + 1);
  WiFi_Callback app_callback = *pcallback;
  void *app_arg = *(void **)(pcallback + 1);
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_SetWEPEx error!\n");
    if (app_callback)
      app_callback(app_arg, data, status);
    return;
  }
  
  if (*plast >= 10)
  {
    printf("Removed old WEP key %d\n", *plast - 10);
    if (app_callback)
      app_callback(app_arg, data, status);
    return;
  }
  
  cmd->action = WIFI_ACT_REMOVE;
  keyparamset->header.type  = WIFI_MRVLIETYPES_KEYPARAMSETV2;
  keyparamset->header.length = TLV_PAYLOADLEN(*keyparamset);
  memset(keyparamset->mac_address, 0, sizeof(keyparamset->mac_address));
  keyparamset->key_idx = *plast;
  keyparamset->key_type = WIFI_KEYTYPE_WEP;
  keyparamset->key_info = 0;
  
  *plast += 10;
  WiFi_SendCommand(CMD_802_11_KEY_MATERIAL, wifi_buffer_command, TLV_NEXT(keyparamset) - wifi_buffer_command, WIFI_BSS_STA, WiFi_SetWEPEx_Callback, arg, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);
}

/* ����WPA���� */
void WiFi_SetWPA(const char *password, WiFi_Callback callback, void *arg)
{
  WiFi_Cmd_SupplicantPMK *cmd = (WiFi_Cmd_SupplicantPMK *)wifi_buffer_command;
  MrvlIETypes_WPA_Passphrase_t *passphrase;
  
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  cmd->action = WIFI_ACT_SET;
  cmd->cache_result = 0; // �����Ӧ��, 0��ʾ�ɹ�, 1��ʾʧ��
  passphrase = (MrvlIETypes_WPA_Passphrase_t *)(cmd + 1);
  passphrase->header.type = WIFI_MRVLIETYPES_WPAPASSPHRASE;
  passphrase->header.length = strlen(password);
  strcpy((char *)passphrase->passphrase, password);
  
  WiFi_SendCommand(CMD_SUPPLICANT_PMK, cmd, TLV_NEXT(passphrase) - wifi_buffer_command, WIFI_BSS_STA, callback, arg, WIFI_TIMEOUT_SCAN, WIFI_DEFAULT_MAXRETRY);
}

/* ��ʾWiFiģ����Ϣ */
void WiFi_ShowCIS(void)
{
  uint8_t data[255];
  uint8_t func, i, n, len;
  uint8_t tpl_code, tpl_link; // 16.2 Basic Tuple Format and Tuple Chain Structure
  uint32_t cis_ptr;
  
  n = WiFi_LowLevel_GetFunctionNum();
  for (func = 0; func <= n; func++)
  {
    // ��ȡCIS�ĵ�ַ
    cis_ptr = (func << 8) | 0x9;
    cis_ptr  = WiFi_LowLevel_ReadReg(0, cis_ptr) | (WiFi_LowLevel_ReadReg(0, cis_ptr + 1) << 8) | (WiFi_LowLevel_ReadReg(0, cis_ptr + 2) << 16);
    printf("[CIS] func=%d, ptr=0x%08x\n", func, cis_ptr);
    
    // ����CIS, ֱ��β�ڵ�
    while ((tpl_code = WiFi_LowLevel_ReadReg(0, cis_ptr++)) != CISTPL_END)
    {
      if (tpl_code == CISTPL_NULL)
        continue;
      
      tpl_link = WiFi_LowLevel_ReadReg(0, cis_ptr++); // ��������ݵĴ�С
      for (i = 0; i < tpl_link; i++)
        data[i] = WiFi_LowLevel_ReadReg(0, cis_ptr + i);
      
      switch (tpl_code)
      {
        case CISTPL_VERS_1:
          printf("Product Information:");
          for (i = 2; data[i] != 0xff; i += len + 1)
          {
            // ���������ַ���
            len = strlen((char *)data + i);
            if (len != 0)
              printf(" %s", data + i);
          }
          printf("\n");
          break;
        case CISTPL_MANFID:
          // 16.6 CISTPL_MANFID: Manufacturer Identification String Tuple
          printf("Manufacturer Code: 0x%04x\n", *(uint16_t *)data); // TPLMID_MANF
          printf("Manufacturer Information: 0x%04x\n", *(uint16_t *)(data + 2)); // TPLMID_CARD
          break;
        case CISTPL_FUNCID:
          // 16.7.1 CISTPL_FUNCID: Function Identification Tuple
          printf("Card Function Code: 0x%02x\n", data[0]); // TPLFID_FUNCTION
          printf("System Initialization Bit Mask: 0x%02x\n", data[1]); // TPLFID_SYSINIT
          break;
        case CISTPL_FUNCE:
          // 16.7.2 CISTPL_FUNCE: Function Extension Tuple
          if (data[0] == 0)
          {
            // 16.7.3 CISTPL_FUNCE Tuple for Function 0 (Extended Data 00h)
            printf("Maximum Block Size: %d\n", *(uint16_t *)(data + 1));
            printf("Maximum Transfer Rate Code: 0x%02x\n", data[3]);
          }
          else
          {
            // 16.7.4 CISTPL_FUNCE Tuple for Function 1-7 (Extended Data 01h)
            printf("Maximum Block Size: %d\n", *(uint16_t *)(data + 0x0c)); // TPLFE_MAX_BLK_SIZE
          }
          break;
        default:
          printf("[CIS Tuple 0x%02x] addr=0x%08x size=%d\n", tpl_code, cis_ptr - 2, tpl_link);
          dump_data(data, tpl_link);
      }
      
      if (tpl_link == 0xff)
        break; // ��TPL_LINKΪ0xffʱ˵����ǰ���Ϊβ�ڵ�
      cis_ptr += tpl_link;
    }
  }
}

/* ��ʾ��Կ */
void WiFi_ShowKeyMaterials(void)
{
  WiFi_Cmd_KeyMaterial *cmd = (WiFi_Cmd_KeyMaterial *)wifi_buffer_command;
  MrvlIETypes_KeyParamSet_t *key = (MrvlIETypes_KeyParamSet_t *)(cmd + 1);
  if (WiFi_IsCommandBusy())
    return;
  
  cmd->action = WIFI_ACT_GET;
  key->header.type = WIFI_MRVLIETYPES_KEYPARAMSET;
  key->header.length = 6; // ���TLV����ֻ�ܰ������������ֶ�
  key->key_type_id = 0;
  key->key_info = WIFI_KEYINFO_UNICASTKEY | WIFI_KEYINFO_MULTICASTKEY; // ͬʱ��ȡPTK��GTK��Կ
  key->key_len = 0;
  
  WiFi_SendCommand(CMD_802_11_KEY_MATERIAL, wifi_buffer_command, key->key - wifi_buffer_command, WIFI_BSS_STA, WiFi_ShowKeyMaterials_Callback, NULL, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);

}

static void WiFi_ShowKeyMaterials_Callback(void *arg, void *data, WiFi_Status status)
{
  uint8_t keyparam_len;
  WiFi_Cmd_KeyMaterial *resp = (WiFi_Cmd_KeyMaterial *)data;
  MrvlIETypes_KeyParamSet_t *key = (MrvlIETypes_KeyParamSet_t *)(resp + 1);
  MrvlIETypes_StaMacAddr_t *mac;
  WiFi_KeyParam_WEP_v1 *keyparam;
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_ShowKeyMaterials failed!\n");
    return;
  }
  
  // ������л�ȡ������Կ
  // info=0x6Ϊ������Կ(PTK), info=0x5Ϊ�㲥��Կ(GTK)
  while ((uint8_t *)key < (uint8_t *)&resp->header + resp->header.size)
  {
    if (key->key_type_id == WIFI_KEYTYPE_WEP)
      printf("[WEP] ");
    else
    {
      if (key->key_info & WIFI_KEYINFO_UNICASTKEY)
        printf("[PTK] ");
      else if (key->key_info & WIFI_KEYINFO_MULTICASTKEY)
        printf("[GTK] ");
    }
    printf("type=%d, info=0x%x, len=%d", key->key_type_id, key->key_info, key->key_len);
    keyparam_len = key->header.length - (TLV_PAYLOADLEN(*key) - sizeof(key->key)); // key->key�������ʵ��С(�п��ܲ�����key->key_len)
    
    if (key->key_type_id == WIFI_KEYTYPE_WEP)
    {
      keyparam = (WiFi_KeyParam_WEP_v1 *)key->key;
      printf(", index=%d, is_default=%d\n", keyparam->wep_key_index, keyparam->is_default_tx_key);
      dump_data(keyparam->wep_key, key->key_len);
    }
    else
    {
      printf("\n");
      dump_data(key->key, keyparam_len);
    }
    
    // ת����һ����Կ+MAC��ַ���
    mac = (MrvlIETypes_StaMacAddr_t *)TLV_NEXT(key);
    key = (MrvlIETypes_KeyParamSet_t *)TLV_NEXT(mac);
  }
  
  if (key == (MrvlIETypes_KeyParamSet_t *)(resp + 1))
    printf("No key materials!\n");
}

/* ����һ��Ad-Hoc�͵�WiFi�ȵ� */
// ��������WEP������ȵ�ʱ, cap_infoΪWIFI_CAPABILITY_PRIVACY
// ������������ȵ�ʱ, cap_infoΪ0
void WiFi_StartADHOC(const char *ssid, uint16_t cap_info, WiFi_Callback callback, void *arg)
{
  WiFi_Cmd_ADHOCStart *cmd = (WiFi_Cmd_ADHOCStart *)wifi_buffer_command;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  strncpy((char *)cmd->ssid, ssid, sizeof(cmd->ssid));
  cmd->bss_type = WIFI_BSS_INDEPENDENT;
  cmd->bcn_period = 100;
  cmd->reserved1 = 0;
  cmd->ibss_param_set.header.type = WIFI_MRVLIETYPES_IBSSPARAMSET;
  cmd->ibss_param_set.header.length = TLV_PAYLOADLEN(cmd->ibss_param_set);
  cmd->ibss_param_set.atim_window = 0;
  cmd->reserved2 = 0;
  cmd->ds_param_set.header.type = WIFI_MRVLIETYPES_PHYPARAMDSSET;
  cmd->ds_param_set.header.length = TLV_PAYLOADLEN(cmd->ds_param_set);
  cmd->ds_param_set.channel = 1;
  memset(cmd->reserved3, 0, sizeof(cmd->reserved3));
  cmd->cap_info = WIFI_CAPABILITY_IBSS | cap_info;
  memset(cmd->data_rate, 0, sizeof(cmd->data_rate));
  cmd->data_rate[0] = 0x82; // 1.0Mbps
  cmd->data_rate[1] = 0x84; // 2.0Mbps
  cmd->data_rate[2] = 0x8b; // 5.5Mbps
  cmd->data_rate[3] = 0x96; // 11.0Mbps
  WiFi_SendCommand(CMD_802_11_AD_HOC_START, wifi_buffer_command, sizeof(WiFi_Cmd_ADHOCStart), WIFI_BSS_STA, callback, arg, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);
}

/* ����һ��Ad-Hoc�͵�WiFi�ȵ㲢���ú����� */
void WiFi_StartADHOCEx(const WiFi_Connection *conn, WiFi_Callback callback, void *arg)
{
  uint16_t ssid_len;
  void **p;
  WiFi_SecurityType *psecurity;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  if (conn->security == WIFI_SECURITYTYPE_WPA || conn->security == WIFI_SECURITYTYPE_WPA2)
  {
    printf("WiFi_StartADHOCEx: WPA is not supported!\n");
    if (callback)
      callback(arg, NULL, WIFI_STATUS_INVALID);
    return;
  }
  
  ssid_len = strlen(conn->ssid);
  p = malloc(2 * sizeof(void *) + sizeof(WiFi_SecurityType) + ssid_len + 1);
  if (p == NULL)
  {
    printf("WiFi_StartADHOCEx: malloc failed!\n");
    if (callback)
      callback(arg, NULL, WIFI_STATUS_MEM);
    return;
  }
  p[0] = arg;
  p[1] = callback;
  psecurity = (WiFi_SecurityType *)(p + 2);
  *psecurity = conn->security;
  memcpy(psecurity + 1, conn->ssid, ssid_len + 1);
  
  if (conn->security == WIFI_SECURITYTYPE_WEP)
    WiFi_SetWEP(WIFI_ACT_ADD, conn->password, WiFi_StartADHOCEx_Callback, p);
  else
    WiFi_MACControl(WIFI_MACCTRL_ETHERNET2 | WIFI_MACCTRL_TX | WIFI_MACCTRL_RX, WiFi_StartADHOCEx_Callback, p);
}

static void WiFi_StartADHOCEx_Callback(void *arg, void *data, WiFi_Status status)
{
  void **p = (void **)arg;
  void *app_arg = p[0];
  WiFi_Callback app_callback = (WiFi_Callback)p[1];
  WiFi_SecurityType *psecurity = (WiFi_SecurityType *)(p + 2);
  char *ssid = (char *)(psecurity + 1);
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_StartADHOCEx error!\n");
    free(arg);
    if (app_callback)
      app_callback(app_arg, data, status);
    return;
  }
  
  switch (WiFi_GetCommandCode(data))
  {
    case CMD_802_11_SET_WEP:
      WiFi_MACControl(WIFI_MACCTRL_ETHERNET2 | WIFI_MACCTRL_WEP | WIFI_MACCTRL_TX | WIFI_MACCTRL_RX, WiFi_StartADHOCEx_Callback, arg);
      break;
    case CMD_MAC_CONTROL:
      if (*psecurity == WIFI_SECURITYTYPE_NONE)
        WiFi_StartADHOC(ssid, 0, WiFi_StartADHOCEx_Callback, arg);
      else
        WiFi_StartADHOC(ssid, WIFI_CAPABILITY_PRIVACY, WiFi_StartADHOCEx_Callback, arg);
      break;
    case CMD_802_11_AD_HOC_START:
      free(arg);
      if (app_callback)
        app_callback(app_arg, data, status);
      break;
  }
}

/* ����΢��AP�ȵ� */
void WiFi_StartMicroAP(const WiFi_Connection *conn, WiFi_AuthenticationType auth_type, WiFi_Callback callback, void *arg)
{
  WiFi_Cmd_SysConfigure *cmd = (WiFi_Cmd_SysConfigure *)wifi_buffer_command;
  MrvlIETypes_SSIDParamSet_t *ssid;
  MrvlIETypes_AuthType_t *authtype;
  void **pp;
  
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  cmd->action = WIFI_ACT_SET;
  
  if (conn->ssid != NULL)
  {
    ssid = (MrvlIETypes_SSIDParamSet_t *)(cmd + 1);
    ssid->header.type = WIFI_MRVLIETYPES_SSIDPARAMSET;
    ssid->header.length = strlen(conn->ssid);
    if (ssid->header.length > sizeof(ssid->ssid))
      ssid->header.length = sizeof(ssid->ssid);
    memcpy(ssid->ssid, conn->ssid, ssid->header.length);
    
    authtype = (MrvlIETypes_AuthType_t *)TLV_NEXT(ssid);
  }
  else
    authtype = (MrvlIETypes_AuthType_t *)(cmd + 1);
  
  authtype->header.type = WIFI_MRVLIETYPES_AUTHTYPE;
  authtype->header.length = TLV_PAYLOADLEN(*authtype);
  authtype->auth_type = auth_type;
  
  pp = (void **)TLV_NEXT(authtype);
  pp[0] = callback;
  pp[1] = arg;
  
  WiFi_SendCommand(APCMD_SYS_CONFIGURE, wifi_buffer_command, (uint8_t *)pp - wifi_buffer_command, WIFI_BSS_UAP, WiFi_StartMicroAP_Callback, pp, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);
}

static void WiFi_StartMicroAP_Callback(void *arg, void *data, WiFi_Status status)
{
  uint16_t code = WiFi_GetCommandCode(data);
  void **pp = arg;
  WiFi_Callback app_callback = (WiFi_Callback)pp[0];
  void *app_arg = pp[1];
  
  if (status != WIFI_STATUS_OK)
  {
    printf("WiFi_StartMicroAP failed! cmd=0x%04x\n", code);
    if (app_callback)
      app_callback(app_arg, data, status);
    return;
  }
  
  switch (code)
  {
    case APCMD_SYS_CONFIGURE:
      WiFi_SendCommand(APCMD_BSS_START, wifi_buffer_command, sizeof(WiFi_CommandHeader), WIFI_BSS_UAP, WiFi_StartMicroAP_Callback, arg, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);
      break;
    case APCMD_BSS_START:
      if (app_callback)
        app_callback(app_arg, data, status);
      break;
  }
}

void WiFi_StopADHOC(WiFi_Callback callback, void *arg)
{
  WiFi_SendCommand(CMD_802_11_AD_HOC_STOP, wifi_buffer_command, sizeof(WiFi_CommandHeader), WIFI_BSS_STA, callback, arg, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);
}

/* �ͷŷ��ͻ����������ûص����� */
static void WiFi_TxBufferComplete(WiFi_TxBuffer *tbuf, void *data, WiFi_Status status)
{
  if (tbuf->busy)
  {
    tbuf->busy = 0;
    if (tbuf->callback)
      tbuf->callback(tbuf->arg, data, status); // ���ûص�����
  }
}

/* ��IEEE�͵�TLVת����MrvlIE�͵�TLV */
uint8_t WiFi_TranslateTLV(MrvlIEType *mrvlie_tlv, const IEEEType *ieee_tlv, uint16_t mrvlie_payload_size)
{
  mrvlie_tlv->header.type = ieee_tlv->header.type;
  if (ieee_tlv->header.length > mrvlie_payload_size)
    mrvlie_tlv->header.length = mrvlie_payload_size; // ��Դ���ݴ�С�����������������, ����ʣ������
  else
    mrvlie_tlv->header.length = ieee_tlv->header.length;
  memset(mrvlie_tlv->data, 0, mrvlie_payload_size); // ����
  memcpy(mrvlie_tlv->data, ieee_tlv->data, mrvlie_tlv->header.length); // ��������
  return mrvlie_tlv->header.length == ieee_tlv->header.length; // ����ֵ��ʾ��������С�Ƿ��㹻
}

/* �ڹ涨�ĳ�ʱʱ����, �ȴ�ָ���Ŀ�״̬λ��λ, �������Ӧ���жϱ�־λ */
// �ɹ�ʱ����1
uint8_t WiFi_Wait(uint8_t status, uint32_t timeout)
{
  uint32_t diff;
  uint32_t start = sys_now();
  while ((WiFi_LowLevel_ReadReg(1, WIFI_INTSTATUS) & status) != status)
  {
    diff = sys_now() - start;
    if (timeout != 0 && diff > timeout)
    {
      // ����ʱʱ���ѵ�
      printf("WiFi_Wait(0x%02x): timeout!\n", status);
      return 0;
    }
  }
  
  // �����Ӧ���жϱ�־λ
  WiFi_LowLevel_WriteReg(1, WIFI_INTSTATUS, WIFI_INTSTATUS_ALL & ~status); // ����Ҫ�����λ����Ϊ1
  // ���ܽ�SDIOITλ�����! �����п��ܵ��¸�λ��Զ������λ
  return 1;
}
