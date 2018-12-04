/* 选项 */
#define WIFI_DATAPORTS_NUM 11 // 数据通道个数
#define WIFI_DEFAULT_MAXRETRY 2 // 命令帧默认最大尝试发送的次数
#define WIFI_DEFAULT_TIMEOUT_CMDRESP 1000 // WiFi命令帧回应的超时时间(ms): 如果发现命令无论怎么重传都收不到回应, 就要考虑增大超时时间
#define WIFI_DEFAULT_TIMEOUT_DATAACK 120000 // WiFi数据帧确认的超时时间(ms): 这个要尽可能大
#define WIFI_DISPLAY_PACKET_SIZE // 显示收发的数据包的大小
//#define WIFI_DISPLAY_PACKET_RX // 显示收到的数据包内容
//#define WIFI_DISPLAY_PACKET_RXRATES // 显示数据包的接收速率
//#define WIFI_DISPLAY_PACKET_TX // 显示发送的数据包内容
#define WIFI_DISPLAY_RESPTIME // 显示命令帧和数据帧从发送到收到确认和回应所经过的时间
#define WIFI_HIGHSPEED // 采用SDIO高速模式
#define WIFI_TIMEOUT_SCAN 30000 // 热点扫描命令的超时时间(ms)
#define WIFI_USEDMA // SDIO采用DMA方式收发数据

// 预定义的两个基本服务集
#define WIFI_BSS_STA WIFI_BSS(WIFI_BSSTYPE_STA, 0) // WiFi模块连接的BSS
#define WIFI_BSS_UAP WIFI_BSS(WIFI_BSSTYPE_UAP, 1) // WiFi模块自身创建的BSS

#define WIFI_FIRMWAREAREA_ADDR 0x08041800 // 固件存储区首地址

#ifdef WIFI_FIRMWAREAREA_ADDR
// 固件存储区的格式: 固件大小(4B)+固件内容+CRC校验码(4B)
#define WIFI_FIRMWARE_SIZE (*(const uint32_t *)WIFI_FIRMWAREAREA_ADDR)
#define WIFI_FIRMWARE_ADDR ((const uint8_t *)WIFI_FIRMWAREAREA_ADDR + 4)
#else
// 这里sd表示这两个固件是SDIO接口专用的, uap表示micro AP, sta表示station
// 取消WIFI_FIRMWAREAREA_ADDR宏定义后, 需要把sd8801_uapsta.c添加到工程中来, 并确保变量的声明已添加__attribute__((aligned))
extern const unsigned char firmware_sd8801[255536];
#define WIFI_FIRMWARE_SIZE sizeof(firmware_sd8801)
#define WIFI_FIRMWARE_ADDR firmware_sd8801
#endif

/* WiFi寄存器及位定义 */
#define _BV(n) (1u << (n))

// 6.9 Card Common Control Registers (CCCR)
#define SDIO_CCCR_IOEN 0x02
#define SDIO_CCCR_IOEN_IOE1 _BV(1)

#define SDIO_CCCR_IORDY 0x03
#define SDIO_CCCR_IORDY_IOR1 _BV(1)

#define SDIO_CCCR_INTEN 0x04
#define SDIO_CCCR_INTEN_IENM _BV(0)
#define SDIO_CCCR_INTEN_IEN1 _BV(1)

#define SDIO_CCCR_BUSIFCTRL 0x07 // Bus Interface Control
#define SDIO_CCCR_BUSIFCTRL_BUSWID_1Bit 0
#define SDIO_CCCR_BUSIFCTRL_BUSWID_4Bit 0x02
#define SDIO_CCCR_BUSIFCTRL_BUSWID_8Bit 0x03

// 中断使能寄存器
#define WIFI_INTMASK 0x02 // Host Interrupt Mask
#define WIFI_INTMASK_HOSTINTMASK 0x03 // enable/disable SDU to SD host interrupt

// 中断标志位寄存器
#define WIFI_INTSTATUS 0x03 // Host Interrupt Status
#define WIFI_INTSTATUS_ALL 0x03
#define WIFI_INTSTATUS_DNLD _BV(1) // Download Host Interrupt Status
#define WIFI_INTSTATUS_UPLD _BV(0) // Upload Host Interrupt Status (可随时手动清除, 无论UPLDCARDRDY是否为1)

// 表示哪些通道有数据可以读取
#define WIFI_RDBITMAPL 0x04
#define WIFI_RDBITMAPU 0x05

