#include <lwip/tcp.h>
#include <lwip/dns.h>
#include <lwip/sys.h> // sys_now函数所在的头文件
#include <lwip/udp.h>
#include <string.h>
#include <time.h>

/*** [DNS TEST] ***/
err_t test_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  uintptr_t count = (uintptr_t)arg;
  //struct pbuf *q;
  if (p != NULL)
  {
    // 输出数据
    printf("%d bytes received!\n", p->tot_len);
    //for (q = p; q != NULL; q = q->next)
    //  fwrite(q->payload, 1, q->len, stdout);
    
    // 统计收到的字节数
    count += p->tot_len;
    tcp_arg(tpcb, (void *)count);
    
    // 下面这两句缺一不可！！！
    tcp_recved(tpcb, p->tot_len); // 拓宽发送窗口
    pbuf_free(p); // 释放接收到的数据
  }
  else
  {
    // 当p=NULL时, 表示对方已经关闭了连接
    err = tcp_close(tpcb);
    printf("Connection is closed! err=%d, count=%d\n", err, count);
  }
  return ERR_OK;
}

static err_t test_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  char *request = "GET /news/?group=lwip HTTP/1.1\r\nHost: savannah.nongnu.org\r\n\r\n";
  printf("Connected! err=%d\n", err);
  tcp_recv(tpcb, test_recv);
  
  // 发送HTTP请求头
  tcp_write(tpcb, request, strlen(request), 0);
  tcp_output(tpcb);
  return ERR_OK;
}

static void test_err(void *arg, err_t err)
{
  printf("Connection error! code=%d\n", err);
}

void connect_test(const ip_addr_t *ipaddr)
{
  err_t err;
  struct tcp_pcb *tpcb;
  
  tpcb = tcp_new();
  if (tpcb == NULL)
  {
    printf("\atcp_new failed!\n");
    return;
  }
  tcp_err(tpcb, test_err);
  
  printf("Connecting to %s...\n", ipaddr_ntoa(ipaddr));
  err = tcp_connect(tpcb, ipaddr, 80, test_connected);
  if (err != ERR_OK)
  {
    tcp_close(tpcb);
    printf("Connection failed! err=%d\n", err);
  }
}

#if LWIP_DNS
static void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
  if (ipaddr != NULL)
  {
    printf("DNS Found IP: %s\n", ipaddr_ntoa(ipaddr));
    connect_test(ipaddr);
  }
  else
    printf("DNS Not Found IP!\n");
}

void dns_test(void)
{
  ip_addr_t dnsip;
  err_t err = dns_gethostbyname("savannah.nongnu.org", &dnsip, dns_found, NULL);
  if (err == ERR_OK)
  {
    printf("In cache! IP: %s\n", ipaddr_ntoa(&dnsip));
    connect_test(&dnsip);
  }
  else if (err == ERR_INPROGRESS)
    printf("Not in cache!\n"); // 缓存中没有时需要等待DNS解析完毕在dns_found回调函数中返回结果
  else
    printf("dns_gethostbyname failed! err=%d\n", err);
}
#endif

/*** [TCP SPEED TEST] ***/
static uint8_t tcp_tester_buffer[1500];

static err_t tcp_tester_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static void tcp_tester_err(void *arg, err_t err);
static err_t tcp_tester_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_tester_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);

static err_t tcp_tester_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  printf("TCP tester accepted %s:%d!\n", ipaddr_ntoa(&newpcb->remote_ip), newpcb->remote_port);
  tcp_err(newpcb, tcp_tester_err);
  tcp_recv(newpcb, tcp_tester_recv);
  tcp_sent(newpcb, tcp_tester_sent);
  
  tcp_tester_sent(NULL, newpcb, 0); // 发出第一个数据包
  return ERR_OK;
}

static void tcp_tester_err(void *arg, err_t err)
{
  // 客户端读完所有数据才关闭连接, 只调用recv(p=NULL)函数显示closed
  // 服务器端已发送完所有数据并收到客户端的确认, 但客户端未调用recv函数读完所有数据就关闭连接, 只调用err(err=-14)函数, 不调用recv(p=NULL)函数
  // 服务器端发送数据还没发完, 或发送完了但只收到客户端的部分确认, 客户端就关闭连接, 则先调用recv(p=NULL)函数, 然后再调用err(err=-14)函数
  
  // PC客户端接收到数据后是先把数据放到缓存里面, 立即发出确认, 并不是非要调用recv函数后才发确认
  // 例如, 服务器发送完1072字节数据后, 客户端只确认收到536字节数据, 则recv和err函数会同时调用
  
  // 所以, 由于close和error事件可能会同时触发, 释放arg指向的内存的时候必须要小心
  // 必须避免内存先在recv(p=NULL)里面释放后, 再在err里面再次释放的问题!
  printf("TCP tester error! err=%d\n", err);
}

static err_t tcp_tester_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  if (p != NULL)
  {
    printf("TCP tester received %d bytes from %s:%d!\n", p->tot_len, ipaddr_ntoa(&tpcb->remote_ip), tpcb->remote_port);
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    
    tcp_arg(tpcb, (void *)1); // 停止发送数据
  }
  else
  {
    printf("TCP tester client %s:%d closed!\n", ipaddr_ntoa(&tpcb->remote_ip), tpcb->remote_port);
    tcp_close(tpcb);
  }
  return ERR_OK;
}

static err_t tcp_tester_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  uint16_t size;
  if (arg == NULL)
  {
    size = tcp_sndbuf(tpcb);
    if (size > sizeof(tcp_tester_buffer))
      size = sizeof(tcp_tester_buffer);
    tcp_write(tpcb, tcp_tester_buffer, size, 0);
  }
  return ERR_OK;
}

void tcp_tester_init(void)
{
  struct tcp_pcb *tpcb = tcp_new();
  tcp_bind(tpcb, IP_ADDR_ANY, 24001);
  tpcb = tcp_listen(tpcb);
  tcp_accept(tpcb, tcp_tester_accept);
}

/*** [UDP TEST] ***/
static void udp_tester_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  struct pbuf *q;
  uint16_t i;
  
  if (p != NULL)
  {
    pbuf_free(p); // 丢弃收到的UDP数据包内容
    
    // 每从客户端收到一个数据包, 都发送一批数据包作为回应
    q = pbuf_alloc(PBUF_TRANSPORT, 1300, PBUF_ROM);
    if (q != NULL)
    {
      printf("Sending UDP packets...\n");
      q->payload = tcp_tester_buffer;
      for (i = 0; i < 1024; i++)
        udp_sendto(pcb, q, addr, port);
      pbuf_free(q);
    }
  }
}

void udp_tester_init(void)
{
  struct udp_pcb *upcb = udp_new();
  udp_bind(upcb, IP_ADDR_ANY, 24002);
  udp_recv(upcb, udp_tester_recv, NULL);
}

/*** [OTHER TEST] ***/
// 下面这个函数演示了如何使用C库将RTC时间值转换为日期时间格式, 不用的话可以删掉
void display_time(void)
{
  char str[30];
  struct tm *ptm;
  time_t sec;
  
  time(&sec);
  ptm = localtime(&sec);
  strftime(str, sizeof(str), "%Y-%m-%d %H:%M:%S", ptm);
  
  printf("%s\n", str);
  printf("sys_now()=%d\n", sys_now());
}
