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
static uint8_t wifi_buffer_command[256]; // 命令帧发送缓冲区
static uint8_t wifi_buffer_packet[1792]; // 数据帧发送缓冲区
static uint8_t wifi_buffer_rx[2048]; // 帧接收缓冲区
static uint32_t wifi_port;
static uint32_t wifi_input_time = 0; // 最后一次调用SDIO中断处理函数的时间
static WiFi_SSIDInfo wifi_ssid_info = {0}; // 当前连接的热点信息
static WiFi_TxBuffer wifi_tx_command = {0}; // 命令帧发送缓冲区描述符
static WiFi_TxBuffer wifi_tx_packet[WIFI_DATAPORTS_NUM] = {0}; // 数据帧发送缓冲区描述符

/* 关联一个热点 */
void WiFi_Associate(const char *ssid, WiFi_AuthenticationType auth_type, WiFi_Callback callback, void *arg)
{
  void **p;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  p = malloc(2 * sizeof(void *) + sizeof(WiFi_AuthenticationType)); // 最后一个成员不是指针, 而是实际数据
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
  void **p = (void **)arg; // 之前分配的内存
  void *app_arg = p[0]; // 用户指定的参数
  WiFi_Callback app_callback = (WiFi_Callback)p[1]; // 用户指定的回调函数
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
      // WiFi_ScanSSID命令执行完毕
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
        // WPA网络必须在命令中加入Vendor参数才能成功连接
        vendor = (MrvlIETypes_VendorParamSet_t *)(auth + 1);
        memcpy(vendor, &wifi_ssid_info.wpa, TLV_STRUCTLEN(wifi_ssid_info.wpa));
        cmd_size += TLV_STRUCTLEN(wifi_ssid_info.wpa);
      }
      else if (security == WIFI_SECURITYTYPE_WPA2)
      {
        // WPA2网络必须在命令中加入RSN参数才能成功连接
        rsn = (MrvlIETypes_RsnParamSet_t *)(auth + 1);
        memcpy(rsn, &wifi_ssid_info.rsn, TLV_STRUCTLEN(wifi_ssid_info.rsn));
        cmd_size += TLV_STRUCTLEN(wifi_ssid_info.rsn);
      }
      
      WiFi_SendCommand(CMD_802_11_ASSOCIATE, wifi_buffer_command, cmd_size, WIFI_BSS_STA, WiFi_Associate_Callback, arg, WIFI_TIMEOUT_SCAN, WIFI_DEFAULT_MAXRETRY);
      // 保留arg内存, 等关联成功后再释放
      break;
    case CMD_802_11_ASSOCIATE:
      // 关联命令执行完毕并收到了回应
      // 现在需要检查是否关联成功
      free(arg);
      resp = (WiFi_CmdResponse_Associate *)data;
      //printf("capability=0x%04x, status_code=0x%04x, aid=0x%04x\n", resp->capability, resp->status_code, resp->association_id);
      if (app_callback)
      {
        if (resp->association_id == 0xffff)
          app_callback(app_arg, data, WIFI_STATUS_FAIL); // 关联失败 (在回调函数的data中检查resp->capability和resp->status_code的值可获得详细原因)
        else if (security == WIFI_SECURITYTYPE_WPA || security == WIFI_SECURITYTYPE_WPA2)
          app_callback(app_arg, data, WIFI_STATUS_INPROGRESS); // 等待认证
        else
          app_callback(app_arg, data, WIFI_STATUS_OK); // 关联成功
      }
      break;
  }
}