// 表示哪些通道可以发送数据
#define WIFI_WRBITMAPL 0x06
#define WIFI_WRBITMAPU 0x07

// 通道0是命令和事件通道, 对应的表示数据大小的寄存器是0x08和0x09
#define WIFI_RDLENP0L 0x08 // Read length for port 0
#define WIFI_RDLENP0U 0x09
// 通道1~15是数据通道, 对应的表示数据大小的寄存器是0x0a~0x27

// 卡状态寄存器 (不推荐使用)
#define WIFI_CARDSTATUS 0x30 // Card Status
//#define WIFI_CARDSTATUS_IOREADY _BV(3) // I/O Ready Indicator
//#define WIFI_CARDSTATUS_CISCARDRDY _BV(2) // Card Information Structure Card Ready
//#define WIFI_CARDSTATUS_UPLDCARDRDY _BV(1) // Upload Card Ready (CMD53读写命令均会清除该位)
//#define WIFI_CARDSTATUS_DNLDCARDRDY _BV(0) // Download Card Ready

// 固件数据发送量寄存器
#define WIFI_SQREADBASEADDR0 0x40 // required firmware length
#define WIFI_SQREADBASEADDR1 0x41

// 固件状态寄存器
#define WIFI_SCRATCH0_0 0x60 // 88W8801 firmware status
#define WIFI_SCRATCH0_1 0x61

#define WIFI_SCRATCH0_2 0x62
#define WIFI_SCRATCH0_3 0x63

// 表示通道0的CMD53地址的寄存器
#define WIFI_IOPORT0 0x78
#define WIFI_IOPORT1 0x79
#define WIFI_IOPORT2 0x7a

/* WiFi模块用到的枚举类型 */
// Authentication Type to be used to authenticate with AP
typedef enum
{
  WIFI_AUTH_MODE_OPEN = 0x00,
  WIFI_AUTH_MODE_SHARED = 0x01,
  WIFI_AUTH_MODE_NETWORK_EAP = 0x80
} WiFi_AuthenticationType;

// BSS type
typedef enum
{
  // 用于热点扫描命令
  WIFI_BSS_INFRASTRUCTURE = 0x01,
  WIFI_BSS_INDEPENDENT = 0x02,
  WIFI_BSS_ANY = 0x03,
  
  // 用于命令帧、数据帧和事件帧的头部
  WIFI_BSSTYPE_STA = 0,
  WIFI_BSSTYPE_UAP = 1
} WiFi_BSSType;

// 部分WiFi命令的action字段
typedef enum
{
  WIFI_ACT_GET = 0,
  WIFI_ACT_SET = 1,
  WIFI_ACT_ADD = 2,
  WIFI_ACT_BITWISE_SET = 2,
  WIFI_ACT_BITWISE_CLR = 3,
  WIFI_ACT_REMOVE = 4
} WiFi_CommandAction;

// WiFi命令列表
typedef enum
{
  CMD_802_11_SCAN = 0x0006, // Starts the scan process
  CMD_802_11_ASSOCIATE = 0x0012, // Initiate an association with the AP
  CMD_802_11_SET_WEP = 0x0013, // Configures the WEP keys
  CMD_802_11_DEAUTHENTICATE = 0x0024, // Starts de-authentication process with the AP
  CMD_MAC_CONTROL = 0x0028, // Controls hardware MAC
  CMD_802_11_AD_HOC_START = 0x002b, // Starts an Ad-Hoc network
  CMD_802_11_AD_HOC_JOIN = 0x002c, // Join an Ad-Hoc network
  CMD_802_11_AD_HOC_STOP = 0x0040, // Stops Ad-Hoc Network
  CMD_802_11_MAC_ADDR = 0x004d, // WLAN MAC address
  CMD_802_11_KEY_MATERIAL = 0x005e, // Gets/sets key material used to do Tx encryption or Rx decryption
  CMD_802_11_BG_SCAN_CONFIG = 0x006b, // Gets/sets background scan configuration
  CMD_802_11_BG_SCAN_QUERY = 0x006c, // Gets background scan results
  CMD_802_11_SUBSCRIBE_EVENT = 0x0075, // Subscribe to events and set thresholds
  CMD_TX_PKT_STATS = 0x008d, // Reports the current Tx packet status
  APCMD_SYS_CONFIGURE = 0x00b0, // Gets/sets AP configuration parameters
  APCMD_BSS_START = 0x00b1, // Starts BSS
  CMD_SUPPLICANT_PMK = 0x00c4 // Configures embedded supplicant information
} WiFi_CommandList;

