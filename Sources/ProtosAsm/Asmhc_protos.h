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


extern LONG  ConvertAddress(register __d0 ULONG*);
// A = $AABBCCDD  =>  A = $DDCCBBAA 

extern LONG  ConvertLong(register __d0 ULONG);
// A = $AABBCCDD  =>  A = $DDCCBBAA 

extern LONG  ConvertWord(register __d0 UWORD);
// A = $AABB      =>  A = $BBAA 

//=============================================================================
//-----------------------------------------------------------------------------
//
//	Host Controller : Sezione settaggio USB
//
//-----------------------------------------------------------------------------
//=============================================================================


// Ritorna la revisione del firmware
extern LONG  HcGetRevision(register __a0 PHCD_DEVICE_DATA);


extern void  HcSetState(register __a0 PHCD_DEVICE_DATA,register __d0 ULONG);
extern ULONG HcGetState(register __a0 PHCD_DEVICE_DATA);
// Setta / ottieni lo stato USB
//	HC_USB_RESET
//	HC_USB_RESUME
//	HC_USB_OPERATIONAL
//	HC_USB_SUSPEND

extern void  HcListEnable(register __a0 PHCD_DEVICE_DATA,register __d0 ULONG);
extern void  HcListDisable(register __a0 PHCD_DEVICE_DATA,register __d0 ULONG);
// Abilita / disabilita l'elaborazione delle liste
//	HC_LIST_PERIODIC
//	HC_LIST_CONTROL
//	HC_LIST_BULK
//	HC_LIST_ISOCHRONOUS


extern void  HcSetCBSR(register __a0 PHCD_DEVICE_DATA,register __d0 ULONG);
// Setta Control Bulk Service Ratio
//	0,1,2,3

extern LONG  HcGetInterruptStatus(register __a0 PHCD_DEVICE_DATA);
// Ottieni lo stato degli interrupt

extern void  HcInterruptRoutingEnable(register __a0 PHCD_DEVICE_DATA);
// Abilita l'interrupt Routing (Su Amiga va disabilitato)

extern void  HcInterruptRoutingDisable(register __a0 PHCD_DEVICE_DATA);
// disabilita l'interrupt routing (gli interrupt sono raccolti dal bus PCI)

extern void  HcRemoteWakeupEnable(register __a0 PHCD_DEVICE_DATA);
// Abilita la caratteristica del Remote Wakeup

extern void  HcRemoteWakeupDisable(register __a0 PHCD_DEVICE_DATA);
// Disabilita il Remote Wakeup

extern char  HcRemoteWakeupConnected(register __a0 PHCD_DEVICE_DATA);
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

extern void  HcSoftReset(register __a0 PHCD_DEVICE_DATA);
// Inizia un Software Reset:
//      Azzera Tutti i bit Dell'Host Controller
//	NON TOCCA la sezione della Root Hub.

extern void  HcChangeRequest(register __a0 PHCD_DEVICE_DATA);
// Forza il cambiamento dei dati dell'Host Controller
// Causa un interrupt e il cambiamento viene effettuato dalla routine di servizio

extern void  HcSetBulkListFilled(register __a0 PHCD_DEVICE_DATA);
// Obbliga l'host ad elaborare le liste BULK

extern void  HcSetControlListFilled(register __a0 PHCD_DEVICE_DATA);
// Obbliga l'host ad elaborare le liste CONTROL

extern char  HcGetSchedulingOverrun(register __a0 PHCD_DEVICE_DATA);
// Ottieni il totale degli scheduling overrun

extern void  HcInterruptEnable(register __a0 PHCD_DEVICE_DATA,register __d0 ULONG);
extern void  HcInterruptDisable(register __a0 PHCD_DEVICE_DATA,register __d0 ULONG);
// Abilita / Disabilita i segnali di interrupt:
//	HC_INT_RHSC	root hub status change
//	HC_INT_FNO	frame number overflow
//	HC_INT_UE	unexpected error
//	HC_INT_RD	resume detect
//	HC_INT_SOF	start of frame
//	HC_INT_WDH	writeback done head
//	HC_INT_SO	scheduling overrun
//	HC_INT_OC	ownership change request

extern void  HcInterruptSetSF(register __a0 PHCD_DEVICE_DATA);
extern void  HcInterruptClearSF(register __a0 PHCD_DEVICE_DATA);
//  Abilita / Disabilita lo Start of Frame interrupt

extern void  HcInterruptSet(register __a0 PHCD_DEVICE_DATA);
extern void  HcInterruptClear(register __a0 PHCD_DEVICE_DATA);
//  Abilita / Disabilita TUTTI i segnali di interrupt


//=============================================================================
//-----------------------------------------------------------------------------
//
//	Host Controller : Sezione Liste
//
//-----------------------------------------------------------------------------
//=============================================================================


extern LONG  HcSetHCCA(register __a0 PHCD_DEVICE_DATA,register __d0 ULONG);
// Inizializza Area HCCA