/* 关联一个热点并输入密码 */
// 连接WPA型的热点时, security成员直接赋值WIFI_SECURITYTYPE_WPA即可, 不需要明确指出WPA版本号
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
  *pmax_retry = max_retry; // 最大尝试重新连接的次数, -1表示无限次数, 0表示不重试
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
      // 关联成功
      free(arg);
      if (app_callback)
        app_callback(app_arg, data, status);
      return;
    }
    else
    {
      // 关联失败, 重试
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

/* 如果之前的命令尚未执行完就请求执行新的命令, 则直接调用回调函数报告错误 */
uint8_t WiFi_CheckCommandBusy(WiFi_Callback callback, void *arg)
{
  // 发送新命令前必须确保之前的命令已经发送完毕并收到回应
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

/* 数据帧、命令帧发送超时处理 */
void WiFi_CheckTimeout(void)
{
  int8_t i;
  uint8_t ret;
  uint32_t diff;
  WiFi_CommandHeader *cmd = (WiFi_CommandHeader *)wifi_buffer_command;
  
  // 定时检查INTSTATUS寄存器, 避免因为没有检测到SDIOIT中断而导致程序卡死
  diff = sys_now() - wifi_input_time;
  if (diff > 2000)
    WiFi_Input();
  
  // 回调函数中的data参数: 失败时为执行失败的命令, 成功时为收到的回应
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
  
  // 发送数据帧时, 回调函数中的data参数保存了发送时使用的通道号
  for (i = 0; i < WIFI_DATAPORTS_NUM; i++)
  {
    ret = WiFi_CheckTxBufferTimeout(&wifi_tx_packet[i]);
    if (ret == 1)
      WiFi_PacketComplete(i + 1, WIFI_STATUS_NORESP); // 通道号从1开始, 数组下标从0开始
  }
}

/* 检查发送缓冲区是否超时 */
static uint8_t WiFi_CheckTxBufferTimeout(WiFi_TxBuffer *tbuf)
{
  uint32_t diff;
  if (tbuf->busy)
  {
    diff = sys_now() - tbuf->start_time;
    if (diff > tbuf->timeout) // 若超时时间到了
    {
      if (tbuf->retry != 0)
      {
        tbuf->busy = 0; // busy清零后才可以重传
        return 2; // 超时, 且没超过最大重试次数
      }
      else
        return 1; // 超时, 且超过了最大重试次数 (busy不清零才可以调用WiFi_TxBufferComplete)
    }
  }
  return 0; // 没有超时
}

/* 检查输入的WEP密钥是否合法, 并将十六进制字符串密钥转换为二进制密钥 */
// dest不会以\0结束, 返回值为密钥的真正长度
int8_t WiFi_CheckWEPKey(uint8_t *dest, const char *src)
{
  uint8_t i;
  uint16_t len = 0;
  uint32_t temp;
  if (src != NULL)
  {
    len = strlen(src);
    if (len == 5 || len == 13)
      memcpy(dest, src, len); // 5个或13个ASCII密钥字符
    else if (len == 10 || len == 26)
    {
      // 10个或26个16进制密钥字符
      for (i = 0; i < len; i += 2)
      {
        if (!isxdigit(src[i]) || !isxdigit(src[i + 1]))
          return -1; // 密钥中含有非法字符
        
        sscanf(src + i, "%02x", &temp);
        dest[i / 2] = temp;
      }
      len /= 2;
    }
    else
      len = 0; // 长度不合法
  }
  return len;
}

/* 与热点断开连接 */
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
    memset(&wifi_ssid_info, 0, sizeof(wifi_ssid_info)); // 清空SSID信息
  else
    printf("WiFi_Deauthenticate failed!\n");
  
  if (app_callback)
    app_callback(app_arg, data, status);
}

/* 固件下载 */
static void WiFi_DownloadFirmware(void)
{
  uint8_t helper_buf[32];
  const uint8_t *data;
  uint16_t curr;
  uint32_t len;
  
  // 下载固件
  data = WIFI_FIRMWARE_ADDR; // 该地址必须是4字节对齐的, 否则将会引起传输错误
  len = WIFI_FIRMWARE_SIZE;
  while (len)
  {
    // 获取本次应下载的字节数
    // 每次可以发送n>=curr字节的数据, 只能用一个CMD53命令发送, WiFi模块只认前curr字节的数据
    if (len != WIFI_FIRMWARE_SIZE) // 第一次发送数据前不需要等待
      WiFi_Wait(WIFI_INTSTATUS_DNLD, 0); // 否则发送前必须等待Download Ready
    // 88W8801不会出现curr=0的情况, 所以这里不需要while语句
    curr = WiFi_LowLevel_ReadReg(1, WIFI_SQREADBASEADDR0) | (WiFi_LowLevel_ReadReg(1, WIFI_SQREADBASEADDR1) << 8);
    //printf("Required: %d bytes, Remaining: %d bytes\n", curr, len);
    
    if (curr & 1)
    {
      // 若curr为奇数(如17), 则说明接收端出现了CRC校验错误, 应重新传送上一次的内容(这部分代码省略)
      printf("Error: an odd size is invalid!\n");
      while (1);
    }
    if (curr > len)
      curr = len;
    
    // 发送固件数据
    if (len < 32) // len为缓冲区剩余大小
    {
      // 若缓冲区空间不足一个数据块, 则借用helper_buf
      memcpy(helper_buf, data, curr);
      WiFi_LowLevel_WriteData(1, wifi_port, helper_buf, curr, sizeof(helper_buf));
    }
    else
      WiFi_LowLevel_WriteData(1, wifi_port, data, curr, len);
    
    len -= curr;
    data += curr;
  }
  
  // 等待Firmware启动
  WiFi_Wait(WIFI_INTSTATUS_DNLD, 0);
  while (WiFi_GetFirmwareStatus() != 0xfedc);
  printf("Firmware is successfully downloaded!\n");
}

/* 丢弃收到但因程序错误一直未处理的数据或命令 */
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

/* 处理WiFi事件 */
static void WiFi_EventProcess(const WiFi_Event *event)
{
  printf("[Event %d] size=%d, bss=0x%02x\n", event->event_id, event->header.length, WIFI_BSS(event->bss_type, event->bss_num));
  switch (event->event_id)
  {
    case 3:
    case 8:
    case 9:
      // WiFi掉线
      memset(&wifi_ssid_info, 0, sizeof(wifi_ssid_info)); // 清空SSID信息
      break;
    case 43:
      // 成功关联热点并完成WPA/WPA2认证
      // WEP模式和无密码模式无需认证, 只要关联上就会产生此事件
      WiFi_AuthenticationCompleteHandler();
      break;
  }
}

/* 获取可用的通道号 */
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

/* 获取所连热点的MAC地址 */
uint8_t WiFi_GetBSSID(uint8_t mac_addr[6])
{
  if (mac_addr != NULL)
    memcpy(mac_addr, wifi_ssid_info.mac_addr, 6);
  return !isempty(wifi_ssid_info.mac_addr, 6); // 返回值表示MAC地址是否不全为0
}

/* 获取需要接收的数据大小 */
// 当port=0时, 获取待接收的命令回应帧或事件帧的大小
// 当port=1~15时, 获取待接收的数据帧的大小, port为数据通道号
uint16_t WiFi_GetDataLength(uint8_t port)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_RDLENP0L + 2 * port) | (WiFi_LowLevel_ReadReg(1, WIFI_RDLENP0U + 2 * port) << 8);
}