// WiFi命令执行结果
typedef enum
{
  CMD_STATUS_SUCCESS = 0x0000, // No error
  CMD_STATUS_ERROR = 0x0001, // Command failed
  CMD_STATUS_UNSUPPORTED = 0x0002 // Command is not supported (result=2表示WiFi模块不支持此命令)
} WiFi_CommandResult;

// WiFi密钥类型
typedef enum
{
  WIFI_KEYTYPE_WEP = 0,
  WIFI_KEYTYPE_TKIP = 1,
  WIFI_KEYTYPE_AES = 2
} WiFi_KeyType;

// Table 45: IEEE 802.11 Standard IE Translated to Marvell IE
// PDF中的表45有一些拼写错误, MRVIIE应该改为MRVLIE
typedef enum
{
  WIFI_MRVLIETYPES_SSIDPARAMSET = 0x0000,
  WIFI_MRVLIETYPES_RATESPARAMSET = 0x0001,
  WIFI_MRVLIETYPES_PHYPARAMDSSET = 0x0003,
  WIFI_MRVLIETYPES_CFPARAMSET = 0x0004,
  WIFI_MRVLIETYPES_IBSSPARAMSET = 0x0006,
  WIFI_MRVLIETYPES_RSNPARAMSET = 0x0030,
  WIFI_MRVLIETYPES_VENDORPARAMSET = 0x00dd,
  WIFI_MRVLIETYPES_KEYPARAMSET = 0x0100,
  WIFI_MRVLIETYPES_CHANLISTPARAMSET = 0x0101,
  WIFI_MRVLIETYPES_TSFTIMESTAMP = 0x0113,
  WIFI_MRVLIETYPES_AUTHTYPE = 0x011f,
  WIFI_MRVLIETYPES_WPAPASSPHRASE = 0x013c,
  WIFI_MRVLIETYPES_PASSPHRASE = 0x0145,
  WIFI_MRVLIETYPES_KEYPARAMSETV2 = 0x019c,
  WIFI_MRVLIETYPES_APMACADDR = 0x012b,
  WIFI_MRVLIETYPES_BEACONPERIOD = 0x012c,
  WIFI_MRVLIETYPES_DTIMPERIOD = 0x012d,
  WIFI_MRVLIETYPES_APTXPOWER = 0x012f,
  WIFI_MRVLIETYPES_APBCASTSSIDCTRL = 0x0130,
  WIFI_MRVLIETYPES_PREAMBLECTRL = 0x0131,
  WIFI_MRVLIETYPES_ANTENNACTRL = 0x0132,
  WIFI_MRVLIETYPES_RTSTHRESHOLD = 0x0133,
  WIFI_MRVLIETYPES_TXDATARATE = 0x0135,
  WIFI_MRVLIETYPES_PKTFWDCTRL = 0x0136,
  WIFI_MRVLIETYPES_STAMACFILTER = 0x0138,
  WIFI_MRVLIETYPES_STAAGEOUT = 0x0139,
  WIFI_MRVLIETYPES_WEPKEY = 0x013b,
  WIFI_MRVLIETYPES_FRAGTHRESHOLD = 0x0146,
  WIFI_MRVLIETYPES_PSSTAAGEOUT = 0x017b
} WiFi_MrvlIETypes;

// SDIO帧类型
typedef enum
{
  WIFI_SDIOFRAME_DATA = 0x00,
  WIFI_SDIOFRAME_COMMAND = 0x01,
  WIFI_SDIOFRAME_EVENT = 0x03
} WiFi_SDIOFrameType;

// 16.5 SDIO Card Metaformat
typedef enum
{
  CISTPL_NULL = 0x00, // Null tuple
  CISTPL_VERS_1 = 0x15, // Level 1 version/product-information
  CISTPL_MANFID = 0x20, // Manufacturer Identification String Tuple
  CISTPL_FUNCID = 0x21, // Function Identification Tuple
  CISTPL_FUNCE = 0x22, // Function Extensions
  CISTPL_END = 0xff // The End-of-chain Tuple
} WiFi_SDIOTupleCode;

// 无线认证类型
typedef enum
{
  WIFI_SECURITYTYPE_NONE = 0,
  WIFI_SECURITYTYPE_WEP = 1,
  WIFI_SECURITYTYPE_WPA = 2,
  WIFI_SECURITYTYPE_WPA2 = 3
} WiFi_SecurityType;

