//=============================================================================
//
//   	HC_ENDPOINT_DESCRIPTOR Assembly API
//      HC_TRANSFER_DESCRIPTOR Assembly API
//      ( Zioben 2002 )
//=============================================================================

#include <exec/types.h>

#ifndef ASMENDPOINT_PROTOS
#define	ASMENDPOINT_PROTOS

#include <usb/Hcdriverstructs.h>

//;---------------------------------
//;Endpoint Descriptor hardware
//;---------------------------------
extern  void 	Set_ED_MaxPackSize(register __a0 PENDPOINT_DESCRIPTOR, register __d0 UWORD);
extern  WORD 	Get_ED_MaxPackSize(register __a0 PENDPOINT_DESCRIPTOR);

extern  void	Set_ED_Format(register __a0 PENDPOINT_DESCRIPTOR, register __d0 char);
extern 	char	Get_ED_Format(register __a0 PENDPOINT_DESCRIPTOR);

extern  void	Set_ED_sKip(register __a0 PENDPOINT_DESCRIPTOR, register __d0 char);
extern	char	Get_ED_sKip(register __a0 PENDPOINT_DESCRIPTOR);

extern	void 	Set_ED_Speed(register __a0 PENDPOINT_DESCRIPTOR, register __d0 char);
extern 	char	Get_ED_Speed(register __a0 PENDPOINT_DESCRIPTOR);

extern 	void	Set_ED_Direction(register __a0 PENDPOINT_DESCRIPTOR, register __d0 char);
extern 	char	Get_ED_Direction(register __a0 PENDPOINT_DESCRIPTOR);

extern	void	Set_ED_EndpointNumber(register __a0 PENDPOINT_DESCRIPTOR, register __d0 char);
extern	char	Get_ED_EndpointNumber(register __a0 PENDPOINT_DESCRIPTOR);

extern	void	Set_ED_FunctionAddress(register __a0 PENDPOINT_DESCRIPTOR, register __d0 char);
extern	char	Get_ED_FunctionAddress(register __a0 PENDPOINT_DESCRIPTOR);

extern	void	Set_ED_TailP(register __a0 PENDPOINT_DESCRIPTOR, register __d0 PTRANSFER_DESCRIPTOR);
extern	PTRANSFER_DESCRIPTOR	Get_ED_TailP(register __a0 PENDPOINT_DESCRIPTOR);

extern	void 	Set_ED_HeadP(register __a0 PENDPOINT_DESCRIPTOR, register __d0 PTRANSFER_DESCRIPTOR);
extern	PTRANSFER_DESCRIPTOR	Get_ED_HeadP(register __a0 PENDPOINT_DESCRIPTOR);

extern	void	Set_ED_Carry(register __a0 PENDPOINT_DESCRIPTOR, register __d0 char);
extern	char	Get_ED_Carry(register __a0 PENDPOINT_DESCRIPTOR);

extern	void 	Set_ED_Halt(register __a0 PENDPOINT_DESCRIPTOR, register __d0 char);
extern	char	Get_ED_Halt(register __a0 PENDPOINT_DESCRIPTOR);

extern	void 	Set_ED_NextED(register __a0 PENDPOINT_DESCRIPTOR, register __d0 PENDPOINT_DESCRIPTOR);
extern	PENDPOINT_DESCRIPTOR	Get_ED_NextED(register __a0 PENDPOINT_DESCRIPTOR);

extern  ULONG	Get_ED_Control(register __a0 PENDPOINT_DESCRIPTOR);
//;---------------------------------
//;Transfer Descriptor hardware
//;---------------------------------
extern	void 	Set_TD_ConditionCode(register __a0 PTRANSFER_DESCRIPTOR, register __d0 char);
extern	char	Get_TD_ConditionCode(register __a0 PTRANSFER_DESCRIPTOR);

extern	void 	Set_TD_ErrorCount(register __a0 PTRANSFER_DESCRIPTOR, register __d0 char);
extern	char	Get_TD_ErrorCount(register __a0 PTRANSFER_DESCRIPTOR);

extern	void	Set_TD_Toggle(register __a0 PTRANSFER_DESCRIPTOR, register __d0 char);
extern	char	Get_TD_Toggle(register __a0 PTRANSFER_DESCRIPTOR);

extern	void 	Set_TD_DelayInterrupt(register __a0 PTRANSFER_DESCRIPTOR, register __d0 char);
extern	char	Get_TD_DelayInterrupt(register __a0 PTRANSFER_DESCRIPTOR);

extern	void 	Set_TD_DirectionPid(register __a0 PTRANSFER_DESCRIPTOR, register __d0 char);
extern	char	Get_TD_DirectionPid(register __a0 PTRANSFER_DESCRIPTOR);

extern	void 	Set_TD_Rounding(register __a0 PTRANSFER_DESCRIPTOR, register __d0 char);
extern	char	Get_TD_Rounding(register __a0 PTRANSFER_DESCRIPTOR);

extern	void	Set_TD_CurrentBufferPointer(register __a0 PTRANSFER_DESCRIPTOR, register __d0 char*);
extern	char*	Get_TD_CurrentBufferPointer(register __a0 PTRANSFER_DESCRIPTOR);

extern	void 	Set_TD_NextTD(register __a0 PTRANSFER_DESCRIPTOR, register __d0 PTRANSFER_DESCRIPTOR);
extern	PTRANSFER_DESCRIPTOR	Get_TD_NextTD(register __a0 PTRANSFER_DESCRIPTOR);

extern	void	Set_TD_BufferEnd(register __a0 PTRANSFER_DESCRIPTOR, register __d0 char*);
extern	char*	Get_TD_BufferEnd(register __a0 PTRANSFER_DESCRIPTOR);

extern  ULONG	Get_TD_Control(register __a0 PTRANSFER_DESCRIPTOR);

#endif