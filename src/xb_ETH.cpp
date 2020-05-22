#include <xb_board.h>
#include "xb_ETH.h"
#ifdef XB_PING
#include <xb_PING.h>
#endif
#include <SPI.h>
#include <Ethernet.h>
#include <utility/w5100.h>


#ifdef XB_GUI
#include <xb_GUI.h>
#include <xb_GUI_Gadget.h>
TGADGETMenu* menuhandle1_mainmenuETH;
TGADGETInputDialog* ETH_inputdialoghandle1_static_ip;
TGADGETInputDialog* ETH_inputdialoghandle2_mask_ip;
TGADGETInputDialog* ETH_inputdialoghandle3_gateway_ip;
TGADGETInputDialog* ETH_inputdialoghandle4_dns_ip;

#endif


TNETStatus NETStatus;

typedef enum { efIDLE, efInit, efDeInit, efHandle } TETHFunction;
TETHFunction ETH_Function;

void NET_Connect()
{
	if (NETStatus == nsConnect) return;
	NETStatus = nsConnect;
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_NET_CONNECT;
	board.DoMessageOnAllTask(&mb, true, doFORWARD);
}

void NET_Disconnect()
{
	if (NETStatus == nsDisconnect) return;
	NETStatus = nsDisconnect;
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_NET_DISCONNECT;
	board.DoMessageOnAllTask(&mb, true, doBACKWARD);
}


//-------------------------------------------------------------------------------------------------------------
#pragma region KONFIGURACJE

bool ETH_Start = false;
IPAddress ETH_StaticIP;
IPAddress ETH_Mask;
IPAddress ETH_Gateway;
IPAddress ETH_DNS;

bool ETH_LoadConfig()
{
#ifdef XB_PREFERENCES
	if (board.PREFERENCES_BeginSection("ETH"))
	{
		ETH_Start = board.PREFERENCES_GetBool("START", false);
		ETH_StaticIP = (uint32_t)board.PREFERENCES_GetUINT32("StaticIP", (uint32_t)ETH_StaticIP);
		ETH_Mask = (uint32_t)board.PREFERENCES_GetUINT32("Mask", (uint32_t)ETH_Mask);
		ETH_Gateway = (uint32_t)board.PREFERENCES_GetUINT32("Gateway", (uint32_t)ETH_Gateway);
		ETH_DNS = (uint32_t)board.PREFERENCES_GetUINT32("DNS", (uint32_t)ETH_DNS);

		board.PREFERENCES_EndSection();
	}
	else
	{
		return false;
	}
	return true;
#else
	return false
#endif

}

bool ETH_SaveConfig()
{
#ifdef XB_PREFERENCES
	if (board.PREFERENCES_BeginSection("ETH"))
	{
		board.PREFERENCES_PutBool("START", ETH_Start);
		board.PREFERENCES_PutUINT32("StaticIP", (uint32_t)ETH_StaticIP);
		board.PREFERENCES_PutUINT32("Mask", (uint32_t)ETH_Mask);
		board.PREFERENCES_PutUINT32("Gateway", (uint32_t)ETH_Gateway);
		board.PREFERENCES_PutUINT32("DNS", (uint32_t)ETH_DNS);

		board.PREFERENCES_EndSection();
	}
	else 
	{ 
		return false; 
	}
	return true;
#else
	return false
#endif
}

#pragma endregion
//-------------------------------------------------------------------------------------------------------------

String ETH_GetEthernetClientStatus(uint8_t Astatus)
{
	String s;
	switch (Astatus)
	{
	case  0x00: s = "CLOSED"; break;
	case  0x13: s = "INIT"; break;
	case 0x14: s = "LISTEN"; break;
	case 0x15: s = "SYNSENT"; break;
	case 0x16: s = "SYNRECV"; break;
	case 0x17: s = "ESTABLISHED"; break;
	case 0x18: s = "FIN_WAIT"; break;
	case 0x1a: s = "CLOSING"; break;
	case 0x1b: s = "TIME_WAIT"; break;
	case 0x1c: s = "CLOSE_WAIT"; break;
	case 0x1d: s = "LAST_ACK"; break;
	case 0x22: s = "UDP"; break;
	case 0x32: s = "IPRAW"; break;
	case 0x42: s = "MACRAW"; break;
	case 0x5f: s = "PPPOE"; break;
	default: s = "?"+String(Astatus)+"?"; break;
	}
	return s;
}

//-------------------------------------------------------------------------------------------------------------