// 回调函数中的状态参数
typedef enum
{
  WIFI_STATUS_OK = 0, // 成功收到了回应
  WIFI_STATUS_FAIL = 1, // 未能完成请求的操作 (例如找到了AP热点但关联失败)
  WIFI_STATUS_BUSY = 2, // 之前的操作尚未完成
  WIFI_STATUS_NORESP = 3, // 重试了几遍都没有收到回应
  WIFI_STATUS_MEM = 4, // 内存不足
  WIFI_STATUS_INVALID = 5, // 无效的参数
  WIFI_STATUS_NOTFOUND = 6, // 未找到目标 (如AP热点)
  WIFI_STATUS_INPROGRESS = 7, // 成功执行命令, 但还需要后续的操作 (比如关联AP成功但还需要后续的认证操作)
  WIFI_STATUS_UNSUPPORTED = 8 // 不支持该命令
} WiFi_Status;
// 返回值为0表示成功, 1表示失败的函数, 返回类型必须声明为WiFi_Status
// 如果声明为uint8_t, 则1为成功, 0为失败

// WEP密钥长度
typedef enum
{
  WIFI_WEPKEYTYPE_40BIT = 1,
  WIFI_WEPKEYTYPE_104BIT = 2
} WiFi_WEPKeyType;

/* 回调函数类型 */
typedef void (*WiFi_Callback)(void *arg, void *data, WiFi_Status status); // data为NULL表示没有收到任何回应

/* WiFi命令字段位定义 */
// Capability information
#define WIFI_CAPABILITY_ESS _BV(0)
#define WIFI_CAPABILITY_IBSS _BV(1)
#define WIFI_CAPABILITY_CF_POLLABLE _BV(2)
#define WIFI_CAPABILITY_CF_POLL_REQUEST _BV(3)
#define WIFI_CAPABILITY_PRIVACY _BV(4)
#define WIFI_CAPABILITY_SHORT_PREAMBLE _BV(5)
#define WIFI_CAPABILITY_PBCC _BV(6)
#define WIFI_CAPABILITY_CHANNEL_AGILITY _BV(7)
#define WIFI_CAPABILITY_SPECTRUM_MGMT _BV(8)
#define WIFI_CAPABILITY_SHORT_SLOT _BV(10)
#define WIFI_CAPABILITY_DSSS_OFDM _BV(13)

#define WIFI_KEYINFO_DEFAULTKEY _BV(3)
#define WIFI_KEYINFO_KEYENABLED _BV(2)
#define WIFI_KEYINFO_UNICASTKEY _BV(1)
#define WIFI_KEYINFO_MULTICASTKEY _BV(0)

#define WIFI_MACCTRL_RX _BV(0)
#define WIFI_MACCTRL_TX _BV(1) // 此位必须要置1才能发送数据！！！
#define WIFI_MACCTRL_LOOPBACK _BV(2)
#define WIFI_MACCTRL_WEP _BV(3)
#define WIFI_MACCTRL_ETHERNET2 _BV(4)
#define WIFI_MACCTRL_PROMISCUOUS _BV(7)
#define WIFI_MACCTRL_ALLMULTICAST _BV(8)
#define WIFI_MACCTRL_ENFORCEPROTECTION _BV(10) // strict protection
#define WIFI_MACCTRL_ADHOCGPROTECTIONMODE _BV(13) // 802.11g protection mode

/* 常用的宏函数 */
// 确定bss字段的值
// type: STA=0, uAP=1
// number: 0~15
#define WIFI_BSS(type, number) ((((type) & 1) << 4) | ((number) & 15))
#define WIFI_BSSTYPE(bss) (((bss) >> 4) & 1)
#define WIFI_BSSNUM(bss) ((bss) & 15)

// 已知结构体大小sizeof(tlv), 求数据域的大小, 一般用于给header.length赋值
// 例如定义一个MrvlIETypes_CfParamSet_t param变量, 赋值param.header.length=TLV_PAYLOADLEN(param)
#define TLV_PAYLOADLEN(tlv) (sizeof(tlv) - sizeof((tlv).header))

// 已知数据域大小, 求整个结构体的大小
// 例如定义一个很大的buffer, 然后定义一个IEEEType *的指针p指向该buffer
// buffer接收到数据后, 要求出接收到的IEEEType数据的实际大小显然不能用sizeof(IEEEType), 因为定义IEEEType结构体时data的长度定义的是1
// 此时就应该使用TLV_STRUCTLEN(*p)
#define TLV_STRUCTLEN(tlv) (sizeof((tlv).header) + (tlv).header.length)

