#include <lwip/apps/httpd.h> // http服务器
#include <lwip/apps/netbiosns.h> // NetBIOS服务
#include <lwip/dhcp.h> // DHCP客户端
#include <lwip/dns.h> // DNS客户端
#include <lwip/init.h> // lwip_init函数所在的头文件
#include <lwip/timeouts.h> // sys_check_timeouts函数所在的头文件
#include <netif/ethernet.h> // ethernet_input函数所在头文件
#include <stm32f10x.h>
#include <string.h>
#include "common.h"
#include "WiFi.h"
#include "wifi_test.h"

//#define METHOD 1 // 只连接热点
//#define METHOD 2 // 只创建热点
#define METHOD 3 // 创建热点, 然后连接路由器 (推荐)

#define AUTO_RECONNECT // 断线自动重连

// 这两个函数位于ethernetif.c中, 但没有头文件声明
err_t ethernetif_init(struct netif *netif);
void ethernetif_input(struct netif *netif);

// 本文件用到的函数声明
void associate_example(void);

static struct netif wifi_88w8801_sta;
static struct netif wifi_88w8801_uap;

int fputc(int ch, FILE *fp)
{
  if (fp == stdout)
  {
    if (ch == '\n')
    {
      while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); // 等待前一字符发送完毕
      USART_SendData(USART1, '\r');
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, ch);
  }
  return ch;
}

/* 显示DHCP分配的IP地址 */
#if LWIP_DHCP
static void display_ip(void)
{
  static uint8_t ip_displayed = 0;
  struct dhcp *dhcp;
  
  if (dhcp_supplied_address(&wifi_88w8801_sta))
  {
    if (ip_displayed == 0)
    {
      ip_displayed = 1;
      dhcp = netif_dhcp_data(&wifi_88w8801_sta);
      printf("DHCP supplied address!\n");
      printf("IP address: %s\n", ipaddr_ntoa(&dhcp->offered_ip_addr));
      printf("Subnet mask: %s\n", ipaddr_ntoa(&dhcp->offered_sn_mask));
      printf("Default gateway: %s\n", ipaddr_ntoa(&dhcp->offered_gw_addr));
#if LWIP_DNS
      printf("DNS Server: %s\n", ipaddr_ntoa(dns_getserver(0)));
      dns_test();
#endif
    }
  }
  else
    ip_displayed = 0;
}
#endif

/* WiFi认证成功回调函数 */
void WiFi_AuthenticationCompleteHandler(void)
{
  printf("Authenticated!\n");
  netif_set_link_up(&wifi_88w8801_sta);
  
#if LWIP_DHCP
  if (netif_dhcp_data(&wifi_88w8801_sta) == NULL)
    dhcp_start(&wifi_88w8801_sta);
#endif
}

/* WiFi事件回调函数 */
void WiFi_EventHandler(const WiFi_Event *event)
{
#ifdef AUTO_RECONNECT
  uint8_t up = netif_is_link_up(&wifi_88w8801_sta);
#endif
  
  switch (event->event_id)
  {
    case 3:
      // 收不到信号 (例如和手机热点建立连接后, 把手机拿走), WiFi模块不会自动重连
      printf("Beacon Loss/Link Loss\n");
      netif_set_link_down(&wifi_88w8801_sta);
      break;
    case 4:
      // Ad-Hoc网络中不止1个结点, 且连接数发生了变化
      printf("The number of stations in this ad hoc newtork has changed!\n");
      netif_set_link_up(&wifi_88w8801_sta);
      break;
    case 8:
      // 认证已解除 (例如手机关闭了热点, 或者连接路由器后因认证失败而自动断开连接)
      printf("Deauthenticated!\n");
      netif_set_link_down(&wifi_88w8801_sta);
      break;
    case 9:
      // 解除了关联
      printf("Disassociated!\n");
      netif_set_link_down(&wifi_88w8801_sta);
      break;
    case 17:
      // Ad-Hoc网络中只剩本结点
      printf("All other stations have been away from this ad hoc network!\n");
      netif_set_link_down(&wifi_88w8801_sta);
      break;
    case 23:
      printf("WMM status change event occurred!\n");
      break;
    case 30:
      printf("IBSS coalescing process is finished and BSSID has changed!\n");
      break;
    case 70:
      printf("WEP decryption error!\n");
      break;
  }
  
  // 断线自动重连
#ifdef AUTO_RECONNECT
  if (up && !netif_is_link_up(&wifi_88w8801_sta))
  {
    printf("Reconnecting...\n");
    associate_example(); // 只需要调用这个函数重新关联热点就行了, 不需要重新获取MAC地址
  }
#endif
}