/* 获取固件状态 */
uint16_t WiFi_GetFirmwareStatus(void)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_SCRATCH0_0) | (WiFi_LowLevel_ReadReg(1, WIFI_SCRATCH0_1) << 8);
}

/* 获取MAC地址 */
// callback不能为NULL
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

/* 请求发送新的数据帧 */
uint8_t *WiFi_GetPacketBuffer(void)
{
  WiFi_DataTx *data = (WiFi_DataTx *)wifi_buffer_packet;
  return data->payload;
}

/* 获取接收通道的状态 */
uint16_t WiFi_GetReadPortStatus(void)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_RDBITMAPL) | (WiFi_LowLevel_ReadReg(1, WIFI_RDBITMAPU) << 8);
}

/* 获取收到的数据帧 */
const WiFi_DataRx *WiFi_GetReceivedPacket(void)
{
  WiFi_DataRx *data = (WiFi_DataRx *)wifi_buffer_rx;
  if (data->header.type == WIFI_SDIOFRAME_DATA)
    return data;
  else
    return NULL;
}

/* 获取热点的认证类型 */
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

/* 获取发送通道的状态 */
uint16_t WiFi_GetWritePortStatus(void)
{
  return WiFi_LowLevel_ReadReg(1, WIFI_WRBITMAPL) | (WiFi_LowLevel_ReadReg(1, WIFI_WRBITMAPU) << 8);
}

/* 初始化WiFi模块 */
void WiFi_Init(void)
{
  // 初始化底层寄存器
  WiFi_LowLevel_Init();
  WiFi_ShowCIS();
  
  // 初始化Function 1
  WiFi_LowLevel_WriteReg(0, SDIO_CCCR_IOEN, SDIO_CCCR_IOEN_IOE1); // IOE1=1 (Enable Function)
  while ((WiFi_LowLevel_ReadReg(0, SDIO_CCCR_IORDY) & SDIO_CCCR_IORDY_IOR1) == 0); // 等待IOR1=1 (I/O Function Ready)
  WiFi_LowLevel_WriteReg(0, SDIO_CCCR_INTEN, SDIO_CCCR_INTEN_IENM | SDIO_CCCR_INTEN_IEN1); // 打开SDIO中断请求
  WiFi_LowLevel_WriteReg(1, WIFI_INTMASK, WIFI_INTMASK_HOSTINTMASK); // 利用中断标志位来判定是否有数据要读取, 可靠性更高
  
  // 下载固件
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
  status = WiFi_LowLevel_ReadReg(1, WIFI_INTSTATUS); // 获取需要处理的中断标志位
  if (status == 0)
    return;
  WiFi_LowLevel_WriteReg(1, WIFI_INTSTATUS, WIFI_INTSTATUS_ALL & ~status); // 必须先清除这些标志位, 然后再进行处理, 这样可以避免清除掉处理过程中新来的中断
  
  if (status & WIFI_INTSTATUS_DNLD)
  {
    // 遍历所有发送了数据但还没有收到确认的通道
    for (port = 1; port <= WIFI_DATAPORTS_NUM; port++) // 在88W8801中, 命令通道释放时不会产生中断, 所以不需要管通道0
    {
      if (wifi_tx_packet[port - 1].busy)
      {
        bitmap = WiFi_GetWritePortStatus(); // 数据帧回调函数中有可能发送新数据帧导致某些通道再次被占用, 所以每次都要重新读寄存器
        if (bitmap & _BV(port))
          WiFi_PacketComplete(port, WIFI_STATUS_OK); // 通道现在为空闲, 表明该通道收到了确认
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
          // 收到以太网数据帧
          if (rx_packet->rx_packet_type == 2) // Ethernet version 2 frame
            WiFi_PacketHandler(rx_packet, port);
          else
            printf("[Recv] type=%d\n", rx_packet->rx_packet_type);
          break;
        case WIFI_SDIOFRAME_COMMAND:
          // 收到命令回应帧
          if (rx_cmd->seq_num == tx_cmd->seq_num) // 序号相符
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
          // 收到事件帧
          WiFi_EventProcess((WiFi_Event *)wifi_buffer_rx);
          WiFi_EventHandler((WiFi_Event *)wifi_buffer_rx); // 调用事件处理回调函数
          break;
      }
    }
  }
}