extern PENDPOINT_DESCRIPTOR	HcGetPeriodCurrentED (register __a0 PHCD_DEVICE_DATA);
extern PENDPOINT_DESCRIPTOR	HcGetControlCurrentED(register __a0 PHCD_DEVICE_DATA);
extern PENDPOINT_DESCRIPTOR	HcGetBulkCurrentED   (register __a0 PHCD_DEVICE_DATA);
// Ottieni l'indirizzo dell'Endpoint Periodico-Controllo-Bulk in elaborazione 

extern PENDPOINT_DESCRIPTOR	HcGetControlHeadED(register __a0 PHCD_DEVICE_DATA);
extern PENDPOINT_DESCRIPTOR	HcGetBulkHeadED   (register __a0 PHCD_DEVICE_DATA);
// Ottieni la TESTA della lista degli Endpoint Control-Bulk

extern void	HcSetControlHeadED(register __a0 PHCD_DEVICE_DATA,register __d0 PENDPOINT_DESCRIPTOR);
extern void	HcSetBulkHeadED   (register __a0 PHCD_DEVICE_DATA,register __d0 PENDPOINT_DESCRIPTOR);
// Inizializza la TESTA della lista degli Endpoint Control-Bulk

extern PTRANSFER_DESCRIPTOR HcGetDoneHead(register __a0 PHCD_DEVICE_DATA);
// ottieni la testa dei Transfer Descriptor elaborati.


//=============================================================================
//-----------------------------------------------------------------------------
//
//	Host Controller : Sezione Frame
//
//-----------------------------------------------------------------------------
//=============================================================================


extern UWORD HcGetFrameInterval(register __a0 PHCD_DEVICE_DATA);
extern void  HcSetFrameInterval(register __a0 PHCD_DEVICE_DATA,register __d0 UWORD);
// Ricava e inizializza il frame interval ( 11999 di default )

extern void  HcSetFSMPS(register __a0 PHCD_DEVICE_DATA,register __d0 UWORD);
// Inizializza il FS Largest Data Packet 
// (ricorda che è la + grande quantità di dati inviablili prima di causare Schedul. Overrun)

extern UWORD HcGetFrameNumber(register __a0 PHCD_DEVICE_DATA);
// Ottieni il FrameNumber

extern void  HcSetPeriodicStart(register __a0 PHCD_DEVICE_DATA,register __d0 UWORD);
extern UWORD HcGetPeriodicStart(register __a0 PHCD_DEVICE_DATA);
// Inizalizza / Ottieni il periodic start: usualmente vale 
//	0x3e67:
//	si ottiene come HcGetFrameInterval()+HcGetFrameInterval()/10;

extern void  HcSetThreShold(register __a0 PHCD_DEVICE_DATA);
// Inizializza il ThreShold, ovvero fa decidere all'Host se è il caso di avviare una 
// tranzazione a bassa velocità...

//=============================================================================
//-----------------------------------------------------------------------------
//
//	Host Controller : Sezione ROOH HUB
//
//-----------------------------------------------------------------------------
//=============================================================================

extern char	HcRhGetNumberDownstreamPorts(register __a0 PHCD_DEVICE_DATA);
// Ottieni il numero di porte nell'host controller;

extern char	HcRhGetPOTPGT(register __a0 PHCD_DEVICE_DATA);
// Ottieni il tempo MINIMO per accedere ad una porta DOPO che la porta è
// stata alimentata:
// TempoDiAttesa = HcRhGetPOTPGT(HostController) * 2ms;
//
// Usualmente TempoDiAttesa= 255 * 2ms = 1<<8+1<<1 = 1 << 9 = 512 ms = Delay(25);

extern char	HcRhGetPowerSwitchingMode(register __a0 PHCD_DEVICE_DATA);
// Ottieni il tipo di PowerSwitch
//	RH_NO_POWERSWITCH			porte sempre alimentate
//	RH_GLOBAL_POWERSWITCH			pulsante globale
//	RH_PORT_X_PORT_POWERSWITCH		pulsante porta per porta

extern char	HcRhGetOvercurrentProtectionMode(register __a0 PHCD_DEVICE_DATA);
// Ottieni il tipo di Protezione da sovralimentazione
//	RH_NO_OVERCURRENT_PROTECTION		nessun tipo di controllo
//	RH_GLOBAL_OVERCURRENT_PROTECTION	pulsante globale
//	RH_PORT_X_PORT_POWERSWITCH		pulsante porta per porta
	
extern char	HcRhIsDeviceRemovable(register __a0 PHCD_DEVICE_DATA,register __d0 char);
// Dice se il dispositivo nella porta X è rimuovibile 
//	X = 1..15

extern char	HcRhIsDevicePowered(register __a0 PHCD_DEVICE_DATA,register __d0 char);
// Dice se il dispositivo nella porta X è alimentato 
//	X = 1..15

