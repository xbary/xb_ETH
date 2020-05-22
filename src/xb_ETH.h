/*
	#define XB_ETH

	#define ETH_STATICIP_DEFAULT "192.168.1.27"
	#define ETH_MASK_DEFAULT "255.255.255.0"
	#define ETH_GATEWAY_DEFAULT "192.168.1.1"
	#define ETH_DNS_DEFAULT "8.8.8.8"

	#define ETH_PIN_RST 2
	#define ETH_PIN_CS 4
	#define ETH_PIN_INT 15
	#define ETH_PIN_MISO 19
    #define ETH_PIN_MOSI 23
    #define ETH_PIN_SCK 18


*/

#ifndef _XB_ETH_h
#define _XB_ETH_h

#include <xb_board.h>

typedef enum { nsDisconnect, nsConnect } TNETStatus;

extern TNETStatus NETStatus;

extern void NET_Connect();
extern void NET_Disconnect();

extern TTaskDef XB_ETH_DefTask;

extern bool ETH_Start;
extern IPAddress ETH_StaticIP;
extern IPAddress ETH_Mask;
extern IPAddress ETH_Gateway;
extern IPAddress ETH_DNS;

#endif

