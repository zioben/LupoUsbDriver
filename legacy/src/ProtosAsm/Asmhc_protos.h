//-----------------------------------------------------
//
//  Interfaccia Routines in assembly
//
//-----------------------------------------------------
#include <exec/exec.h>
#include <usb/HcDriverstructs.h>


#ifndef ASMHC_PROTOS
#define ASM_HCPROTOS 1

//=============================================================================
//-----------------------------------------------------------------------------
//
//	Api di utilità
//
//-----------------------------------------------------------------------------
//=============================================================================


extern LONG  ConvertAddress(ULONG* _d0 __asm("d0"));
// A = $AABBCCDD  =>  A = $DDCCBBAA 

extern LONG  ConvertLong(ULONG _d0 __asm("d0"));
// A = $AABBCCDD  =>  A = $DDCCBBAA 

extern LONG  ConvertWord(UWORD _d0 __asm("d0"));
// A = $AABB      =>  A = $BBAA 

//=============================================================================
//-----------------------------------------------------------------------------
//
//	Host Controller : Sezione settaggio USB
//
//-----------------------------------------------------------------------------
//=============================================================================


// Ritorna la revisione del firmware
extern LONG  HcGetRevision(PHCD_DEVICE_DATA _a0 __asm("a0"));


extern void  HcSetState(PHCD_DEVICE_DATA _a0 __asm("a0"),ULONG _d0 __asm("d0"));
extern ULONG HcGetState(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Setta / ottieni lo stato USB
//	HC_USB_RESET
//	HC_USB_RESUME
//	HC_USB_OPERATIONAL
//	HC_USB_SUSPEND

extern void  HcListEnable(PHCD_DEVICE_DATA _a0 __asm("a0"),ULONG _d0 __asm("d0"));
extern void  HcListDisable(PHCD_DEVICE_DATA _a0 __asm("a0"),ULONG _d0 __asm("d0"));
// Abilita / disabilita l'elaborazione delle liste
//	HC_LIST_PERIODIC
//	HC_LIST_CONTROL
//	HC_LIST_BULK
//	HC_LIST_ISOCHRONOUS


extern void  HcSetCBSR(PHCD_DEVICE_DATA _a0 __asm("a0"),ULONG _d0 __asm("d0"));
// Setta Control Bulk Service Ratio
//	0,1,2,3

extern LONG  HcGetInterruptStatus(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Ottieni lo stato degli interrupt

extern void  HcInterruptRoutingEnable(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Abilita l'interrupt Routing (Su Amiga va disabilitato)

extern void  HcInterruptRoutingDisable(PHCD_DEVICE_DATA _a0 __asm("a0"));
// disabilita l'interrupt routing (gli interrupt sono raccolti dal bus PCI)

extern void  HcRemoteWakeupEnable(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Abilita la caratteristica del Remote Wakeup

extern void  HcRemoteWakeupDisable(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Disabilita il Remote Wakeup

extern char  HcRemoteWakeupConnected(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Indica se il dispositivo supporta il remote WakeUp
//
// if (HcemoteWakeupConnected(HostController))
// {
//   HcRemoteWakeupEnable(HostController)
// }
// else
// {
//   printf("\nNo Wakeup Enable supported\n")
// }

extern void  HcSoftReset(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Inizia un Software Reset:
//      Azzera Tutti i bit Dell'Host Controller
//	NON TOCCA la sezione della Root Hub.

extern void  HcChangeRequest(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Forza il cambiamento dei dati dell'Host Controller
// Causa un interrupt e il cambiamento viene effettuato dalla routine di servizio

extern void  HcSetBulkListFilled(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Obbliga l'host ad elaborare le liste BULK

extern void  HcSetControlListFilled(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Obbliga l'host ad elaborare le liste CONTROL

extern char  HcGetSchedulingOverrun(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Ottieni il totale degli scheduling overrun

extern void  HcInterruptEnable(PHCD_DEVICE_DATA _a0 __asm("a0"),ULONG _d0 __asm("d0"));
extern void  HcInterruptDisable(PHCD_DEVICE_DATA _a0 __asm("a0"),ULONG _d0 __asm("d0"));
// Abilita / Disabilita i segnali di interrupt:
//	HC_INT_RHSC	root hub status change
//	HC_INT_FNO	frame number overflow
//	HC_INT_UE	unexpected error
//	HC_INT_RD	resume detect
//	HC_INT_SOF	start of frame
//	HC_INT_WDH	writeback done head
//	HC_INT_SO	scheduling overrun
//	HC_INT_OC	ownership change request

extern void  HcInterruptSetSF(PHCD_DEVICE_DATA _a0 __asm("a0"));
extern void  HcInterruptClearSF(PHCD_DEVICE_DATA _a0 __asm("a0"));
//  Abilita / Disabilita lo Start of Frame interrupt

extern void  HcInterruptSet(PHCD_DEVICE_DATA _a0 __asm("a0"));
extern void  HcInterruptClear(PHCD_DEVICE_DATA _a0 __asm("a0"));
//  Abilita / Disabilita TUTTI i segnali di interrupt


//=============================================================================
//-----------------------------------------------------------------------------
//
//	Host Controller : Sezione Liste
//
//-----------------------------------------------------------------------------
//=============================================================================


extern LONG  HcSetHCCA(PHCD_DEVICE_DATA _a0 __asm("a0"),ULONG _d0 __asm("d0"));
// Inizializza Area HCCA

extern PENDPOINT_DESCRIPTOR	HcGetPeriodCurrentED (PHCD_DEVICE_DATA _a0 __asm("a0"));
extern PENDPOINT_DESCRIPTOR	HcGetControlCurrentED(PHCD_DEVICE_DATA _a0 __asm("a0"));
extern PENDPOINT_DESCRIPTOR	HcGetBulkCurrentED   (PHCD_DEVICE_DATA _a0 __asm("a0"));
// Ottieni l'indirizzo dell'Endpoint Periodico-Controllo-Bulk in elaborazione 

extern PENDPOINT_DESCRIPTOR	HcGetControlHeadED(PHCD_DEVICE_DATA _a0 __asm("a0"));
extern PENDPOINT_DESCRIPTOR	HcGetBulkHeadED   (PHCD_DEVICE_DATA _a0 __asm("a0"));
// Ottieni la TESTA della lista degli Endpoint Control-Bulk

extern void	HcSetControlHeadED(PHCD_DEVICE_DATA _a0 __asm("a0"),PENDPOINT_DESCRIPTOR _d0 __asm("d0"));
extern void	HcSetBulkHeadED   (PHCD_DEVICE_DATA _a0 __asm("a0"),PENDPOINT_DESCRIPTOR _d0 __asm("d0"));
// Inizializza la TESTA della lista degli Endpoint Control-Bulk

extern PTRANSFER_DESCRIPTOR HcGetDoneHead(PHCD_DEVICE_DATA _a0 __asm("a0"));
// ottieni la testa dei Transfer Descriptor elaborati.


//=============================================================================
//-----------------------------------------------------------------------------
//
//	Host Controller : Sezione Frame
//
//-----------------------------------------------------------------------------
//=============================================================================


extern UWORD HcGetFrameInterval(PHCD_DEVICE_DATA _a0 __asm("a0"));
extern void  HcSetFrameInterval(PHCD_DEVICE_DATA _a0 __asm("a0"),UWORD _d0 __asm("d0"));
// Ricava e inizializza il frame interval ( 11999 di default )

extern void  HcSetFSMPS(PHCD_DEVICE_DATA _a0 __asm("a0"),UWORD _d0 __asm("d0"));
// Inizializza il FS Largest Data Packet 
// (ricorda che è la + grande quantità di dati inviablili prima di causare Schedul. Overrun)

extern UWORD HcGetFrameNumber(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Ottieni il FrameNumber

extern void  HcSetPeriodicStart(PHCD_DEVICE_DATA _a0 __asm("a0"),UWORD _d0 __asm("d0"));
extern UWORD HcGetPeriodicStart(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Inizalizza / Ottieni il periodic start: usualmente vale 
//	0x3e67:
//	si ottiene come HcGetFrameInterval()+HcGetFrameInterval()/10;

extern void  HcSetThreShold(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Inizializza il ThreShold, ovvero fa decidere all'Host se è il caso di avviare una 
// tranzazione a bassa velocità...

//=============================================================================
//-----------------------------------------------------------------------------
//
//	Host Controller : Sezione ROOH HUB
//
//-----------------------------------------------------------------------------
//=============================================================================

extern char	HcRhGetNumberDownstreamPorts(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Ottieni il numero di porte nell'host controller;

extern char	HcRhGetPOTPGT(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Ottieni il tempo MINIMO per accedere ad una porta DOPO che la porta è
// stata alimentata:
// TempoDiAttesa = HcRhGetPOTPGT(HostController) * 2ms;
//
// Usualmente TempoDiAttesa= 255 * 2ms = 1<<8+1<<1 = 1 << 9 = 512 ms = Delay(25);

extern char	HcRhGetPowerSwitchingMode(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Ottieni il tipo di PowerSwitch
//	RH_NO_POWERSWITCH			porte sempre alimentate
//	RH_GLOBAL_POWERSWITCH			pulsante globale
//	RH_PORT_X_PORT_POWERSWITCH		pulsante porta per porta

extern char	HcRhGetOvercurrentProtectionMode(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Ottieni il tipo di Protezione da sovralimentazione
//	RH_NO_OVERCURRENT_PROTECTION		nessun tipo di controllo
//	RH_GLOBAL_OVERCURRENT_PROTECTION	pulsante globale
//	RH_PORT_X_PORT_POWERSWITCH		pulsante porta per porta
	
extern char	HcRhIsDeviceRemovable(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
// Dice se il dispositivo nella porta X è rimuovibile 
//	X = 1..15

extern char	HcRhIsDevicePowered(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
// Dice se il dispositivo nella porta X è alimentato 
//	X = 1..15

extern void	HcRhClearGlobalPower(PHCD_DEVICE_DATA _a0 __asm("a0"));
extern void	HcRhSetGlobalPower(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Spegni-Accendi tutte le porte 
// (funziona solo se l'host è dotato di Global Powerswitch)

extern void	HcRhSetRemoteWakeup(PHCD_DEVICE_DATA _a0 __asm("a0"));
extern void	HcRhClearRemoteWakeup(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Abilita / Disabilita il segnale di Device Remote Wakeup enable

extern char	HcRhGetGlobalOvercurrent(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Ottieni lo stato di global OverCurrent
// (funziona solo se l'host è dotato di Global Powerswitch)

extern char	HcRhGetOvercurrentIndicatorChange(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Verifica se si è verificato un cambiamento nel segnale di overcurrent

extern void	HcRhClearOvercurrentIndicatorChange(PHCD_DEVICE_DATA _a0 __asm("a0"));
// Azzera la caratteristica


//=============================================================================
//-----------------------------------------------------------------------------
//
//	Host Controller : Sezione ROOH HUB PORTS
//
//-----------------------------------------------------------------------------
//=============================================================================

extern char 	HcRhPort_GetPortPowerStatus(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
//  0 = Port Power X OFF     ( X = 1..15 )
//  1 = Port Power X ON

extern void 	HcRhPort_SetPortPower(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern void 	HcRhPort_ClearPortPower(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
//  Accende / Spegne la porta la porta X ( X = 1..15)
//  (valido solo nel caso che la porta sia gestita da Powerswitch porta per porta)

extern char 	HcRhPort_GetCurrentConnectStatus(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern char 	HcRhPort_GetConnectStatusChange(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern void 	HcRhPort_ClearConnectStatusChange(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
// Avvisa se un dispositivo è connesso alla porta X (X = 1..15)
//	  1 = Dispositivo connesso alla porta X 
//	  0 = Dispositivo non connesso alla porta X 
// Verifica se è cambiato lo stato
// Avvisa che hai capito che è cambiato lo stato

extern void 	HcRhPort_ClearPortEnable(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern void 	HcRhPort_SetPortEnable(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern char 	HcRhPort_GetPortEnableStatus(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern char 	HcRhPort_GetPortEnablestatusChange(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern void	HcRhPort_ClearPortEnableStatusChange(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
// Abilita / Disabilita la porta X (X = 1..15)
//
// Controlla lo stato della porta X  (1 = Porta X Abilitata; 0 = Porta X Disabilitata)
// Verifica se è cambiato lo stato
// Avvisa che hai capito che è cambiato lo stato

extern void 	HcRhPort_SetPortSuspend(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern void 	HcRhPort_ClearSuspendStatus(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern char 	HcRhPort_GetPortSuspendStatus(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern char 	HcRhPort_GetPortSuspendStatusChange(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern void 	HcRhPort_ClearPortSuspendSatatusChange(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
// Sospende / Riabilita la porta
//
// Controlla lo stato della porta X  (1 = Porta X Sospesa; 0 = Porta X Non Sospesa)
// Verifica se è cambiato lo stato
// Avvisa che hai capito che è cambiato lo stato


extern void 	HcRhPort_SetPortReset(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern char 	HcRhPort_GetPortResetStatus(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern char 	HcRhPort_GetPortResetStatusChange(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern void 	HcRhPort_ClearPortResetstatusChange(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
// Avvia un reset lungo la porta
// Controlla lo stato della porta X  (1 = Porta X si resetta; 0 = Porta X non in fase di reset)
// Verifica se è cambiato lo stato
// Avvisa che hai capito che è cambiato lo stato

extern char 	HcRhPort_GetPortOvercurrentIndicator(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern char 	HcRhPort_GetPortOvercurrentIndicatorChange(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
extern void 	HcRhPort_ClearPortOvercurrentIndicatorChange(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
// Controlla lo stato dell'overcurrent della porta X (X= 1..15) 
//  	0 = No OverCurrent
//	1 = Overcurrent
// Verifica se è cambiato lo stato
// Avvisa che hai capito che è cambiato lo stato

extern char 	HcRhPort_LowSpeedDeviceAttached(PHCD_DEVICE_DATA _a0 __asm("a0"),char _d0 __asm("d0"));
//	0 = HighSpeed device attached
//	1 = Low Speed Device attached

#endif