void IRQ__ETH_PIN_INT()
{
	board.TriggerInterrupt(&XB_ETH_DefTask);
}

void XB_ETH_Setup()
{
	board.Log("Init.", true, true);
	ETH_StaticIP.fromString(ETH_STATICIP_DEFAULT);
	ETH_Mask.fromString(ETH_MASK_DEFAULT);
	ETH_Gateway.fromString(ETH_GATEWAY_DEFAULT);
	ETH_DNS.fromString(ETH_DNS_DEFAULT);
	board.LoadConfiguration();
	board.Log('.');
	board.pinMode(ETH_PIN_RST, OUTPUT);
	board.digitalWrite(ETH_PIN_RST, LOW);
	board.pinMode(ETH_PIN_CS, OUTPUT);
	board.digitalWrite(ETH_PIN_CS,HIGH);
	board.pinMode(ETH_PIN_INT, INPUT_PULLUP);
	attachInterrupt(ETH_PIN_INT, &IRQ__ETH_PIN_INT, CHANGE);
	delay(600);
	board.digitalWrite(ETH_PIN_RST, HIGH);
	Ethernet.init(ETH_PIN_CS);

	NET_Disconnect();
	ETH_Function = efIDLE;
	ETH_Start = true;
	board.Log(".OK");
}
//-------------------------------------------------------------------------------------------------------------
void XB_ETH_DoInterrupt(void)
{
	//board.Log("Wiznet interrupt...",true,true);
}
//-------------------------------------------------------------------------------------------------------------
uint32_t XB_ETH_DoLoop()
{
	switch (ETH_Function)
	{
	case efIDLE:
	{
		if (ETH_Start)
		{
			ETH_Function = efInit;
			return 3000;
		}
		return 0;
	}
	case efInit:
	{
		board.Log("Connect Ethernet mac:", true, true);
		uint8_t mac[8];
		(*(uint64_t *)mac) = ESP.getEfuseMac();
		mac[7]++;
		String mactxt = board.DeviceIDtoString(*(TUniqueID*)mac).substring(0, 17);
		board.Log(mactxt.c_str());
		board.Log(" .");
		

		Ethernet.begin(mac, ETH_StaticIP, ETH_DNS, ETH_Gateway, ETH_Mask);
		board.Log('.');
		if (Ethernet.hardwareStatus() == EthernetNoHardware) 
		{
			board.Log("..ERROR");
			board.Log("Ethernet shield was not found.  Sorry, can't run without hardware. :(", true, true,tlError);
			ETH_Function = efIDLE;
			return 2000;
		}
		board.Log('.');
		if (Ethernet.linkStatus() == LinkOFF) 
		{
			board.Log("..ERROR");
			board.Log("Ethernet cable is not connected. :(", true, true, tlError);
			ETH_Function = efIDLE;
			return 2000;
		}
		
		Ethernet.setRetransmissionCount(10);
		Ethernet.setRetransmissionTimeout(1000);
		NET_Connect();
		board.Log(String("[Wiznet"+String(W5100.getChip())+"00].OK").c_str());
		ETH_Function = efHandle;
		return 2000;
	}
	case efDeInit:
	{
		NET_Disconnect();
		ETH_Function = efIDLE;
		return 2000;
	}
	case efHandle:
	{
		DEF_WAITMS_VAR(EFH1)
		BEGIN_WAITMS(EFH1, 250)
			RESET_WAITMS(EFH1)
			if ((Ethernet.linkStatus() == LinkOFF) || (Ethernet.hardwareStatus() == EthernetNoHardware))
			{
				board.Log("Disconnect Ethernet..", true, true);
				NET_Disconnect();
				board.Log(".OK");
				ETH_Function = efDeInit;
				return 100;
			}
			
		END_WAITMS(EFH1)

		return 0;
	}
	}
	return 0;
}
//-------------------------------------------------------------------------------------------------------------
bool XB_ETH_DoMessage(TMessageBoard* Am)
{
	switch (Am->IDMessage)
	{
	case IM_FREEPTR:
	{
#ifdef XB_GUI
		FREEPTR(menuhandle1_mainmenuETH);
		FREEPTR(ETH_inputdialoghandle1_static_ip);
		FREEPTR(ETH_inputdialoghandle2_mask_ip);
		FREEPTR(ETH_inputdialoghandle3_gateway_ip);
		FREEPTR(ETH_inputdialoghandle4_dns_ip);
#endif
		return true;
	}

	case IM_GET_TASKNAME_STRING: GET_TASKNAME("ETH"); break;
	case IM_GET_TASKSTATUS_STRING:
	{
		switch (ETH_Function)
		{
			GET_TASKSTATUS(efIDLE, 2);
			GET_TASKSTATUS(efInit, 2);
			GET_TASKSTATUS(efDeInit, 2);
			GET_TASKSTATUS(efHandle, 2);
		}
		return true;
	}

	case IM_LOAD_CONFIGURATION: 
	{
		
		if (!ETH_LoadConfig())
		{
			board.Log("Error");
		}
		break;
	}
	case IM_SAVE_CONFIGURATION: 
	{
		
		if (!ETH_SaveConfig())
		{
			board.Log("Error");
		}
		break;
	}

#ifdef XB_GUI		
	case IM_MENU:
	{

		OPEN_MAINMENU()
		{
			menuhandle1_mainmenuETH = GUIGADGET_CreateMenu(&XB_ETH_DefTask, 1, false, X, Y);
		}

		BEGIN_MENU(1, "ETH Configuration", WINDOW_POS_X_DEF, WINDOW_POS_Y_DEF, 40, MENU_AUTOCOUNT, 0, true)
		{
			BEGIN_MENUITEM_CHECKED("Start", taLeft, ETH_Start)
			{
				CLICK_MENUITEM()
				{
					ETH_Start = !ETH_Start;
					if (!ETH_Start) ETH_Function = efDeInit;
				}
			}
			END_MENUITEM()
				SEPARATOR_MENUITEM()
				CONFIGURATION_MENUITEMS()
				SEPARATOR_MENUITEM()
				BEGIN_MENUITEM(String("Set StaticIP (" + ETH_StaticIP.toString() + ")"), taLeft)
			{
				CLICK_MENUITEM()
				{
					ETH_inputdialoghandle1_static_ip = GUIGADGET_CreateInputDialog(&XB_ETH_DefTask, 0);
				}
			}
			END_MENUITEM()
				BEGIN_MENUITEM(String("Set STA Mask (" + ETH_Mask.toString() + ")"), taLeft)
			{
				CLICK_MENUITEM()
				{
					ETH_inputdialoghandle2_mask_ip = GUIGADGET_CreateInputDialog(&XB_ETH_DefTask, 1);
				}
			}
			END_MENUITEM()
				BEGIN_MENUITEM(String("Set STA GateWay (" + ETH_Gateway.toString() + ")"), taLeft)
			{
				CLICK_MENUITEM()
				{
					ETH_inputdialoghandle3_gateway_ip = GUIGADGET_CreateInputDialog(&XB_ETH_DefTask, 2);
				}
			}
			END_MENUITEM()
				BEGIN_MENUITEM(String("Set DNS (" + ETH_DNS.toString() + ")"), taLeft)
			{
				CLICK_MENUITEM()
				{
					ETH_inputdialoghandle4_dns_ip = GUIGADGET_CreateInputDialog(&XB_ETH_DefTask, 3);
				}
			}
			END_MENUITEM()
		}
		END_MENU()

		return true;
	}
	case IM_INPUTDIALOG:
	{
			BEGIN_DIALOG(0, "Edit Static IP", "Please input IPv4:", tivIP, 15, &ETH_StaticIP)
			{
			}
			END_DIALOG()
			BEGIN_DIALOG(1, "Edit Mask", "Please input IPv4:", tivIP, 15, &ETH_Mask)
			{
			}
			END_DIALOG()
			BEGIN_DIALOG(2, "Edit Gateway", "Please input IPv4:", tivIP, 15, &ETH_Gateway)
			{
			}
			END_DIALOG()
			BEGIN_DIALOG(3, "Edit DNS", "Please input IPv4:", tivIP, 15, &ETH_DNS)
			{
			}
			END_DIALOG()

			return true;
	}

#endif
	default: break;
	}
	return true;
}

TTaskDef XB_ETH_DefTask = { 1, &XB_ETH_Setup,&XB_ETH_DoLoop,&XB_ETH_DoMessage,&XB_ETH_DoInterrupt };

/*
EthernetClient.cpp

int EthernetClient::connect(const char* host, uint16_t port, int timeout)
{
	(void)timeout;
	return connect(host, port);
}

int EthernetClient::connect(IPAddress ip, uint16_t port, int timeout)
{
	(void)timeout;
	return connect(ip, port);
}

Ethernet.h

	virtual int connect(IPAddress ip, uint16_t port, int timeout);
	virtual int connect(const char* host, uint16_t port, int timeout);

*/