/* 发送命令帧前, 必须保证命令发送缓冲区为空 */
uint8_t WiFi_IsCommandBusy(void)
{
  return wifi_tx_command.busy;
}

/* 加入Ad-Hoc网络 */
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
      strncpy((char *)cmd->ssid, (char *)wifi_ssid_info.ssid.ssid, sizeof(cmd->ssid)); // strncpy会将未使用的区域填充为0
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

/* 加入带有密码的Ad-Hoc网络 */
void WiFi_JoinADHOCEx(const WiFi_Connection *conn, int8_t max_retry, WiFi_Callback callback, void *arg)
{
  int8_t *pmax_retry;
  uint16_t ssid_len;
  void **p;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  if (conn->security == WIFI_SECURITYTYPE_WPA || conn->security == WIFI_SECURITYTYPE_WPA2)
  {
    // 88W8686虽然能够连上电脑创建的WPA2认证类型的Ad-Hoc热点, 也能完成EAPOL认证
    // 但是同时也会自己创建一个WEP型的同名Ad-Hoc热点, 使通信无法正常进行
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

/* 获取或设置MAC地址 */
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

/* 配置MAC */
void WiFi_MACControl(uint16_t action, WiFi_Callback callback, void *arg)
{
  WiFi_Cmd_MACCtrl *cmd = (WiFi_Cmd_MACCtrl *)wifi_buffer_command;
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  cmd->action = action;
  cmd->reserved = 0;
  WiFi_SendCommand(CMD_MAC_CONTROL, wifi_buffer_command, sizeof(WiFi_Cmd_MACCtrl), WIFI_BSS_STA, callback, arg, WIFI_DEFAULT_TIMEOUT_CMDRESP, WIFI_DEFAULT_MAXRETRY);
}

/* 释放数据帧发送缓冲区并调用回调函数 */
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

/* 显示速率列表 */
void WiFi_PrintRates(uint16_t rates)
{
  // 速率列表在固件API手册的3.1 Receive Packet Descriptor一节里面的RxRate字段描述里面
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

/* 显示数据帧的发送情况 */
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

/* 扫描全部热点 (仅显示) */
void WiFi_Scan(WiFi_Callback callback, void *arg)
{
  uint8_t i;
  void **p;
  WiFi_CmdRequest_Scan *cmd = (WiFi_CmdRequest_Scan *)wifi_buffer_command; // 要发送的命令
  MrvlIETypes_ChanListParamSet_t *chanlist = (MrvlIETypes_ChanListParamSet_t *)(cmd + 1); // 这里的+1指的是前进sizeof(指针类型)个地址单元, 而非只前进1个地址单元
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
  
  // 通道的基本参数
  chanlist->header.type = WIFI_MRVLIETYPES_CHANLISTPARAMSET;
  chanlist->header.length = 4 * sizeof(chanlist->channels);
  for (i = 0; i < 4; i++) // 先扫描前4个通道 (i作下标, i+1才是通道号)
  {
    chanlist->channels[i].band_config_type = 0; // 2.4GHz band, 20MHz channel width
    chanlist->channels[i].chan_number = i + 1; // 通道号
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
  
  // 发送扫描接下来的4个通道的命令
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
  
  // 显示本次扫描结果, num_of_set为热点数
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
            // SSID名称
            memcpy(ssid, ie_params->data, ie_params->header.length);
            ssid[ie_params->header.length] = '\0';
            break;
          case WIFI_MRVLIETYPES_RATESPARAMSET:
            // 速率
            rates = ie_params;
            break;
          case WIFI_MRVLIETYPES_PHYPARAMDSSET:
            // 通道号
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
      
      printf("SSID '%s', ", ssid); // 热点名称
      printf("MAC %02X:%02X:%02X:%02X:%02X:%02X, ", bss_desc_set->bssid[0], bss_desc_set->bssid[1], bss_desc_set->bssid[2], bss_desc_set->bssid[3], bss_desc_set->bssid[4], bss_desc_set->bssid[5]); // MAC地址
      printf("RSSI %d, Channel %d\n", bss_desc_set->rssi, channel); // 信号强度和通道号
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
        // 关于速率值的表示方法, 请参阅802.11-2016.pdf的9.4.2.3 Supported Rates and BSS Membership Selectors element一节的内容
        printf("  Rates:");
        for (j = 0; j < rates->header.length; j++)
          printf(" %.1fMbps", (rates->data[j] & 0x7f) * 0.5);
        printf("\n");
      }
      
      // 转向下一个热点信息
      bss_desc_set = (WiFi_BssDescSet *)((uint8_t *)bss_desc_set + sizeof(bss_desc_set->ie_length) + bss_desc_set->ie_length);
    }
    
    // resp->buf_size就是bss_desc_set的总大小
    // 因此tft_table == buffer + sizeof(WiFi_CmdResponse_Scan) + resp->buf_size
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
    
    // TSF timestamp table是整个数据的末尾, 后面没有Channel/band table
    //if (((uint8_t *)tft_table - (uint8_t *)data) + TLV_STRUCTLEN(*tft_table) == resp->header.frame_header.length)
    //  printf("data end!\n");
  }
  
  // 扫描完毕时调用回调函数
  if (n == 0)
  {
    free(arg);
    if (app_callback)
      app_callback(app_arg, data, status);
  }
}

/* 获取指定名称的热点的详细信息 */
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
  memset(info, 0, sizeof(WiFi_SSIDInfo)); // 将整个info结构体清零
  
  cmd->bss_type = WIFI_BSS_ANY;
  memset(cmd->bss_id, 0, sizeof(cmd->bss_id));
  
  // 给info->ssid成员赋值
  info->ssid.header.type = WIFI_MRVLIETYPES_SSIDPARAMSET;
  info->ssid.header.length = strlen(ssid);
  memcpy(info->ssid.ssid, ssid, info->ssid.header.length);
  memcpy(cmd + 1, &info->ssid, TLV_STRUCTLEN(info->ssid)); // 把info->ssid复制到待发送的命令内容中
  
  chan_list = (MrvlIETypes_ChanListParamSet_t *)((uint8_t *)(cmd + 1) + TLV_STRUCTLEN(info->ssid));
  chan_list->header.type = WIFI_MRVLIETYPES_CHANLISTPARAMSET;
  chan_list->header.length = 14 * sizeof(chan_list->channels); // 一次性扫描14个通道
  for (i = 0; i < 14; i++) // i作下标, i+1才是通道号
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
    // 未找到指定的AP热点, 此时info结构体中除了ssid成员外, 其余的成员均为0
    // resp中的内容到了num_of_set成员后就没有了
    printf("SSID not found!\n");
    if (app_callback)
      app_callback(app_arg, data, WIFI_STATUS_NOTFOUND);
    return;
  }
  
  // bss_desc_set以扫描到的第一个信息项为准
  memcpy(info->mac_addr, bss_desc_set->bssid, sizeof(info->mac_addr));
  info->cap_info = bss_desc_set->cap_info;
  info->bcn_period = bss_desc_set->bcn_interval;
  
  // 若info->xxx.header.type=0, 则表明没有该项的信息 (除SSID结构体外, 因为SSID的type=WIFI_MRVLIETYPES_SSIDPARAMSET=0)
  ie_params = &bss_desc_set->ie_parameters;
  ie_size = bss_desc_set->ie_length - (sizeof(WiFi_BssDescSet) - sizeof(bss_desc_set->ie_length) - sizeof(bss_desc_set->ie_parameters)); // 所有IEEE_Type数据的总大小
  while (ie_size > 0)
  {
    switch (ie_params->header.type)
    {
      case WIFI_MRVLIETYPES_RATESPARAMSET:
        // 速率
        WiFi_TranslateTLV((MrvlIEType *)&info->rates, ie_params, sizeof(info->rates.rates));
        break;
      case WIFI_MRVLIETYPES_PHYPARAMDSSET:
        // 通道号
        info->channel = ie_params->data[0];
        break;
      case WIFI_MRVLIETYPES_RSNPARAMSET:
        // 通常只有一个RSN信息 (与WPA2相关)
        WiFi_TranslateTLV((MrvlIEType *)&info->rsn, ie_params, sizeof(info->rsn.rsn));
        break;
      case WIFI_MRVLIETYPES_VENDORPARAMSET:
        // 通常会有多项VENDOR信息 (与WPA相关)
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
              if (ie_params->header.length == 24) // 合法大小
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
    
    // 转向下一个TLV
    ie_size -= TLV_STRUCTLEN(*ie_params);
    ie_params = (IEEEType *)TLV_NEXT(ie_params);
  }
  
  if (app_callback)
    app_callback(app_arg, data, status);
}

