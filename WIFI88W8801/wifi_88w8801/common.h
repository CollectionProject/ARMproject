// lwip��struct netif��state��Աָ����û��Զ�������
// application state����˼�������������(��ṹ��)�йص�Ӧ�ó����Զ�������
struct ethernetif {
  struct eth_addr *ethaddr;
  
  const void *data; // �յ�������
  int16_t port; // �յ����������ڵ�ͨ����
};

void delay(uint16_t nms);
void dump_data(const void *data, uint32_t len);
uint8_t isempty(const void *data, uint32_t len);
uint32_t sys_now(void);
void sys_now_init(void);