// 已知本TLV的地址和大小, 求下一个TLV的地址
#define TLV_NEXT(tlv) ((uint8_t *)(tlv) + TLV_STRUCTLEN(*(tlv)))

// 字节序转换函数
#ifndef htons
#define htons(x) ((((x) & 0x00ffUL) << 8) | (((x) & 0xff00UL) >> 8))
#endif
#ifndef ntohs
#define ntohs htons
#endif

#define WiFi_GetCommandCode(data) (((data) == NULL) ? 0 : (((const WiFi_CommandHeader *)(data))->cmd_code & 0x7fff))
#define WiFi_IsCommandResponse(data) ((data) != NULL && ((const WiFi_CommandHeader *)(data))->cmd_code & 0x8000)
#define WiFi_IsConnected() WiFi_GetBSSID(NULL)

/* TLV (Tag Length Value) of IEEE IE Type Format */
typedef __packed struct
{
  uint8_t type;
  uint8_t length; // 数据域的大小
} IEEEHeader;

// information element parameter
// 所有IEEETypes_*类型的基类型
typedef __packed struct
{
  IEEEHeader header;
  uint8_t data[1];
} IEEEType;

typedef __packed struct
{
  IEEEHeader header;
  uint8_t channel;
} IEEETypes_DsParamSet_t;

typedef __packed struct
{
  IEEEHeader header;
  uint16_t atim_window;
} IEEETypes_IbssParamSet_t;

/* TLV (Tag Length Value) of MrvllEType Format */
typedef __packed struct
{
  uint16_t type;
  uint16_t length;
} MrvlIEHeader;

// 所有MrvlIETypes_*类型的基类型
typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t data[1];
} MrvlIEType;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t which_antenna;
  uint8_t antenna_mode;
} MrvlIETypes_Antenna_Ctrl_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t bcast_ssid_ctl;
} MrvlIETypes_ApBCast_SSID_Ctrl_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t tx_power_dbm;
} MrvlIETypes_ApTx_Power_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint16_t auth_type;
} MrvlIETypes_AuthType_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint16_t becaon_period;
} MrvlIETypes_Beacon_Period_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t count;
  uint8_t period;
  uint16_t max_duration;
  uint16_t duration_remaining;
} MrvlIETypes_CfParamSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  __packed struct
  {
    uint8_t band_config_type;
    uint8_t chan_number;
    uint8_t scan_type;
    uint16_t min_scan_time;
    uint16_t max_scan_time;
  } channels[1];
} MrvlIETypes_ChanListParamSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t dtim_period;
} MrvlIETypes_DTIM_Period_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint16_t frag_threshold;
} MrvlIETypes_Frag_Threshold_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint16_t key_type_id;
  uint16_t key_info;
  uint16_t key_len;
  uint8_t key[32];
} MrvlIETypes_KeyParamSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t mac_address[6];
  uint8_t key_idx;
  uint8_t key_type;
  uint16_t key_info;
} MrvlIETypes_KeyParamSet_v2_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t passphrase[64];
} MrvlIETypes_Passphrase_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t channel;
} MrvlIETypes_PhyParamDSSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t pkt_fwd_ctl;
} MrvlIETypes_PKT_Fwd_Ctrl_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t preamble_type;
} MrvlIETypes_Preamble_Ctrl_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint32_t ps_sta_ageout_100ms;
} MrvlIETypes_PS_STA_Ageout_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t rates[14];
} MrvlIETypes_RatesParamSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t rsn[64];
} MrvlIETypes_RsnParamSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint16_t rts_threshold;
} MrvlIETypes_RTS_Threshold_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t ssid[32];
} MrvlIETypes_SSIDParamSet_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint32_t sta_ageout_timer_100ms;
} MrvlIETypes_STA_Ageout_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t mac_address[6];
} MrvlIETypes_StaMacAddr_t;

typedef MrvlIETypes_StaMacAddr_t MrvlIETypes_ApMacAddr_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t filter_mode;
  uint8_t count;
  uint8_t mac_address[1][6];
} MrvlIETypes_STA_MAC_Filter_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint64_t tsf_table[1];
} MrvlIETypes_TsfTimestamp_t;

typedef __packed struct
{
  MrvlIEHeader header;
  uint16_t tx_data_rate;
} MrvlIETypes_TxData_Rate_t;

