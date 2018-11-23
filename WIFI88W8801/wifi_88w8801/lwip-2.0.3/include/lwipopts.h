#define NO_SYS 1 // 无操作系统

#define LWIP_NETCONN 0
#define LWIP_SOCKET 0
#define LWIP_STATS 0

#define MEM_ALIGNMENT 4 // STM32单片机是32位的单片机, 因此是4字节对齐的

#define SYS_LIGHTWEIGHT_PROT 0 // 不进行临界区保护

// 配置DHCP
#define LWIP_DHCP 1
#define LWIP_NETIF_HOSTNAME 1

// 配置DNS
#define LWIP_DNS 1
#define LWIP_RAND() ((u32_t)rand())
