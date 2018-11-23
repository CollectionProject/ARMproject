//#define DEBUG_CAN

#include <stddef.h>
//#include <time.h>

#include <def.h>
#include <pdo.h>
#include <sdo.h>
#include <sync.h>
#include <objdictdef.h>
#include <nmtSlave.h>
#include <lifegrd.h>

/**************************************************************************/
/* The node id                                                            */
/**************************************************************************/
/* Computed by strNode */
/* node_id default value. 
   You should always overwrite this by using the function setNodeId(u8 nodeId) in your C code.
*/
#define NODE_ID 0x01
u8 bDeviceNodeId = NODE_ID;
/**************************************************************************/
/* Declaration of the mapped variables                                    */
/**************************************************************************/
/* Computed by strDeclareMapVar */
<<<<<<< HEAD
u32 horiStatusWord;		// Mapped at index 0x6000, subindex 0x1
u32 horiInputWord;  	// Mapped at index 0x6000, subindex 0x2
u32 vertStatusWord;		// Mapped at index 0x6001, subindex 0x1
u32 vertInputWord;	  // Mapped at index 0x6001, subindex 0x2
u16 horiControlWord;  // Mapped at index 0x6002, subindex 0x1
u8  horiModeWord;     // Mapped at index 0x6002, subindex 0x2
u32 canopenErrNB;		// Mapped at index 0x6003, subindex 0x1
u32 canopenErrVAL;		// Mapped at index 0x6003, subindex 0x2
u32 horiposition;		// Mapped at index 0x6004, subindex 0x1
u32 horispeed;  		// Mapped at index 0x6004, subindex 0x2
=======
u32 horiStatusWord;		// Mapped at index 0x6000, subindex 0x1
u32 horiInputWord;  	// Mapped at index 0x6000, subindex 0x2
u32 vertStatusWord;		// Mapped at index 0x6001, subindex 0x1
u32 vertInputWord;	  // Mapped at index 0x6001, subindex 0x2
u16 horiControlWord;  // Mapped at index 0x6002, subindex 0x1
u8  horiModeWord;     // Mapped at index 0x6002, subindex 0x2
u32 canopenErrNB;		// Mapped at index 0x6003, subindex 0x1
u32 canopenErrVAL;		// Mapped at index 0x6003, subindex 0x2
u32 horiposition;		// Mapped at index 0x6004, subindex 0x1
u32 horispeed;  		// Mapped at index 0x6004, subindex 0x2
>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
u32 vertposition;		// Mapped at index 0x6005, subindex 0x1
u32 vertspeed;  		// Mapped at index 0x6005, subindex 0x2

//*****************************************************************************/
/* Computed by strStartDico */

/* Array of message processing information */
/* Should not be modified */
proceed_info proceed_infos[] = {
  {NMT,		    "NMT",	        NULL},
  {SYNC,        "SYNC",       proceedSYNC},
  {TIME_STAMP,	"TIME_STAMP",	NULL},
  {PDO1tx,	    "PDO1tx",     proceedPDO},
  {PDO1rx,	    "PDO1rx",	    proceedPDO},
  {PDO2tx,	    "PDO2tx",	    proceedPDO},
  {PDO2rx,	    "PDO2rx",	    proceedPDO},
  {PDO3tx,	    "PDO3tx",	    proceedPDO},
  {PDO3rx,	    "PDO3rx",	    proceedPDO},
  {PDO4tx,	    "PDO4tx",	    proceedPDO},
  {PDO4rx,	    "PDO4rx",	    proceedPDO},
  {SDOtx,	    "SDOtx",	      proceedSDO},
  {SDOrx,	    "SDOrx",        proceedSDO},
  {0xD,		    "Unknown",	    NULL},
  {NODE_GUARD,	"NODE GUARD", proceedNMTerror},
  {0xF,		    "Unknown",	    NULL}
};