extern void	HcRhClearGlobalPower(register __a0 PHCD_DEVICE_DATA);
extern void	HcRhSetGlobalPower(register __a0 PHCD_DEVICE_DATA);
// Spegni-Accendi tutte le porte 
// (funziona solo se l'host è dotato di Global Powerswitch)

extern void	HcRhSetRemoteWakeup(register __a0 PHCD_DEVICE_DATA);
extern void	HcRhClearRemoteWakeup(register __a0 PHCD_DEVICE_DATA);
// Abilita / Disabilita il segnale di Device Remote Wakeup enable

extern char	HcRhGetGlobalOvercurrent(register __a0 PHCD_DEVICE_DATA);
// Ottieni lo stato di global OverCurrent
// (funziona solo se l'host è dotato di Global Powerswitch)

extern char	HcRhGetOvercurrentIndicatorChange(register __a0 PHCD_DEVICE_DATA);
// Verifica se si è verificato un cambiamento nel segnale di overcurrent

extern void	HcRhClearOvercurrentIndicatorChange(register __a0 PHCD_DEVICE_DATA);
// Azzera la caratteristica


//=============================================================================
//-----------------------------------------------------------------------------
//
//	Host Controller : Sezione ROOH HUB PORTS
//
//-----------------------------------------------------------------------------
//=============================================================================

extern char 	HcRhPort_GetPortPowerStatus(register __a0 PHCD_DEVICE_DATA,register __d0 char);
//  0 = Port Power X OFF     ( X = 1..15 )
//  1 = Port Power X ON

extern void 	HcRhPort_SetPortPower(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern void 	HcRhPort_ClearPortPower(register __a0 PHCD_DEVICE_DATA,register __d0 char);
//  Accende / Spegne la porta la porta X ( X = 1..15)
//  (valido solo nel caso che la porta sia gestita da Powerswitch porta per porta)

extern char 	HcRhPort_GetCurrentConnectStatus(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern char 	HcRhPort_GetConnectStatusChange(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern void 	HcRhPort_ClearConnectStatusChange(register __a0 PHCD_DEVICE_DATA,register __d0 char);
// Avvisa se un dispositivo è connesso alla porta X (X = 1..15)
//	  1 = Dispositivo connesso alla porta X 
//	  0 = Dispositivo non connesso alla porta X 
// Verifica se è cambiato lo stato
// Avvisa che hai capito che è cambiato lo stato

extern void 	HcRhPort_ClearPortEnable(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern void 	HcRhPort_SetPortEnable(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern char 	HcRhPort_GetPortEnableStatus(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern char 	HcRhPort_GetPortEnablestatusChange(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern void	HcRhPort_ClearPortEnableStatusChange(register __a0 PHCD_DEVICE_DATA,register __d0 char);
// Abilita / Disabilita la porta X (X = 1..15)
//
// Controlla lo stato della porta X  (1 = Porta X Abilitata; 0 = Porta X Disabilitata)
// Verifica se è cambiato lo stato
// Avvisa che hai capito che è cambiato lo stato

extern void 	HcRhPort_SetPortSuspend(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern void 	HcRhPort_ClearSuspendStatus(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern char 	HcRhPort_GetPortSuspendStatus(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern char 	HcRhPort_GetPortSuspendStatusChange(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern void 	HcRhPort_ClearPortSuspendSatatusChange(register __a0 PHCD_DEVICE_DATA,register __d0 char);
// Sospende / Riabilita la porta
//
// Controlla lo stato della porta X  (1 = Porta X Sospesa; 0 = Porta X Non Sospesa)
// Verifica se è cambiato lo stato
// Avvisa che hai capito che è cambiato lo stato


extern void 	HcRhPort_SetPortReset(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern char 	HcRhPort_GetPortResetStatus(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern char 	HcRhPort_GetPortResetStatusChange(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern void 	HcRhPort_ClearPortResetstatusChange(register __a0 PHCD_DEVICE_DATA,register __d0 char);
// Avvia un reset lungo la porta
// Controlla lo stato della porta X  (1 = Porta X si resetta; 0 = Porta X non in fase di reset)
// Verifica se è cambiato lo stato
// Avvisa che hai capito che è cambiato lo stato

extern char 	HcRhPort_GetPortOvercurrentIndicator(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern char 	HcRhPort_GetPortOvercurrentIndicatorChange(register __a0 PHCD_DEVICE_DATA,register __d0 char);
extern void 	HcRhPort_ClearPortOvercurrentIndicatorChange(register __a0 PHCD_DEVICE_DATA,register __d0 char);
// Controlla lo stato dell'overcurrent della porta X (X= 1..15) 
//  	0 = No OverCurrent
//	1 = Overcurrent
// Verifica se è cambiato lo stato
// Avvisa che hai capito che è cambiato lo stato

extern char 	HcRhPort_LowSpeedDeviceAttached(register __a0 PHCD_DEVICE_DATA,register __d0 char);
//	0 = HighSpeed device attached
//	1 = Low Speed Device attached

#endif