// 整个结构体的最大大小为256字节
typedef __packed struct
{
  MrvlIEHeader header;
  uint8_t vendor[64]; // 通常情况下64字节已足够
} MrvlIETypes_VendorParamSet_t;

typedef MrvlIETypes_Passphrase_t MrvlIETypes_WPA_Passphrase_t;

/* WiFi命令帧和数据帧格式 */
// WiFi模块所有类型的帧的头部
typedef __packed struct
{
  uint16_t length; // 大小包括此成员本身
  uint16_t type;
} WiFi_SDIOFrameHeader;

// WiFi模块命令帧的头部
typedef __packed struct
{
  WiFi_SDIOFrameHeader frame_header;
  uint16_t cmd_code;
  uint16_t size;
  uint8_t seq_num;
  uint8_t bss;
  uint16_t result;
} WiFi_CommandHeader;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint8_t bssid[6]; // MAC address
  uint8_t ssid[32];
  uint8_t bss_type;
  uint16_t bcn_period;
  uint8_t dtim_period; // Specify DTIM period (TBTTs)
  uint8_t timestamp[8];
  uint8_t start_ts[8]; // Starting timestamp
  IEEETypes_DsParamSet_t ds_param_set; // IEEE DS parameter set element
  uint32_t reserved1;
  IEEETypes_IbssParamSet_t ibss_param_set; // IEEE IBSS parameter set
  uint32_t reserved2;
  uint16_t cap_info;
  uint8_t data_rates[14];
  uint32_t reserved3;
} WiFi_Cmd_ADHOCJoin;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint8_t ssid[32];
  uint8_t bss_type;
  uint16_t bcn_period;
  uint8_t reserved1;
  IEEETypes_IbssParamSet_t ibss_param_set; // ATIM window length in TU
  uint32_t reserved2;
  IEEETypes_DsParamSet_t ds_param_set; // The channel for ad-hoc network
  uint8_t reserved3[6];
  uint16_t cap_info; // Capability information
  uint8_t data_rate[14];
} WiFi_Cmd_ADHOCStart;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint8_t peer_sta_addr[6];
  uint16_t reason_code; // Reason code defined in IEEE 802.11 specification section 7.3.1.7 to indicate de-authentication reason
} WiFi_Cmd_Deauthenticate;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t action;
} WiFi_Cmd_KeyMaterial;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t action;
  uint8_t mac_addr[6];
} WiFi_Cmd_MACAddr;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t action;
  uint16_t reserved;
} WiFi_Cmd_MACCtrl;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t action;
  uint16_t tx_key_index; // Key set being used for transmit (0~3)
  uint8_t wep_types[4]; // use 40 or 104 bits
  uint8_t keys[4][16];
} WiFi_Cmd_SetWEP;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t action;
  uint16_t cache_result;
} WiFi_Cmd_SupplicantPMK;

typedef WiFi_Cmd_KeyMaterial WiFi_Cmd_SysConfigure;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint8_t peer_sta_addr[6]; // Peer MAC address
  uint16_t cap_info; // Capability information
  uint16_t listen_interval; // Listen interval
  uint16_t bcn_period; // Beacon period
  uint8_t dtim_period; // DTIM period
} WiFi_CmdRequest_Associate;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint8_t bss_type;
  uint8_t bss_id[6];
} WiFi_CmdRequest_Scan;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t capability;
  uint16_t status_code;
  uint16_t association_id;
  IEEEType ie_buffer;
} WiFi_CmdResponse_Associate;

typedef __packed struct
{
  WiFi_CommandHeader header;
  uint16_t buf_size;
  uint8_t num_of_set;
} WiFi_CmdResponse_Scan;

// WiFi模块接收的数据帧
typedef __packed struct
{
  WiFi_SDIOFrameHeader header;
  uint8_t bss_type;
  uint8_t bss_num;
  uint16_t rx_packet_length; // Number of bytes in the payload
  uint16_t rx_packet_offset; // Offset from the start of the packet to the beginning of the payload data packet
  uint16_t rx_packet_type;
  uint16_t seq_num; // Sequence number
  uint8_t priority; // Specifies the user priority of received packet
  uint8_t rx_rate; // Rate at which this packet is received
  int8_t snr; // Signal to noise ratio for this packet (dB)
  uint8_t nf; // Noise floor for this packet (dBm). Noise floor is always negative. The absolute value is passed.
  uint8_t ht_info; // HT information
  uint8_t reserved1[3];
  uint8_t flags; // TDLS flags
  uint8_t reserved2[43];
  uint8_t payload[1]; // 数据链路层上的帧
} WiFi_DataRx;