/* WiFi模块收到新的数据帧 */
void WiFi_PacketHandler(const WiFi_DataRx *data, int8_t port)
{
  uint8_t bss;
  struct netif *netif;
  struct ethernetif *state;
  
  bss = WIFI_BSS(data->bss_type, data->bss_num);
  if (bss == WIFI_BSS_STA)
    netif = &wifi_88w8801_sta;
  else if (bss == WIFI_BSS_UAP)
    netif = &wifi_88w8801_uap;
  else
    return;

  state = netif->state;
  state->data = data;
  state->port = port;
  ethernetif_input(netif); // 交给lwip处理
}

static void microap_callback(void *arg, void *data, WiFi_Status status)
{
  if (status == WIFI_STATUS_OK)
  {
    printf("MicroAP started!\n");
    netif_set_link_up(&wifi_88w8801_uap);
    
#if METHOD == 3
    associate_example();
#endif
  }
  else
    printf("Cannot start MicroAP!\n");
}

// 创建路由器热点示例代码
void microap_example(void)
{
  WiFi_Connection conn;
  
  conn.security = WIFI_SECURITYTYPE_NONE;
  conn.ssid = NULL; // 默认SSID
  //conn.ssid = "Hello World"; // 自定义SSID
  WiFi_StartMicroAP(&conn, WIFI_AUTH_MODE_OPEN, microap_callback, NULL);
}

static void associate_callback(void *arg, void *data, WiFi_Status status)
{
  switch (status)
  {
    case WIFI_STATUS_OK:
      printf("Associated!\n");
      break;
    case WIFI_STATUS_NOTFOUND:
      printf("SSID not found!\n");
      break;
    case WIFI_STATUS_FAIL:
      printf("Association failed!\n");
      break;
    case WIFI_STATUS_INPROGRESS:
      printf("Waiting for authentication!\n");
      break;
    default:
      printf("Unknown error! status=%d\n", status);
  }
}

// 连接路由器热点的示例代码
void associate_example(void)
{
  WiFi_Connection conn;
  //WiFi_WEPKey wepkey = {0}; // 未使用的成员必须设为0
  
  /*
  conn.security = WIFI_SECURITYTYPE_WEP;
  conn.ssid = "Oct1158-2";
  conn.password = &wepkey;
  wepkey.keys[0] = "1234567890123";
  wepkey.index = 0; // 密钥序号必须要正确
  WiFi_AssociateEx(&conn, WIFI_AUTH_MODE_OPEN, -1, associate_callback, NULL); // 开放系统方式
  //WiFi_AssociateEx(&conn, WIFI_AUTH_MODE_SHARED, -1, associate_callback, NULL); // 共享密钥方式
  */
  
  /*
  conn.security = WIFI_SECURITYTYPE_NONE;
  conn.ssid = "vivo Y29L";
  WiFi_AssociateEx(&conn, WIFI_AUTH_MODE_OPEN, -1, associate_callback, NULL);
  */
  
  ///*
  conn.security = WIFI_SECURITYTYPE_WPA; // WPA和WPA2都可以使用此选项
  conn.ssid = "Oct1158-2";
  conn.password = "wi5Nkj9QHBigqRmcXox86u544BNIzM9r";
  WiFi_AssociateEx(&conn, WIFI_AUTH_MODE_OPEN, -1, associate_callback, NULL); // 必须使用WIFI_AUTH_MODE_OPEN选项
  //*/
}

static void adhoc_callback(void *arg, void *data, WiFi_Status status)
{
  if (status == WIFI_STATUS_OK)
    printf("ADHOC %sed!\n", (char *)arg);
  else
    printf("Cannot %s ADHOC!\n", (char *)arg);
}

// 创建或连接ADHOC型热点的示例代码
void adhoc_example(void)
{
  WiFi_Connection conn;
  WiFi_WEPKey wepkey = {0}; // 未使用的成员必须设为0
  
  /*
  conn.security = WIFI_SECURITYTYPE_WEP;
  conn.ssid = "Octopus-WEP";
  conn.password = &wepkey;
  wepkey.keys[0] = "3132333435";
  wepkey.index = 0; // 范围: 0~3
  WiFi_JoinADHOCEx(&conn, -1, adhoc_callback, "join");
  */
  
  ///*
  // 注意: 电脑上无论密码输入是否正确都可以连接, 但只有正确的密码才可以通信
  conn.security = WIFI_SECURITYTYPE_WEP;
  conn.ssid = "WM-G-MR-09";
  conn.password = &wepkey;
  wepkey.keys[0] = "1234567890123";
  wepkey.index = 0;
  WiFi_StartADHOCEx(&conn, adhoc_callback, "start");
  //*/
  
  /*
  conn.security = WIFI_SECURITYTYPE_NONE;
  conn.ssid = "Octopus-WEP";
  WiFi_JoinADHOCEx(&conn, -1, adhoc_callback, "join");
  */
  
  /*
  conn.security = WIFI_SECURITYTYPE_NONE;
  conn.ssid = "WM-G-MR-09";
  WiFi_StartADHOCEx(&conn, adhoc_callback, "start");
  */
}

