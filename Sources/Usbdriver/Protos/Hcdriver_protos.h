//------------------------------------------------
//
//  HcDriver protos
//
//------------------------------------------------


#ifndef HCDRIVER_PROTOS
#define HCDRIVER_PROTOS


#include <usb/HcDriverstructs.h>
#include <usb/ProtosAsm/asmhc_protos.h>


ULONG            HcCreateHccaArea();
void             HcKillHccaArea(PHCD_DEVICE_DATA hc);
struct MsgPort*  HcCreateMessagePort(char *NamePort);
void             HcKillMessagePort(PHCD_DEVICE_DATA hc);
PHCD_DEVICE_DATA HcCreateHc(void);
void             HcKillHc(PHCD_DEVICE_DATA hc);
void             HcDeleteDeviceData(PHCD_DEVICE_DATA hc);
PHCD_DEVICE_DATA HcCreateDeviceData(void);
UBYTE            SetupRootHubMessagePort(PHCD_DEVICE_DATA hc,struct MsgPort *Port);
UBYTE            FreeRootHubMessagePort(PHCD_DEVICE_DATA hc,struct MsgPort *Port);
ULONG            Get32BitFrameNumber(PHCD_DEVICE_DATA hc);
ULONG            WaitNextFrameNumber(PHCD_DEVICE_DATA hc);
ULONG            SetFrameInterval(PHCD_DEVICE_DATA devicedata,BOOL UpNotDown);
ULONG            HcFindDevice(PHCD_DEVICE_DATA dd);
ULONG            HcGetAviableBandwidth(PHCD_DEVICE_DATA dd);
ULONG            HcSetup(PHCD_DEVICE_DATA hc);
void             HcListSetup(PHCD_DEVICE_DATA hc);
void             HcStart(PHCD_DEVICE_DATA hc);
void             HcStop(PHCD_DEVICE_DATA hc);
void             HcPause(PHCD_DEVICE_DATA hc);

//------------------------------------------------------------------------
//	crea e distrugge l'hcca
//------------------------------------------------------------------------

//ULONG HcCreateHccaArea();
//void  HcKillHccaArea(PHCD_DEVICE_DATA hc);

//------------------------------------------------------------------------
//	crea e distrugge la messageport
//------------------------------------------------------------------------

//struct MsgPort* HcCreateMessagePort(char *nameport);
//void HcKillMessagePort(PHCD_DEVICE_DATA hc);

//------------------------------------------------------------------------
//	crea e distrugge un PHC_DEVICE_DATA (SOLO STRUTTURA PRINCIPALE)
//------------------------------------------------------------------------

//PHCD_DEVICE_DATA HcCreateHc(void);
//void HcKillHc(PHCD_DEVICE_DATA hc);

//------------------------------------------------------------------------
// crea e distrugge un PHCD_DEVICE_DATA
// HcCreateDeviceData: alloca	anche la porta messaggi
// HcDeleteDeviceData: 	-Blocca gli interrupt
//								-USB_STATE = Reset
//								-SOFTRESET
//								-DEALLOCA HCCA
//								-DEALLOCA PORTA messaggi
//								-DEALLOCA STRUTTURA
//------------------------------------------------------------------------

//void HcDeleteDeviceData(PHCD_DEVICE_DATA hc);
//PHCD_DEVICE_DATA HcCreateDeviceData(void);

//------------------------------------------------------------------------
//	 Macro del libro
//------------------------------------------------------------------------


//UBYTE SetupRootHubMessagePort(PHCD_DEVICE_DATA hc,struct MsgPort *Port);
//UBYTE FreeRootHubMessagePort (PHCD_DEVICE_DATA hc,struct MsgPort *Port);

//ULONG Get32BitFrameNumber(PHCD_DEVICE_DATA hc);
//ULONG SetFrameInterval(PHCD_DEVICE_DATA devicedata,BOOL UpNotDown);


//------------------------------------------------------------------------
//	 Trova dispositivo
//------------------------------------------------------------------------

//ULONG HcFindDevice(PHCD_DEVICE_DATA dd);

//------------------------------------------------------------------------
//	 Calcola Banda passante
//------------------------------------------------------------------------

//ULONG HcGetAviableBandwidth(PHCD_DEVICE_DATA dd);

//------------------------------------------------------------------------
//	 HcSetup:		-trova dispositivo
//	 					-test revisione
//						-SoftReset
//						-UsbReset
//						-Setta registri di timing
//	 Se tutto OK ritorna 0
//------------------------------------------------------------------------

//ULONG HcSetup(PHCD_DEVICE_DATA hc);

//------------------------------------------------------------------------
//	 HcListSetup:	-Crea HCCA
//	 					-Crea	BulkBase
//						-Crea ControlBase
//						-Crea	Interrupt list
//------------------------------------------------------------------------

//void  HcListSetup(PHCD_DEVICE_DATA hc);

//------------------------------------------------------------------------
//	 HcStart:		-Abilita Tutte le liste
//	 					-Abilita tutti gli interrupt
//						-USB state = Operational
//------------------------------------------------------------------------

//void  HcStart(PHCD_DEVICE_DATA hc);

//------------------------------------------------------------------------
//	 HcStop:			-Blocca liste
//	 					-Blocca Interrupt
//						-Reset  usb
//						-SoftReset
//------------------------------------------------------------------------

//void  HcStop(PHCD_DEVICE_DATA hc);


//------------------------------------------------------------------------
//	 HcPause:		-Blocca liste
//	 					-Blocca Interrupt
//						-Suspend  usb
//------------------------------------------------------------------------

//void  HcPause(PHCD_DEVICE_DATA hc);

#endif