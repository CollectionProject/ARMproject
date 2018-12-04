#include <lwip/apps/httpd.h> // http������
#include <lwip/apps/netbiosns.h> // NetBIOS����
#include <lwip/dhcp.h> // DHCP�ͻ���
#include <lwip/dns.h> // DNS�ͻ���
#include <lwip/init.h> // lwip_init�������ڵ�ͷ�ļ�
#include <lwip/timeouts.h> // sys_check_timeouts�������ڵ�ͷ�ļ�
#include <netif/ethernet.h> // ethernet_input��������ͷ�ļ�
#include <stm32f10x.h>
#include <string.h>
#include "common.h"
#include "WiFi.h"
#include "wifi_test.h"

//#define METHOD 1 // ֻ�����ȵ�
//#define METHOD 2 // ֻ�����ȵ�
#define METHOD 3 // �����ȵ�, Ȼ������·���� (�Ƽ�)

#define AUTO_RECONNECT // �����Զ�����

// ����������λ��ethernetif.c��, ��û��ͷ�ļ�����
err_t ethernetif_init(struct netif *netif);
void ethernetif_input(struct netif *netif);

// ���ļ��õ��ĺ�������
void associate_example(void);

static struct netif wifi_88w8801_sta;
static struct netif wifi_88w8801_uap;

int fputc(int ch, FILE *fp)
{
  if (fp == stdout)
  {
    if (ch == '\n')
    {
      while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); // �ȴ�ǰһ�ַ��������
      USART_SendData(USART1, '\r');
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, ch);
  }
  return ch;
}

/* ��ʾDHCP�����IP��ַ */
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

/* WiFi��֤�ɹ��ص����� */
void WiFi_AuthenticationCompleteHandler(void)
{
  printf("Authenticated!\n");
  netif_set_link_up(&wifi_88w8801_sta);
  
#if LWIP_DHCP
  if (netif_dhcp_data(&wifi_88w8801_sta) == NULL)
    dhcp_start(&wifi_88w8801_sta);
#endif
}