static void scan_callback(void *arg, void *data, WiFi_Status status)
{
  if (status == WIFI_STATUS_OK)
    printf("Scan finished!\n");
  else
    printf("Scan failed!\n");
  
#if METHOD == 2 || METHOD == 3
  microap_example();
#elif METHOD == 1
  associate_example();
#elif METHOD == 0
  adhoc_example(); // 88W8801不支持Ad-Hoc, 请勿使用此选项
#endif
}

// 获取网卡MAC地址成功后, 就立即将网卡添加到lwip中, 但暂不把网卡设为"已连接"状态
// 只需要在单片机复位的时候获取一次网卡地址就行, 后面重新连接热点则无需调用此函数
static void mac_address_callback(void *arg, void *data, WiFi_Status status)
{
  struct ip4_addr ipaddr, netmask, gw;
  uint8_t bss;
  
  if (status != WIFI_STATUS_OK)
  {
    printf("Cannot get MAC address!\n");
    return;
  }
  WiFi_Scan(scan_callback, NULL); // 发送扫描热点的命令 (不阻塞等待回应)
  
  // 将获得的MAC地址传递给lwip
  memcpy(wifi_88w8801_sta.hwaddr, data, 6);
  memcpy(wifi_88w8801_uap.hwaddr, data, 6);
  printf("MAC Addr: %02X:%02X:%02X:%02X:%02X:%02X\n", wifi_88w8801_sta.hwaddr[0], wifi_88w8801_sta.hwaddr[1], wifi_88w8801_sta.hwaddr[2], wifi_88w8801_sta.hwaddr[3], wifi_88w8801_sta.hwaddr[4], wifi_88w8801_sta.hwaddr[5]);
  
  // 初始化连接热点所用的网络接口 (支持DHCP)
  bss = WIFI_BSS_STA;
#if LWIP_DHCP
  // 最后一个参数用netif_input和ethernet_input都是一样的效果
  // netif_input函数内会判断网卡的类型, 如果是以太网卡, 就调用ethernet_input
  // lwip示例代码doc/NO_SYS_SampleCode.c中用的就是netif_input
  netif_add(&wifi_88w8801_sta, IP_ADDR_ANY, IP_ADDR_ANY, IP_ADDR_ANY, &bss, ethernetif_init, netif_input);
  // 倒数第二个参数是网络接口的初始化函数, 倒数第三个参数是给初始化函数传递的自定义数据
#else
  IP4_ADDR(&ipaddr, 192, 168, 43, 15); // IP地址
  IP4_ADDR(&netmask, 255, 255, 255, 0); // 子网掩码
  IP4_ADDR(&gw, 192, 168, 43, 1); // 网关
  netif_add(&wifi_88w8801_sta, &ipaddr, &netmask, &gw, &bss, ethernetif_init, ethernet_input); // 添加WiFi模块到lwip中

#if LWIP_DNS
  IP4_ADDR(&ipaddr, 8, 8, 8, 8); // 首选DNS服务器
  dns_setserver(0, &ipaddr);
  IP4_ADDR(&ipaddr, 8, 8, 4, 4); // 备用DNS服务器
  dns_setserver(1, &ipaddr);
#endif
#endif
  netif_set_default(&wifi_88w8801_sta); // 设为默认网卡
  netif_set_up(&wifi_88w8801_sta); // 允许lwip使用该网卡
  // 注意: 必须要在low_level_init函数中去掉NETIF_FLAG_LINK_UP选项, 因为无线网卡默认是未连接状态
  
  // 初始化创建uAP热点所用的网络接口 (该接口暂不支持DHCP服务器)
  bss = WIFI_BSS_UAP;
  IP4_ADDR(&ipaddr, 192, 168, 20, 1);
  IP4_ADDR(&netmask, 255, 255, 255, 0);
  ip4_addr_set_zero(&gw);
  netif_add(&wifi_88w8801_uap, &ipaddr, &netmask, &gw, &bss, ethernetif_init, netif_input);
  netif_set_up(&wifi_88w8801_uap);
}