// WiFi模块发送的数据帧
typedef __packed struct
{
  WiFi_SDIOFrameHeader header;
  uint8_t bss_type;
  uint8_t bss_num;
  uint16_t tx_packet_length; // Number of bytes in the payload data frame
  uint16_t tx_packet_offset; // Offset of the beginning of the payload data packet (802.3 or 802.11 frames) from the beginning of the packet (bytes)
  uint16_t tx_packet_type;
  uint32_t tx_control; // See 3.2.1 Per-Packet Settings
  uint8_t priority; // Specifies the user priority of transmit packet
  uint8_t flags;
  uint8_t pkt_delay_2ms; // Amount of time the packet has been queued in the driver layer for WMM implementations
  uint16_t reserved1;
  uint8_t tx_token_id;
  uint16_t reserved2;
  uint8_t payload[1]; // 数据链路层上的帧
} WiFi_DataTx;

// WiFi模块事件帧
typedef __packed struct
{
  WiFi_SDIOFrameHeader header;
  uint16_t event_id; // Enumerated identifier for the event
  uint8_t bss_num;
  uint8_t bss_type;
  
  // 部分事件拥有如下两个字段
  __packed struct
  {
    uint16_t reason_code; // IEEE Reason Code as described in the 802.11 standard
    uint8_t mac_addr[6]; // Peer STA Address
  } event_body;
} WiFi_Event;

/* WiFi的常用数据结构 */
typedef __packed struct
{
  uint16_t ie_length; // Total information element length (不含sizeof(ie_length))
  uint8_t bssid[6]; // BSSID
  uint8_t rssi; // RSSI value as received from peer
  
  // Probe Response/Beacon Payload
  uint64_t pkt_time_stamp; // Timestamp
  uint16_t bcn_interval; // Beacon interval
  uint16_t cap_info; // Capabilities information
  IEEEType ie_parameters; // 存放的是一些IEEE类型的数据
} WiFi_BssDescSet;

typedef struct
{
  WiFi_SecurityType security;
  char *ssid;
  void *password;
} WiFi_Connection;

typedef __packed struct
{
  uint8_t wep_key_index;
  uint8_t is_default_tx_key;
  uint8_t wep_key[13];
} WiFi_KeyParam_WEP_v1;

typedef __packed struct
{
  uint16_t key_len;
  uint8_t wep_key[13];
} WiFi_KeyParam_WEP_v2;

// WiFi热点信息
typedef struct
{
  MrvlIETypes_SSIDParamSet_t ssid;
  uint8_t mac_addr[6];
  uint16_t cap_info;
  uint16_t bcn_period;
  uint8_t channel;
  WiFi_KeyType key_type;
  MrvlIETypes_RatesParamSet_t rates;
  MrvlIETypes_RsnParamSet_t rsn;
  MrvlIETypes_VendorParamSet_t wpa;
  MrvlIETypes_VendorParamSet_t wwm;
  MrvlIETypes_VendorParamSet_t wps;
} WiFi_SSIDInfo;

typedef struct
{
  WiFi_Callback callback;
  void *arg;
  uint8_t busy;
  uint8_t retry; // 数据帧缓冲区不使用这个成员
  uint32_t start_time;
  uint32_t timeout;
} WiFi_TxBuffer;

typedef struct
{
  WiFi_SDIOFrameHeader header;
  int8_t port;
} WiFi_TxCallbackData;

typedef __packed struct
{
  uint32_t pkt_init_cnt;
  uint32_t pkt_success_cnt;
  uint32_t tx_attempts;
  uint32_t retry_failure;
  uint32_t expiry_failure;
} WiFi_TxPktStatEntry;

typedef __packed struct
{
  uint8_t oui[3];
  uint8_t oui_type;
  uint16_t version;
  uint8_t multicast_oui[4];
  uint16_t unicast_num;
  uint8_t unicast_oui[1][4]; // 这里假定unicast_num=1
  uint16_t auth_num; // 只有当unicast_num=1时, 该成员才会在这个位置上
  uint8_t auth_oui[1][4];
} WiFi_Vendor;

typedef struct
{
  char *keys[4];
  uint8_t index; // 0~3
} WiFi_WEPKey;