/* 发送WiFi命令, 收到回应或超时时调用callback回调函数 */
// size=0表示data包含完整的命令数据, 且code, size和bss可直接从data中获取(用于重发)
// retry可以为0(第一次失败时就直接调用回调函数, 不再重试), 但timeout不能为0(否则收到回应前会误认为超时并调用回调函数)
//
// 无操作系统的环境下只能使用非阻塞方式执行WiFi命令, 并通过回调函数通知命令执行的结果 (回调函数应保证能够被调用并只调用一次)
// 如果有操作系统, 某个任务想要以阻塞方式执行WiFi命令, 可以在该函数里面添加发送命令前阻塞等待表示命令通道是否可用的0-1信号量的代码
// 当命令通道可用时, 使信号量的值为1并唤醒其中一个等待发送命令的任务, 发送命令后继续阻塞等待回应, 调用回调函数并根据命令执行结果(成功还是失败)决定函数的返回值
void WiFi_SendCommand(uint16_t code, const void *data, uint16_t size, uint8_t bss, WiFi_Callback callback, void *arg, uint32_t timeout, uint8_t max_retry)
{
  static uint8_t seq_num = 0; // 88W8801中命令序列号为8位
  WiFi_CommandHeader *cmdhdr = (WiFi_CommandHeader *)wifi_buffer_command;
  
  if (WiFi_CheckCommandBusy(callback, arg)) // 发送命令前必须确保之前的命令已经发送完毕
    return;
  
  // 直接发送命令: WiFi_SendCommand(0, data, 0, ...)
  // 填充命令头并发送命令: WiFi_SendCommand(code, data, size, ...)
  // 重试上次命令: WiFi_SendCommand(0, NULL, 0, ...)
  if (data != NULL && data != wifi_buffer_command)
    memmove(wifi_buffer_command, data, (size != 0) ? size : ((WiFi_SDIOFrameHeader *)data)->length); // 将要发送的命令内容复制到缓冲区中, 以便出错时重发
  
  //printf("seq_num=%d\n", seq_num);
  if (size != 0)
  {
    cmdhdr->frame_header.length = size;
    cmdhdr->frame_header.type = WIFI_SDIOFRAME_COMMAND;
    cmdhdr->cmd_code = code;
    cmdhdr->size = size - sizeof(WiFi_SDIOFrameHeader); // 命令大小包括命令头部, 但不包括SDIO帧头部
    cmdhdr->seq_num = seq_num++;
    cmdhdr->bss = bss; // 88W8801新增字段
    cmdhdr->result = 0;
  }
  else
    size = cmdhdr->frame_header.length; // 重发命令时不填写cmdhdr
  WiFi_LowLevel_WriteData(1, wifi_port, wifi_buffer_command, size, sizeof(wifi_buffer_command));
  // WriteData函数出错的概率很小, 这里简单起见就不去判断它的返回值了
  // 即使出错了(如CRC校验错误), 由于收不到命令回应, WiFi_CheckTimeout函数也会重传该命令
  
  wifi_tx_command.arg = arg;
  wifi_tx_command.busy = 1;
  wifi_tx_command.callback = callback;
  wifi_tx_command.retry = max_retry;
  wifi_tx_command.start_time = sys_now();
  wifi_tx_command.timeout = timeout;
}

