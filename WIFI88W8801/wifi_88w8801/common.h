// lwip中struct netif的state成员指向的用户自定义数据
// application state的意思就是与这个对象(或结构体)有关的应用程序自定义数据
struct ethernetif {
  struct eth_addr *ethaddr;
  
  const void *data; // 收到的数据
  int16_t port; // 收到的数据所在的通道号
};

void delay(uint16_t nms);
void dump_data(const void *data, uint32_t len);
uint8_t isempty(const void *data, uint32_t len);
uint32_t sys_now(void);
void sys_now_init(void);