/* WiFi�¼��ص����� */
void WiFi_EventHandler(const WiFi_Event *event)
{
#ifdef AUTO_RECONNECT
  uint8_t up = netif_is_link_up(&wifi_88w8801_sta);
#endif
  
  switch (event->event_id)
  {
    case 3:
      // �ղ����ź� (������ֻ��ȵ㽨�����Ӻ�, ���ֻ�����), WiFiģ�鲻���Զ�����
      printf("Beacon Loss/Link Loss\n");
      netif_set_link_down(&wifi_88w8801_sta);
      break;
    case 4:
      // Ad-Hoc�����в�ֹ1�����, �������������˱仯
      printf("The number of stations in this ad hoc newtork has changed!\n");
      netif_set_link_up(&wifi_88w8801_sta);
      break;
    case 8:
      // ��֤�ѽ�� (�����ֻ��ر����ȵ�, ��������·����������֤ʧ�ܶ��Զ��Ͽ�����)
      printf("Deauthenticated!\n");
      netif_set_link_down(&wifi_88w8801_sta);
      break;
    case 9:
      // ����˹���
      printf("Disassociated!\n");
      netif_set_link_down(&wifi_88w8801_sta);
      break;
    case 17:
      // Ad-Hoc������ֻʣ�����
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
  
  // �����Զ�����
#ifdef AUTO_RECONNECT
  if (up && !netif_is_link_up(&wifi_88w8801_sta))
  {
    printf("Reconnecting...\n");
    associate_example(); // ֻ��Ҫ��������������¹����ȵ������, ����Ҫ���»�ȡMAC��ַ
  }
#endif
}

/* WiFiģ���յ��µ�����֡ */
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
  ethernetif_input(netif); // ����lwip����
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

// ����·�����ȵ�ʾ������
void microap_example(void)
{
  WiFi_Connection conn;
  
  conn.security = WIFI_SECURITYTYPE_NONE;
  conn.ssid = NULL; // Ĭ��SSID
  //conn.ssid = "Hello World"; // �Զ���SSID
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

// ����·�����ȵ��ʾ������
void associate_example(void)
{
  WiFi_Connection conn;
  //WiFi_WEPKey wepkey = {0}; // δʹ�õĳ�Ա������Ϊ0
  
  /*
  conn.security = WIFI_SECURITYTYPE_WEP;
  conn.ssid = "Oct1158-2";
  conn.password = &wepkey;
  wepkey.keys[0] = "1234567890123";
  wepkey.index = 0; // ��Կ��ű���Ҫ��ȷ
  WiFi_AssociateEx(&conn, WIFI_AUTH_MODE_OPEN, -1, associate_callback, NULL); // ����ϵͳ��ʽ
  //WiFi_AssociateEx(&conn, WIFI_AUTH_MODE_SHARED, -1, associate_callback, NULL); // ������Կ��ʽ
  */
  
  /*
  conn.security = WIFI_SECURITYTYPE_NONE;
  conn.ssid = "vivo Y29L";
  WiFi_AssociateEx(&conn, WIFI_AUTH_MODE_OPEN, -1, associate_callback, NULL);
  */
  
  ///*
  conn.security = WIFI_SECURITYTYPE_WPA; // WPA��WPA2������ʹ�ô�ѡ��
  conn.ssid = "Oct1158-2";
  conn.password = "wi5Nkj9QHBigqRmcXox86u544BNIzM9r";
  WiFi_AssociateEx(&conn, WIFI_AUTH_MODE_OPEN, -1, associate_callback, NULL); // ����ʹ��WIFI_AUTH_MODE_OPENѡ��
  //*/
}

static void adhoc_callback(void *arg, void *data, WiFi_Status status)
{
  if (status == WIFI_STATUS_OK)
    printf("ADHOC %sed!\n", (char *)arg);
  else
    printf("Cannot %s ADHOC!\n", (char *)arg);
}

// ����������ADHOC���ȵ��ʾ������
void adhoc_example(void)
{
  WiFi_Connection conn;
  WiFi_WEPKey wepkey = {0}; // δʹ�õĳ�Ա������Ϊ0
  
  /*
  conn.security = WIFI_SECURITYTYPE_WEP;
  conn.ssid = "Octopus-WEP";
  conn.password = &wepkey;
  wepkey.keys[0] = "3132333435";
  wepkey.index = 0; // ��Χ: 0~3
  WiFi_JoinADHOCEx(&conn, -1, adhoc_callback, "join");
  */
  
  ///*
  // ע��: �������������������Ƿ���ȷ����������, ��ֻ����ȷ������ſ���ͨ��
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
  adhoc_example(); // 88W8801��֧��Ad-Hoc, ����ʹ�ô�ѡ��
#endif
}

// ��ȡ����MAC��ַ�ɹ���, ��������������ӵ�lwip��, ���ݲ���������Ϊ"������"״̬
// ֻ��Ҫ�ڵ�Ƭ����λ��ʱ���ȡһ��������ַ����, �������������ȵ���������ô˺���
static void mac_address_callback(void *arg, void *data, WiFi_Status status)
{
  struct ip4_addr ipaddr, netmask, gw;
  uint8_t bss;
  
  if (status != WIFI_STATUS_OK)
  {
    printf("Cannot get MAC address!\n");
    return;
  }
  WiFi_Scan(scan_callback, NULL); // ����ɨ���ȵ������ (�������ȴ���Ӧ)
  
  // ����õ�MAC��ַ���ݸ�lwip
  memcpy(wifi_88w8801_sta.hwaddr, data, 6);
  memcpy(wifi_88w8801_uap.hwaddr, data, 6);
  printf("MAC Addr: %02X:%02X:%02X:%02X:%02X:%02X\n", wifi_88w8801_sta.hwaddr[0], wifi_88w8801_sta.hwaddr[1], wifi_88w8801_sta.hwaddr[2], wifi_88w8801_sta.hwaddr[3], wifi_88w8801_sta.hwaddr[4], wifi_88w8801_sta.hwaddr[5]);
  
  // ��ʼ�������ȵ����õ�����ӿ� (֧��DHCP)
  bss = WIFI_BSS_STA;
#if LWIP_DHCP
  // ���һ��������netif_input��ethernet_input����һ����Ч��
  // netif_input�����ڻ��ж�����������, �������̫����, �͵���ethernet_input
  // lwipʾ������doc/NO_SYS_SampleCode.c���õľ���netif_input
  netif_add(&wifi_88w8801_sta, IP_ADDR_ANY, IP_ADDR_ANY, IP_ADDR_ANY, &bss, ethernetif_init, netif_input);
  // �����ڶ�������������ӿڵĳ�ʼ������, ���������������Ǹ���ʼ���������ݵ��Զ�������
#else
  IP4_ADDR(&ipaddr, 192, 168, 43, 15); // IP��ַ
  IP4_ADDR(&netmask, 255, 255, 255, 0); // ��������
  IP4_ADDR(&gw, 192, 168, 43, 1); // ����
  netif_add(&wifi_88w8801_sta, &ipaddr, &netmask, &gw, &bss, ethernetif_init, ethernet_input); // ���WiFiģ�鵽lwip��

#if LWIP_DNS
  IP4_ADDR(&ipaddr, 8, 8, 8, 8); // ��ѡDNS������
  dns_setserver(0, &ipaddr);
  IP4_ADDR(&ipaddr, 8, 8, 4, 4); // ����DNS������
  dns_setserver(1, &ipaddr);
#endif
#endif
  netif_set_default(&wifi_88w8801_sta); // ��ΪĬ������
  netif_set_up(&wifi_88w8801_sta); // ����lwipʹ�ø�����
  // ע��: ����Ҫ��low_level_init������ȥ��NETIF_FLAG_LINK_UPѡ��, ��Ϊ��������Ĭ����δ����״̬
  
  // ��ʼ������uAP�ȵ����õ�����ӿ� (�ýӿ��ݲ�֧��DHCP������)
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
  
  // ���ڷ�������PA9��Ϊ�����������, ���ڽ�������PA10����Ĭ�ϵĸ�������
  gpio.GPIO_Mode = GPIO_Mode_AF_PP;
  gpio.GPIO_Pin = GPIO_Pin_9;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &gpio);
  
  USART_StructInit(&usart);
  usart.USART_BaudRate = 115200;
  USART_Init(USART1, &usart);
  USART_Cmd(USART1, ENABLE);
  printf("STM32F103ZE SDIO 88W8801\n");
  
  rtc_init(); // ��ʼ��RTCʱ��
  
  WiFi_Init(); // ��ʼ��WiFiģ��
  WiFi_GetMACAddress(mac_address_callback, NULL); // ���ͻ�ȡ������ַ������, ����ͳɹ�������������, �������ȴ������Ӧ
  // �յ������Ӧ������ָ���Ļص�����, �ص�����������ѭ����WiFi_Input�����ڲ������õ�
  
  lwip_init();
  
  netbiosns_init();
  netbiosns_set_name("STM32F103ZE"); // �������
  
  httpd_init();
  tcp_tester_init(); // TCP���ٷ���
  udp_tester_init(); // UDP���ٷ���
  while (1)
  {
    // WiFiģ���жϺͳ�ʱ����
    if (SDIO_GetFlagStatus(SDIO_FLAG_SDIOIT) == SET)
    {
      SDIO_ClearFlag(SDIO_FLAG_SDIOIT);
      WiFi_Input();
    }
    else
      WiFi_CheckTimeout();
    
    // lwipЭ��ջ��ʱ������
    sys_check_timeouts();
    
    // ��ʾDHCP��ȡ����IP��ַ
#if LWIP_DHCP
    display_ip();
#endif
    
    // ���ڵ�������
    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
    {
      data = USART_ReceiveData(USART1);
      switch (data)
      {
        case 'A':
          // �뿪ADHOC����
          WiFi_StopADHOC(stop_callback, "ADHOC\0stopped"); // �����ǰδ����ADHOCģʽ, ��������յ���Ӧ, �����ջص������϶������
          break;
        case 'D':
          // ȡ�������ȵ�
          WiFi_Deauthenticate(3, stop_callback, "Connection\0closed"); // LEAVING_NETWORK_DEAUTH
          break;
        case 'd':
          // WiFiģ��涨, �������һֱû�ж�ȡ���յ���֡, �򲻻�����µ�֡
          // �����������ǿ�ƶ���WiFi�������е�����
          WiFi_DiscardData();
          break;
        case 'k':
          WiFi_ShowKeyMaterials();
          break;
#if LWIP_DNS
        case 'n':
          // DNS����
          dns_test();
          break;
#endif
        case 'R':
          // ���������ȵ�
          if (WiFi_IsConnected()) // ��������жϵ����Ƿ��������ȵ�, ��netif_is_link_up�����жϵ����Ƿ��������ȵ����������֤
            printf("Please disconnect first!\n");
          else
          {
            printf("Reconnecting...\n");
            associate_example();
          }
          break;
        case 's':
          // ��ʾ״̬�Ĵ�����ֵ
          printf("SDIO->STA=0x%08x, ", SDIO->STA);
          printf("CARDSTATUS=%d, INTSTATUS=%d\n", WiFi_LowLevel_ReadReg(1, WIFI_CARDSTATUS), WiFi_LowLevel_ReadReg(1, WIFI_INTSTATUS));
          printf("RDBITMAP=0x%04x, WRBITMAP=0x%04x\n", WiFi_GetReadPortStatus(), WiFi_GetWritePortStatus());
          break;
        case 't':
          // ��ʾ��ǰʱ��
          display_time();
          break;
        case 'T':
          // ͳ��֡�ķ������
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