//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
//                       OBJECT DICTIONARY
//                   
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// Make your change, depending of your application
 
/** index 1000h: device type. You have to change the value below, so
 *  it fits your canopen-slave-module
 */
/* Not used, so, should not be modified */
#define OBJNAME devicetype
/*const*/ u32 OBJNAME = 0x0;
/*const*/ subindex Index1000[] =
{
  { RO, uint32, sizeof(u32), (void*)&OBJNAME }
};
#undef OBJNAME

/** index 1001: error register. Change the entries to fit your application
 */
/* Not used, so, should not be modified */
#define OBJNAME errorRegister
/*const*/ u8 OBJNAME = 0x0;
/*const*/ subindex Index1001[] =
{
  { RO, uint8, sizeof(u8), (void*)&OBJNAME }
};
#undef OBJNAME

/** index 1005: COB_ID SYNC
*/
/* Should not be modified */
#define OBJNAME CobIdSync
/*const*/ u32 OBJNAME = 0x40000080; // bit 30 = 1 : device can generate a SYNC message
/*const*/ subindex Index1005[] =
{
  { RW, uint32, sizeof(u32), (void*)&OBJNAME }
};
#undef OBJNAME

/** index 1006: SYNC period
*/

#define OBJNAME SyncPeriod
// For producing the SYNC signal every n micro-seconds.
// Put O to not producing SYNC
/*const*/ u32 OBJNAME = 0x0; /* Default 0 to not produce SYNC */
/*const*/ subindex Index1006[] =
{
  { RW, uint32, sizeof(u32), (void*)&OBJNAME }
};
#undef OBJNAME


/**************************************************************************/
/* HeartBeat consumers : The nodes which can send a heartbeat             */
/**************************************************************************/
/* Computed by strHeartBeatConsumers */
static  u32 HBConsumerTimeArray[] = {
  0x00000000}; // Format 0x00NNTTTT (N=Node T=time in ms)

static  u8 HBConsumerCount = 1; // 1 nodes could send me their heartbeat.

subindex Index1016[] = {
  { RO, uint8, sizeof(u8), (void*)&HBConsumerCount },
  { RW, uint32, sizeof(u32), (void*)&HBConsumerTimeArray[0] }};

/**************************************************************************/
/* The node produce an heartbeat                                          */
/**************************************************************************/
/* Computed by strHeartBeatProducer */
/* Every HBProducerTime, the node sends its heartbeat */
static u16 HBProducerTime = 0;  /* in ms. If 0 : not activated */ 
 subindex Index1017[] =
{
	{ RW, uint16, sizeof(u16), &HBProducerTime }
};

/**************************************************************************/
/* Next to 0x1018                                                 */
/**************************************************************************/
/* Computed by strVaria1 */
/** index 1018: identify object. Adjust the entries for your node/company
 */
/* Values can be modified */
#define OBJNAME theIdentity
/*const*/ s_identity OBJNAME =
{
  4,       // number of supported entries
  0x1234,  // Vendor-ID (given by the can-cia)
  0x5678,  // Product Code
  0x1364,  // Revision number
  0x7964,  // serial number
} ;


/* Should not be modified */
/*const*/ subindex Index1018[] =
{
  { RO, uint8,  sizeof(u8),  (void*)&OBJNAME.count },
  { RO, uint32, sizeof(u32), (void*)&OBJNAME.vendor_id},
  { RO, uint32, sizeof(u32), (void*)&OBJNAME.product_code},
  { RO, uint32, sizeof(u32), (void*)&OBJNAME.revision_number},
  { RO, uint32, sizeof(u32), (void*)&OBJNAME.serial_number}
};
#undef OBJNAME

/** now the communication profile entries are grouped together, so they
 *  can be accessed in a standardised manner. This could be memory-optimized
 *  if the empty entries wouldn't be added, but then the communication profile
 *  area must be accessed different (see objacces.c file)
 */
