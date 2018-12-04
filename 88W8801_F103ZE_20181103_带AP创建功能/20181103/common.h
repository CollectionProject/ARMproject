// lwip��struct netif��state��Աָ����û��Զ�������
// application state����˼�������������(��ṹ��)�йص�Ӧ�ó����Զ�������
struct ethernetif {
  struct eth_addr *ethaddr;
  
  uint8_t bss; // ����ӿ����ڵĻ�������
  const void *data; // �յ�������
  int16_t port; // �յ����������ڵ�ͨ����
};

void delay(uint16_t nms);
void dump_data(const void *data, uint32_t len);
uint8_t isempty(const void *data, uint32_t len);
uint32_t rtc_divider(void);
void rtc_init(void);
uint32_t sys_now(void);