/* WiFi模块底层函数 */
uint8_t WiFi_LowLevel_GetFunctionNum(void);
void WiFi_LowLevel_Init(void);
uint8_t WiFi_LowLevel_ReadData(uint8_t func, uint32_t addr, void *data, uint32_t size, uint32_t bufsize);
uint8_t WiFi_LowLevel_ReadReg(uint8_t func, uint32_t addr);
void WiFi_LowLevel_SetBlockSize(uint8_t func, uint32_t size);
uint8_t WiFi_LowLevel_WriteData(uint8_t func, uint32_t addr, const void *data, uint32_t size, uint32_t bufsize);
uint8_t WiFi_LowLevel_WriteReg(uint8_t func, uint32_t addr, uint8_t value);

/* WiFi模块主要函数 */
void WiFi_Associate(const char *ssid, WiFi_AuthenticationType auth_type, WiFi_Callback callback, void *arg);
void WiFi_AssociateEx(const WiFi_Connection *conn, WiFi_AuthenticationType auth_type, int8_t max_retry, WiFi_Callback callback, void *arg);
uint8_t WiFi_CheckCommandBusy(WiFi_Callback callback, void *arg);
int8_t WiFi_CheckWEPKey(uint8_t *dest, const char *src);
void WiFi_CheckTimeout(void);
void WiFi_Deauthenticate(uint16_t reason, WiFi_Callback callback, void *arg);
void WiFi_DiscardData(void);
int8_t WiFi_GetAvailablePort(uint16_t bitmap);
uint8_t WiFi_GetBSSID(uint8_t mac_addr[6]);
uint16_t WiFi_GetDataLength(uint8_t port);
uint16_t WiFi_GetFirmwareStatus(void);
void WiFi_GetMACAddress(WiFi_Callback callback, void *arg);
uint8_t *WiFi_GetPacketBuffer(void);
uint16_t WiFi_GetReadPortStatus(void);
const WiFi_DataRx *WiFi_GetReceivedPacket(void);
WiFi_SecurityType WiFi_GetSecurityType(const WiFi_SSIDInfo *info);
uint16_t WiFi_GetWritePortStatus(void);
void WiFi_Init(void);
void WiFi_Input(void);
uint8_t WiFi_IsCommandBusy(void);
void WiFi_JoinADHOC(const char *ssid, WiFi_Callback callback, void *arg);
void WiFi_JoinADHOCEx(const WiFi_Connection *conn, int8_t max_retry, WiFi_Callback callback, void *arg);
void WiFi_MACAddr(const uint8_t newaddr[6], WiFi_CommandAction action, WiFi_Callback callback, void *arg);
void WiFi_MACControl(uint16_t action, WiFi_Callback callback, void *arg);
void WiFi_PrintRates(uint16_t rates);
void WiFi_PrintTxPacketStatus(void);
void WiFi_Scan(WiFi_Callback callback, void *arg);
void WiFi_ScanSSID(const char *ssid, WiFi_SSIDInfo *info, WiFi_Callback callback, void *arg);
void WiFi_SendCommand(uint16_t code, const void *data, uint16_t size, uint8_t bss, WiFi_Callback callback, void *arg, uint32_t timeout, uint8_t max_retry);
int8_t WiFi_SendPacket(void *data, uint16_t size, uint8_t bss, WiFi_Callback callback, void *arg, uint32_t timeout);
void WiFi_SetWEP(WiFi_CommandAction action, const WiFi_WEPKey *key, WiFi_Callback callback, void *arg);
void WiFi_SetWEPEx(const WiFi_WEPKey *key, WiFi_Callback callback, void *arg);
void WiFi_SetWPA(const char *password, WiFi_Callback callback, void *arg);
void WiFi_ShowCIS(void);
void WiFi_ShowKeyMaterials(void);
void WiFi_StartADHOC(const char *ssid, uint16_t cap_info, WiFi_Callback callback, void *arg);
void WiFi_StartADHOCEx(const WiFi_Connection *conn, WiFi_Callback callback, void *arg);
void WiFi_StartMicroAP(const WiFi_Connection *conn, WiFi_AuthenticationType auth_type, WiFi_Callback callback, void *arg);
void WiFi_StopADHOC(WiFi_Callback callback, void *arg);
uint8_t WiFi_TranslateTLV(MrvlIEType *mrvlie_tlv, const IEEEType *ieee_tlv, uint16_t mrvlie_payload_size);
uint8_t WiFi_Wait(uint8_t status, uint32_t timeout);

/* 外部自定义回调函数 */
void WiFi_AuthenticationCompleteHandler(void);
void WiFi_EventHandler(const WiFi_Event *event);
void WiFi_PacketHandler(const WiFi_DataRx *data, int8_t port);