/* Should not be modified */
const indextable CommunicationProfileArea[] =
{
  DeclareIndexTableEntry(Index1000), // creates a line like: { Index1000, 1 },
  DeclareIndexTableEntry(Index1001),
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  DeclareIndexTableEntry(Index1005),
  DeclareIndexTableEntry(Index1006),
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  { NULL, 0 },
  DeclareIndexTableEntry(Index1016),
  DeclareIndexTableEntry(Index1017),
  DeclareIndexTableEntry(Index1018),
};

/**************************************************************************/
/* The SDO Server parameters                                              */
/**************************************************************************/
/* Computed by strSdoServer */
/* BEWARE You cannot define more than one SDO server */


#define INDEX_LAST_SDO_SERVER           0x1200
#define DEF_MAX_COUNT_OF_SDO_SERVER     INDEX_LAST_SDO_SERVER - 0x11FF

/* The values should not be modified here, but can be changed at runtime */
#define OBJNAME serverSDO1
static s_sdo_parameter OBJNAME = 
{ 3,                   // Number of entries. Always 3 for the SDO	       
  0x600 + NODE_ID,     // The cob_id transmited in CAN msg to the server     
  0x580 + NODE_ID,     // The cob_id received in CAN msg from the server  
  NODE_ID              // The node id of the client. Should not be modified 
};
static subindex Index1200[] =
{
  { RO, uint8,  sizeof( u8 ), (void*)&OBJNAME.count },
  { RO, uint32, sizeof( u32), (void*)&OBJNAME.cob_id_client },
  { RO, uint32, sizeof( u32), (void*)&OBJNAME.cob_id_server },
  { RW, uint8,  sizeof( u8),  (void*)&OBJNAME.node_id }
};
#undef OBJNAME

/** Create the server SDO Parameter area.
 */
/* Should not be modified */
const indextable serverSDOParameter[] =
{
  DeclareIndexTableEntry(Index1200)
};

/**************************************************************************/
/* The SDO(s) clients                                                     */
/**************************************************************************/
/* Computed by strSdoClients */
/* For a slave node, declare only one SDO client to send data to the master */
/* The master node must have one SDO client for each slave */
#define INDEX_LAST_SDO_CLIENT           0x1289
#define DEF_MAX_COUNT_OF_SDO_CLIENT     10