static void stop_callback(void *arg, void *data, WiFi_Status status)
{
  char *s1 = (char *)arg;
  char *s2 = s1 + strlen(s1) + 1;
  WiFi_CommandHeader *header = (WiFi_CommandHeader *)data;
  if (status == WIFI_STATUS_OK)
  {
    if (header->result == CMD_STATUS_SUCCESS)
    {
      netif_set_link_down(&wifi_88w8801_sta);
      printf("%s %s!\n", s1, s2);
    }
    else if (header->result == CMD_STATUS_UNSUPPORTED)
      printf("Operation is not supported!\n");
  }
  else
    printf("%s not %s!\n", s1, s2);
}

int main(void)
{
  GPIO_InitTypeDef gpio;
  USART_InitTypeDef usart;
  uint8_t data;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
  
  // 串口发送引脚PA9设为复用推挽输出, 串口接收引脚PA10保持默认的浮空输入
  gpio.GPIO_Mode = GPIO_Mode_AF_PP;
  gpio.GPIO_Pin = GPIO_Pin_9;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &gpio);
  
  USART_StructInit(&usart);
  usart.USART_BaudRate = 115200;
  USART_Init(USART1, &usart);
  USART_Cmd(USART1, ENABLE);
  printf("STM32F103ZE SDIO 88W8801\n");
  
  rtc_init(); // 初始化RTC时钟
  
  WiFi_Init(); // 初始化WiFi模块
  WiFi_GetMACAddress(mac_address_callback, NULL); // 发送获取网卡地址的命令, 命令发送成功后函数立即返回, 不阻塞等待命令回应
  // 收到命令回应后会调用指定的回调函数, 回调函数是在主循环的WiFi_Input函数内部被调用的
  
  lwip_init();
  
  netbiosns_init();
  netbiosns_set_name("STM32F103ZE"); // 计算机名
  
  httpd_init();
  tcp_tester_init(); // TCP测速服务
  udp_tester_init(); // UDP测速服务
  while (1)
  {
    // WiFi模块中断和超时处理
    if (SDIO_GetFlagStatus(SDIO_FLAG_SDIOIT) == SET)
    {
      SDIO_ClearFlag(SDIO_FLAG_SDIOIT);
      WiFi_Input();
    }
    else
      WiFi_CheckTimeout();
    
    // lwip协议栈定时处理函数
    sys_check_timeouts();
    
    // 显示DHCP获取到的IP地址
#if LWIP_DHCP
    display_ip();
#endif
    
    // 串口调试命令
    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
    {
      data = USART_ReceiveData(USART1);
      switch (data)
      {
        case 'A':
          // 离开ADHOC网络
          WiFi_StopADHOC(stop_callback, "ADHOC\0stopped"); // 如果当前未处于ADHOC模式, 则命令不会收到回应, 但最终回调函数肯定会调用
          break;
        case 'D':
          // 取消关联热点
          WiFi_Deauthenticate(3, stop_callback, "Connection\0closed"); // LEAVING_NETWORK_DEAUTH
          break;
        case 'd':
          // WiFi模块规定, 如果程序一直没有读取新收到的帧, 则不会接收新的帧
          // 这个命令用于强制丢弃WiFi缓冲区中的数据
          WiFi_DiscardData();
          break;
        case 'k':
          WiFi_ShowKeyMaterials();
          break;
#if LWIP_DNS
        case 'n':
          // DNS测试
          dns_test();
          break;
#endif
        case 'R':
          // 重新连接热点
          if (WiFi_IsConnected()) // 这个函数判断的是是否已连上热点, 而netif_is_link_up函数判断的是是否已连上热点且完成了认证
            printf("Please disconnect first!\n");
          else
          {
            printf("Reconnecting...\n");
            associate_example();
          }
          break;
        case 's':
          // 显示状态寄存器的值
          printf("SDIO->STA=0x%08x, ", SDIO->STA);
          printf("CARDSTATUS=%d, INTSTATUS=%d\n", WiFi_LowLevel_ReadReg(1, WIFI_CARDSTATUS), WiFi_LowLevel_ReadReg(1, WIFI_INTSTATUS));
          printf("RDBITMAP=0x%04x, WRBITMAP=0x%04x\n", WiFi_GetReadPortStatus(), WiFi_GetWritePortStatus());
          break;
        case 't':
          // 显示当前时间
          display_time();
          break;
        case 'T':
          // 统计帧的发送情况
          WiFi_PrintTxPacketStatus();
          break;
      }
    }
  }
}

void HardFault_Handler(void)
{
  printf("Hard Error!\n");
  while (1);
}