/* 发送数据帧 */
// data指向的是WiFi_DataTx.payload
// size=0表示再次发送上一帧, 此时data参数将被忽略
// 返回值表示发送数据所用的通道号
int8_t WiFi_SendPacket(void *data, uint16_t size, uint8_t bss, WiFi_Callback callback, void *arg, uint32_t timeout)
{
  static int8_t port = 0;
  int8_t newport;
  uint16_t bitmap;
  WiFi_DataTx *packet = (WiFi_DataTx *)wifi_buffer_packet;
  
  // 发送新数据帧: WiFi_SendPacket(data, size, ...)
  // 重传前一数据帧: WiFi_SendPacket(NULL, 0, ...)
  if (size == 0)
    data = packet->payload;
  
  do
  {
    // 数据通道号必须连续使用, 不能跨越使用, 否则将会导致数据帧发送失败
    // 例如, 使用了通道4后, 下次必须使用通道5, 不能使用通道6
    if (port == WIFI_DATAPORTS_NUM)
      newport = 1;
    else
      newport = port + 1;
    
    // 等待数据通道可用
    do
    {
      bitmap = WiFi_GetWritePortStatus();
    } while ((bitmap & _BV(newport)) == 0);
    
    // 通道释放出来后, 调用该通道对应的回调函数
    if (wifi_tx_packet[newport - 1].busy)
      WiFi_PacketComplete(newport, WIFI_STATUS_OK);
  } while (wifi_tx_packet[newport - 1].busy); // 如果在回调函数中发送了新数据帧, 则根据静态变量port的新值重新计算通道号
  // 回调函数中的新数据帧使用了相同的端口号, 所以一定能发送成功, 不会在里面再次调用WiFi_PacketComplete形成多层递归
  port = newport;
  
  if (data != packet->payload)
    memmove(packet->payload, data, size); // 将要发送的数据内容复制到缓冲区中
  
  if (size != 0)
  {
    // 有关发送数据包的细节, 请参考Firmware Specification PDF的Chapter 3: Data Path
    packet->header.length = sizeof(WiFi_DataTx) - sizeof(packet->payload) + size;
    packet->header.type = WIFI_SDIOFRAME_DATA;
    
    packet->bss_type = WIFI_BSSTYPE(bss);
    packet->bss_num = WIFI_BSSNUM(bss);
    packet->tx_packet_length = size;
    packet->tx_packet_offset = sizeof(WiFi_DataTx) - sizeof(packet->payload) - sizeof(packet->header); // 不包括SDIOFrameHeader
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
  
  // 接下来需要等待Download Ready位置1, 表明数据帧发送成功
  wifi_tx_packet[port - 1].arg = arg;
  wifi_tx_packet[port - 1].busy = 1;
  wifi_tx_packet[port - 1].callback = callback;
  wifi_tx_packet[port - 1].retry = 0;
  wifi_tx_packet[port - 1].start_time = sys_now();
  wifi_tx_packet[port - 1].timeout = timeout;
  return port;
}

/* 设置WEP密钥 (长度必须为5或13个字符) */
// action: WIFI_ACT_ADD / WIFI_ACT_REMOVE, 移除密钥时参数key可以设为NULL
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

/* 使用CMD_802_11_KEY_MATERIAL命令设置WEP密钥 */
// 仅支持单个密钥
void WiFi_SetWEPEx(const WiFi_WEPKey *key, WiFi_Callback callback, void *arg)
{
  static int8_t last = -1;
  int8_t len;
  int8_t *plast;
  WiFi_Cmd_KeyMaterial *cmd = (WiFi_Cmd_KeyMaterial *)wifi_buffer_command;
  MrvlIETypes_KeyParamSet_v2_t *keyparamset = (MrvlIETypes_KeyParamSet_v2_t *)(cmd + 1); // 设置密钥时必须使用V2版本的TLV参数
  WiFi_KeyParam_WEP_v2 *keyparam = (WiFi_KeyParam_WEP_v2 *)(keyparamset + 1);
  WiFi_Callback *pcallback;
  
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  cmd->action = WIFI_ACT_SET;
  keyparamset->header.type = WIFI_MRVLIETYPES_KEYPARAMSETV2;
  keyparamset->header.length = TLV_PAYLOADLEN(*keyparamset);
  memset(keyparamset->mac_address, 0, sizeof(keyparamset->mac_address)); // 如果MAC地址不为0, 则必须要连接了热点之后才能设置成功
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
  
  // 待删除密钥的序号 (放到命令发送缓冲区存放的命令末尾, 供回调函数使用)
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
  // 移除旧密钥
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

/* 设置WPA密码 */
void WiFi_SetWPA(const char *password, WiFi_Callback callback, void *arg)
{
  WiFi_Cmd_SupplicantPMK *cmd = (WiFi_Cmd_SupplicantPMK *)wifi_buffer_command;
  MrvlIETypes_WPA_Passphrase_t *passphrase;
  
  if (WiFi_CheckCommandBusy(callback, arg))
    return;
  
  cmd->action = WIFI_ACT_SET;
  cmd->cache_result = 0; // 命令回应中, 0表示成功, 1表示失败
  passphrase = (MrvlIETypes_WPA_Passphrase_t *)(cmd + 1);
  passphrase->header.type = WIFI_MRVLIETYPES_WPAPASSPHRASE;
  passphrase->header.length = strlen(password);
  strcpy((char *)passphrase->passphrase, password);
  
  WiFi_SendCommand(CMD_SUPPLICANT_PMK, cmd, TLV_NEXT(passphrase) - wifi_buffer_command, WIFI_BSS_STA, callback, arg, WIFI_TIMEOUT_SCAN, WIFI_DEFAULT_MAXRETRY);
}

/* 显示WiFi模块信息 */
void WiFi_ShowCIS(void)
{
  uint8_t data[255];
  uint8_t func, i, n, len;
  uint8_t tpl_code, tpl_link; // 16.2 Basic Tuple Format and Tuple Chain Structure
  uint32_t cis_ptr;
  
  n = WiFi_LowLevel_GetFunctionNum();
  for (func = 0; func <= n; func++)
  {
    // 获取CIS的地址
    cis_ptr = (func << 8) | 0x9;
    cis_ptr  = WiFi_LowLevel_ReadReg(0, cis_ptr) | (WiFi_LowLevel_ReadReg(0, cis_ptr + 1) << 8) | (WiFi_LowLevel_ReadReg(0, cis_ptr + 2) << 16);
    printf("[CIS] func=%d, ptr=0x%08x\n", func, cis_ptr);
    
    // 遍历CIS, 直到尾节点
    while ((tpl_code = WiFi_LowLevel_ReadReg(0, cis_ptr++)) != CISTPL_END)
    {
      if (tpl_code == CISTPL_NULL)
        continue;
      
      tpl_link = WiFi_LowLevel_ReadReg(0, cis_ptr++); // 本结点数据的大小
      for (i = 0; i < tpl_link; i++)
        data[i] = WiFi_LowLevel_ReadReg(0, cis_ptr + i);
      
      switch (tpl_code)
      {
        case CISTPL_VERS_1:
          printf("Product Information:");
          for (i = 2; data[i] != 0xff; i += len + 1)
          {
            // 遍历所有字符串
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
        break; // 当TPL_LINK为0xff时说明当前结点为尾节点
      cis_ptr += tpl_link;
    }
  }
}

/* 显示密钥 */
void WiFi_ShowKeyMaterials(void)
{
  WiFi_Cmd_KeyMaterial *cmd = (WiFi_Cmd_KeyMaterial *)wifi_buffer_command;
  MrvlIETypes_KeyParamSet_t *key = (MrvlIETypes_KeyParamSet_t *)(cmd + 1);
  if (WiFi_IsCommandBusy())
    return;
  
  cmd->action = WIFI_ACT_GET;
  key->header.type = WIFI_MRVLIETYPES_KEYPARAMSET;
  key->header.length = 6; // 这个TLV参数只能包含下面三个字段
  key->key_type_id = 0;
  key->key_info = WIFI_KEYINFO_UNICASTKEY | WIFI_KEYINFO_MULTICASTKEY; // 同时获取PTK和GTK密钥
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
  
  // 输出所有获取到的密钥
  // info=0x6为单播密钥(PTK), info=0x5为广播密钥(GTK)
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
    keyparam_len = key->header.length - (TLV_PAYLOADLEN(*key) - sizeof(key->key)); // key->key数组的真实大小(有可能不等于key->key_len)
    
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
    
    // 转到下一对密钥+MAC地址组合
    mac = (MrvlIETypes_StaMacAddr_t *)TLV_NEXT(key);
    key = (MrvlIETypes_KeyParamSet_t *)TLV_NEXT(mac);
  }
  
  if (key == (MrvlIETypes_KeyParamSet_t *)(resp + 1))
    printf("No key materials!\n");
}

/* 创建一个Ad-Hoc型的WiFi热点 */
// 创建带有WEP密码的热点时, cap_info为WIFI_CAPABILITY_PRIVACY
// 创建无密码的热点时, cap_info为0
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

/* 创建一个Ad-Hoc型的WiFi热点并设置好密码 */
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

/* 创建微型AP热点 */
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

/* 释放发送缓冲区并调用回调函数 */
static void WiFi_TxBufferComplete(WiFi_TxBuffer *tbuf, void *data, WiFi_Status status)
{
  if (tbuf->busy)
  {
    tbuf->busy = 0;
    if (tbuf->callback)
      tbuf->callback(tbuf->arg, data, status); // 调用回调函数
  }
}

/* 将IEEE型的TLV转换成MrvlIE型的TLV */
uint8_t WiFi_TranslateTLV(MrvlIEType *mrvlie_tlv, const IEEEType *ieee_tlv, uint16_t mrvlie_payload_size)
{
  mrvlie_tlv->header.type = ieee_tlv->header.type;
  if (ieee_tlv->header.length > mrvlie_payload_size)
    mrvlie_tlv->header.length = mrvlie_payload_size; // 若源数据大小超过缓冲区最大容量, 则丢弃剩余数据
  else
    mrvlie_tlv->header.length = ieee_tlv->header.length;
  memset(mrvlie_tlv->data, 0, mrvlie_payload_size); // 清零
  memcpy(mrvlie_tlv->data, ieee_tlv->data, mrvlie_tlv->header.length); // 复制数据
  return mrvlie_tlv->header.length == ieee_tlv->header.length; // 返回值表示缓冲区大小是否足够
}

/* 在规定的超时时间内, 等待指定的卡状态位置位, 并清除相应的中断标志位 */
// 成功时返回1
uint8_t WiFi_Wait(uint8_t status, uint32_t timeout)
{
  uint32_t diff;
  uint32_t start = sys_now();
  while ((WiFi_LowLevel_ReadReg(1, WIFI_INTSTATUS) & status) != status)
  {
    diff = sys_now() - start;
    if (timeout != 0 && diff > timeout)
    {
      // 若超时时间已到
      printf("WiFi_Wait(0x%02x): timeout!\n", status);
      return 0;
    }
  }
  
  // 清除对应的中断标志位
  WiFi_LowLevel_WriteReg(1, WIFI_INTSTATUS, WIFI_INTSTATUS_ALL & ~status); // 不需要清除的位必须为1
  // 不能将SDIOIT位清除掉! 否则有可能导致该位永远不再置位
  return 1;
}