#define _CREATE_SDO_CLIENT_(SDONUM) \
static  s_sdo_parameter clientSDO ## SDONUM = \
{ 3,    /* Number of entries. Always 3 for the SDO*/\
0x600,  /* The cob_id transmited in CAN msg to the server. Put the "good" value instead : 0x600 + server node */\
0x580,  /* The cob_id received in CAN msg from the server. Put the "good" value instead : 0x600 + server node*/\
0x00    /* The node id of the server. Put the "good" value : server node*/\
};\
static  subindex Index ## SDONUM  [] =\
{\
	{ RO, uint8, sizeof( u8 ), (void*)&clientSDO ## SDONUM.count },\
	{ RO, uint32, sizeof( u32), (void*)&clientSDO ## SDONUM.cob_id_client },\
	{ RO, uint32, sizeof( u32), (void*)&clientSDO ## SDONUM.cob_id_server },\
	{ RO, uint8, sizeof( u8), (void*)&clientSDO ## SDONUM.node_id }\
};

_CREATE_SDO_CLIENT_(1280)
_CREATE_SDO_CLIENT_(1281)
_CREATE_SDO_CLIENT_(1282)
_CREATE_SDO_CLIENT_(1283)
_CREATE_SDO_CLIENT_(1284)
_CREATE_SDO_CLIENT_(1285)
_CREATE_SDO_CLIENT_(1286)
_CREATE_SDO_CLIENT_(1287)
_CREATE_SDO_CLIENT_(1288)
_CREATE_SDO_CLIENT_(1289)

/* Create the client SDO Parameter area. */
const indextable   clientSDOParameter[] =
{
  DeclareIndexTableEntry(Index1280),
  DeclareIndexTableEntry(Index1281),
  DeclareIndexTableEntry(Index1282),
  DeclareIndexTableEntry(Index1283),
  DeclareIndexTableEntry(Index1284),
  DeclareIndexTableEntry(Index1285),
  DeclareIndexTableEntry(Index1286),
  DeclareIndexTableEntry(Index1287),
  DeclareIndexTableEntry(Index1288),
  DeclareIndexTableEntry(Index1289)
};

/**************************************************************************/
/* The PDO(s) Which could be received                                     */
/**************************************************************************/
/* Computed by strPdoReceive */
#define INDEX_LAST_PDO_RECEIVE  0X1405
#define MAX_COUNT_OF_PDO_RECEIVE 6

#define _CREATE_RXPDO_(RXPDO) \
static  s_pdo_communication_parameter RxPDO ## RXPDO = \
{ 2,      /* Number of entries. Always 2*/\
0x000,  /* Default cobid*/\
0x00    /* Transmission type. See objetdictdef.h*/\
};\
static  subindex Index ## RXPDO[] =\
{\
	{ RO, uint8, sizeof( u8 ), (void*)&RxPDO ## RXPDO.count },\
	{ RW, uint32, sizeof( u32), (void*)&RxPDO ## RXPDO.cob_id },\
	{ RW, uint8, sizeof( u8), (void*)&RxPDO ## RXPDO.type }\
};

// This define the PDO receive entries from index 0x1400 to 0x1405 
_CREATE_RXPDO_(1400)
_CREATE_RXPDO_(1401)
<<<<<<< HEAD
_CREATE_RXPDO_(1402)
_CREATE_RXPDO_(1403)
_CREATE_RXPDO_(1404)
_CREATE_RXPDO_(1405)
=======
_CREATE_RXPDO_(1402)
_CREATE_RXPDO_(1403)
_CREATE_RXPDO_(1404)
_CREATE_RXPDO_(1405)
>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c

/* Create the Receive PDO Parameter area. */
const indextable   receivePDOParameter[] =
{ 
  DeclareIndexTableEntry(Index1400),
  DeclareIndexTableEntry(Index1401),
<<<<<<< HEAD
  DeclareIndexTableEntry(Index1402),
  DeclareIndexTableEntry(Index1403),
	DeclareIndexTableEntry(Index1404),
=======
  DeclareIndexTableEntry(Index1402),
  DeclareIndexTableEntry(Index1403),
	DeclareIndexTableEntry(Index1404),
>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
	DeclareIndexTableEntry(Index1405),
};

/**************************************************************************/
/* The PDO(s) Which could be transmited                                   */
/**************************************************************************/
/* Computed by strPdoTransmit */
#define INDEX_LAST_PDO_TRANSMIT  0X1802
#define MAX_COUNT_OF_PDO_TRANSMIT 4
/** Usually the ID of a transmitting PDO is 0x180 + device_node_id,
*  but the node_id is not known during compilation... so what to do?!
*  the correct values have to be setted up during bootup of the device...
*/
#define _CREATE_TXPDO_(TXPDO) \
static  s_pdo_communication_parameter TxPDO ## TXPDO = \
{ 2,      /* Number of entries. Always 2*/\
0x000,  /* Default cobid*/\
0x00    /* Transmission type. See objetdictdef.h*/\
};\
static  subindex Index ## TXPDO[] =\
{\
	{ RO, uint8, sizeof( u8 ), (void*)&TxPDO ## TXPDO.count },\
	{ RW, uint32, sizeof( u32), (void*)&TxPDO ## TXPDO.cob_id },\
	{ RW, uint8, sizeof( u8), (void*)&TxPDO ## TXPDO.type }\
};

// This define the PDO transmit entries from index 0x1800 to 0x1802 
_CREATE_TXPDO_(1800)
_CREATE_TXPDO_(1801)
_CREATE_TXPDO_(1802)
_CREATE_TXPDO_(1803)
/* Create the Transmit PDO Parameter area. */
const indextable   transmitPDOParameter[] =
{ 
  DeclareIndexTableEntry(Index1800),
  DeclareIndexTableEntry(Index1801),
<<<<<<< HEAD
  DeclareIndexTableEntry(Index1802),
=======
  DeclareIndexTableEntry(Index1802),
>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
	DeclareIndexTableEntry(Index1803)
};

/**************************************************************************/
/* PDO Mapping parameters                                                 */
/**************************************************************************/
/* Computed by strPdoParam */

#  define PDO_MAP(index, sub_index, size_variable_in_bits)\
0x ## index ## sub_index ## size_variable_in_bits

/* Beware : 
index                 *must* be writen 4 numbers in hexa
sub_index             *must* be writen 2 numbers in hexa
size_variable_in_u8 *must* be writen 2 numbers in hexa
*/
/* Max number of data which can be put in a PDO
   Example, one PDO contains 2 objects, an other contains 5 objects.
   put 
   MAX_COUNT_OF_PDO_MAPPING 5
*/
#define MAX_COUNT_OF_PDO_MAPPING 8

typedef struct td_s_pdo_mapping_parameter  // Index: 0x21
{
/** count of mapping entries
	*/
	u8 count;
	/** mapping entries itself.
	*/
	u32 object[MAX_COUNT_OF_PDO_MAPPING];
} s_pdo_mapping_parameter;

/**************************************************************************/
/* The mapping area of the PDO received                                   */
/**************************************************************************/
/* Computed by strPdoReceiveMapTop */
/* Note, The index 160x is used to map the PDO 140x. The relation between the two is automatic */
/* Computed by  strCreateRxMap */
#define _CREATE_RXMAP_(RXMAP) \
static  s_pdo_mapping_parameter RxMap ## RXMAP = \
{ 0,\
{\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00)\
 }\
 };\
 subindex Index ## RXMAP [] =\
{\
  { RW, uint8, sizeof( u8 ), (void*)&RxMap ## RXMAP.count },\
  { RW, uint32, sizeof( u32 ), (void*)&RxMap ## RXMAP.object[0] },\
  { RW, uint32, sizeof( u32 ), (void*)&RxMap ## RXMAP.object[1] },\
  { RW, uint32, sizeof( u32 ), (void*)&RxMap ## RXMAP.object[2] },\
  { RW, uint32, sizeof( u32 ), (void*)&RxMap ## RXMAP.object[3] },\
  { RW, uint32, sizeof( u32 ), (void*)&RxMap ## RXMAP.object[4] },\
  { RW, uint32, sizeof( u32 ), (void*)&RxMap ## RXMAP.object[5] },\
  { RW, uint32, sizeof( u32 ), (void*)&RxMap ## RXMAP.object[6] },\
  { RW, uint32, sizeof( u32 ), (void*)&RxMap ## RXMAP.object[7] }\
};
/* Computed by strPdoReceiveMapBot */
#define INDEX_LAST_PDO_MAPPING_RECEIVE  0x1603
_CREATE_RXMAP_(1600)
_CREATE_RXMAP_(1601)
<<<<<<< HEAD
_CREATE_RXMAP_(1602)
=======
_CREATE_RXMAP_(1602)
>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
_CREATE_RXMAP_(1603)

const indextable   RxPDOMappingTable[ ] =
{
  DeclareIndexTableEntry(Index1600),
<<<<<<< HEAD
  DeclareIndexTableEntry(Index1601),
=======
  DeclareIndexTableEntry(Index1601),
>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
  DeclareIndexTableEntry(Index1602),
  DeclareIndexTableEntry(Index1603)
};

/**************************************************************************/
/* The mapping area of the PDO transmited                                   */
/**************************************************************************/
/* Computed by strPdoTransmitMapTop */
/* Note, The index 18xx is used to map the PDO 1Axxx. The relation between the two is automatic */
/* Computed by  strCreateRxMap */
#define _CREATE_TXMAP_(TXMAP) \
static  s_pdo_mapping_parameter TxMap ## TXMAP = \
{ 0,\
{\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00),\
    PDO_MAP(0000, 00, 00)\
 }\
 };\
 subindex Index ## TXMAP [] =\
{\
  { RW, uint8, sizeof( u8 ), (void*)&TxMap ## TXMAP.count },\
  { RW, uint32, sizeof( u32 ), (void*)&TxMap ## TXMAP.object[0] },\
  { RW, uint32, sizeof( u32 ), (void*)&TxMap ## TXMAP.object[1] },\
  { RW, uint32, sizeof( u32 ), (void*)&TxMap ## TXMAP.object[2] },\
  { RW, uint32, sizeof( u32 ), (void*)&TxMap ## TXMAP.object[3] },\
  { RW, uint32, sizeof( u32 ), (void*)&TxMap ## TXMAP.object[4] },\
  { RW, uint32, sizeof( u32 ), (void*)&TxMap ## TXMAP.object[5] },\
  { RW, uint32, sizeof( u32 ), (void*)&TxMap ## TXMAP.object[6] },\
  { RW, uint32, sizeof( u32 ), (void*)&TxMap ## TXMAP.object[7] }\
};
/* Computed by strPdoTransmitMapBot */
#define INDEX_LAST_PDO_MAPPING_TRANSMIT  0x1a02
_CREATE_TXMAP_(1a00)
_CREATE_TXMAP_(1a01)
_CREATE_TXMAP_(1a02)

const indextable   TxPDOMappingTable[ ] =
{
  DeclareIndexTableEntry(Index1a00),
  DeclareIndexTableEntry(Index1a01),
  DeclareIndexTableEntry(Index1a02)
};

/**************************************************************************/
/* The mapped variables at index 0x2000 - 0x5fff                          */
/**************************************************************************/
/* Computed by strMapVar */

#define MANUFACTURER_SPECIFIC_LAST_INDEX 0x0
const indextable manufacturerProfileTable[] = 
{
  {NULL, 0}
};



/**************************************************************************/
/* The mapped variables at index 0x6000 - 0x61ff                          */
/**************************************************************************/
/* Computed by strMapVar */

/********* Index 6000 *********/
static u8 highestSubIndex_6000 = 2; // number of subindex - 1
subindex Index6000[] = 
{
  { RO, uint8, sizeof(u8), (void*)&highestSubIndex_6000 },
  { RW, uint16, sizeof (u16), (void*)&horiStatusWord },
  { RW, uint16, sizeof (u16), (void*)&horiInputWord }
<<<<<<< HEAD
};

// /********* Index 6000 *********///modify at 2012.5.25
// static u8 highestSubIndex_6000 = 1; // number of subindex - 1
// subindex Index6000[] = 
// {
//   { RO, uint8, sizeof(u8), (void*)&highestSubIndex_6000 },
//   { RW, uint16, sizeof (u16), (void*)&horiStatusWord }
=======
};

// /********* Index 6000 *********///modify at 2012.5.25
// static u8 highestSubIndex_6000 = 1; // number of subindex - 1
// subindex Index6000[] = 
// {
//   { RO, uint8, sizeof(u8), (void*)&highestSubIndex_6000 },
//   { RW, uint16, sizeof (u16), (void*)&horiStatusWord }
>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
// };

/********* Index 6001 *********/
static u8 highestSubIndex_6001 = 2; // number of subindex - 1
subindex Index6001[] = 
{
  { RO, uint8, sizeof(u8), (void*)&highestSubIndex_6001 },
  { RW, uint16, sizeof (u16), (void*)&vertStatusWord },
  { RW, uint16, sizeof (u16), (void*)&vertInputWord }
};

/********* Index 6002 *********/
static u8 highestSubIndex_6002 = 2; // number of subindex - 1
subindex Index6002[] = 
{
  { RO, uint8, sizeof(u8), (void*)&highestSubIndex_6002 },
  { RW, uint8, sizeof (u32), (void*)&horiControlWord },
  { RW, uint8, sizeof (u32), (void*)&horiModeWord }
};

/********* Index 6003 *********/
static u8 highestSubIndex_6003 = 2; // number of subindex - 1
subindex Index6003[] = 
{
  { RO, uint8, sizeof(u8), (void*)&highestSubIndex_6003 },
  { RW, uint32, sizeof (u32), (void*)&canopenErrNB },
  { RW, uint32, sizeof (u16), (void*)&canopenErrVAL }
<<<<<<< HEAD
};

=======
};

>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
/********* Index 6004 *********/
static u8 highestSubIndex_6004 = 2; // number of subindex - 1
subindex Index6004[] = 
{
  { RO, uint8, sizeof(u8), (void*)&highestSubIndex_6004 },
  { RW, uint32, sizeof (u32), (void*)&horiposition },
  { RW, uint32, sizeof (u16), (void*)&horispeed }
<<<<<<< HEAD
};

=======
};

>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
/********* Index 6005 *********/
static u8 highestSubIndex_6005 = 2; // number of subindex - 1
subindex Index6005[] = 
{
  { RO, uint8, sizeof(u8), (void*)&highestSubIndex_6005 },
  { RW, uint32, sizeof (u32), (void*)&vertposition },
  { RW, uint32, sizeof (u32), (void*)&vertspeed }
};

#define DIGITAL_INPUT_LAST_TABLE_INDEX 0x6005
const indextable digitalInputTable[] = 
{
  DeclareIndexTableEntry(Index6000),
  DeclareIndexTableEntry(Index6001),
  DeclareIndexTableEntry(Index6002),
<<<<<<< HEAD
  DeclareIndexTableEntry(Index6003),
  DeclareIndexTableEntry(Index6004),
=======
  DeclareIndexTableEntry(Index6003),
  DeclareIndexTableEntry(Index6004),
>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
  DeclareIndexTableEntry(Index6005)
};



/**************************************************************************/
/* The mapped variables at index 0x6200 - 0x9fff                          */
/**************************************************************************/
/* Computed by strMapVar */

#define DIGITAL_OUTPUT_LAST_TABLE_INDEX 0x0
const indextable digitalOutputTable[] = 
{
  {NULL, 0}
};

/**************************************************************************/
/* Varia used by other files                                              */
/**************************************************************************/
/* Computed by strVaria2 */
u8 count_sync[MAX_COUNT_OF_PDO_TRANSMIT];

#define COMM_PROFILE_LAST 0x1018

/* Should not be modified */
const dict_cste dict_cstes = {
COMM_PROFILE_LAST,
INDEX_LAST_SDO_SERVER,            
DEF_MAX_COUNT_OF_SDO_SERVER,	    
INDEX_LAST_SDO_CLIENT,	          
DEF_MAX_COUNT_OF_SDO_CLIENT,	    
INDEX_LAST_PDO_RECEIVE,	          
MAX_COUNT_OF_PDO_RECEIVE,	        
INDEX_LAST_PDO_TRANSMIT,	        
MAX_COUNT_OF_PDO_TRANSMIT,	      
INDEX_LAST_PDO_MAPPING_RECEIVE,	  
INDEX_LAST_PDO_MAPPING_TRANSMIT,
MANUFACTURER_SPECIFIC_LAST_INDEX,
DIGITAL_INPUT_LAST_TABLE_INDEX,
DIGITAL_OUTPUT_LAST_TABLE_INDEX,
NB_OF_HEARTBEAT_PRODUCERS	  
};  
