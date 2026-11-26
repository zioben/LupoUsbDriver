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
extern  void 	Set_ED_MaxPackSize(PENDPOINT_DESCRIPTOR _a0 __asm("a0"), UWORD _d0 __asm("d0"));
extern  WORD 	Get_ED_MaxPackSize(PENDPOINT_DESCRIPTOR _a0 __asm("a0"));

extern  void	Set_ED_Format(PENDPOINT_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern 	char	Get_ED_Format(PENDPOINT_DESCRIPTOR _a0 __asm("a0"));

extern  void	Set_ED_sKip(PENDPOINT_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern	char	Get_ED_sKip(PENDPOINT_DESCRIPTOR _a0 __asm("a0"));

extern	void 	Set_ED_Speed(PENDPOINT_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern 	char	Get_ED_Speed(PENDPOINT_DESCRIPTOR _a0 __asm("a0"));

extern 	void	Set_ED_Direction(PENDPOINT_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern 	char	Get_ED_Direction(PENDPOINT_DESCRIPTOR _a0 __asm("a0"));

extern	void	Set_ED_EndpointNumber(PENDPOINT_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern	char	Get_ED_EndpointNumber(PENDPOINT_DESCRIPTOR _a0 __asm("a0"));

extern	void	Set_ED_FunctionAddress(PENDPOINT_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern	char	Get_ED_FunctionAddress(PENDPOINT_DESCRIPTOR _a0 __asm("a0"));

extern	void	Set_ED_TailP(PENDPOINT_DESCRIPTOR _a0 __asm("a0"), PTRANSFER_DESCRIPTOR _d0 __asm("d0"));
extern	PTRANSFER_DESCRIPTOR	Get_ED_TailP(PENDPOINT_DESCRIPTOR _a0 __asm("a0"));

extern	void 	Set_ED_HeadP(PENDPOINT_DESCRIPTOR _a0 __asm("a0"), PTRANSFER_DESCRIPTOR _d0 __asm("d0"));
extern	PTRANSFER_DESCRIPTOR	Get_ED_HeadP(PENDPOINT_DESCRIPTOR _a0 __asm("a0"));

extern	void	Set_ED_Carry(PENDPOINT_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern	char	Get_ED_Carry(PENDPOINT_DESCRIPTOR _a0 __asm("a0"));

extern	void 	Set_ED_Halt(PENDPOINT_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern	char	Get_ED_Halt(PENDPOINT_DESCRIPTOR _a0 __asm("a0"));

extern	void 	Set_ED_NextED(PENDPOINT_DESCRIPTOR _a0 __asm("a0"), PENDPOINT_DESCRIPTOR _d0 __asm("d0"));
extern	PENDPOINT_DESCRIPTOR	Get_ED_NextED(PENDPOINT_DESCRIPTOR _a0 __asm("a0"));

extern  ULONG	Get_ED_Control(PENDPOINT_DESCRIPTOR _a0 __asm("a0"));
//;---------------------------------
//;Transfer Descriptor hardware
//;---------------------------------
extern	void 	Set_TD_ConditionCode(PTRANSFER_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern	char	Get_TD_ConditionCode(PTRANSFER_DESCRIPTOR _a0 __asm("a0"));

extern	void 	Set_TD_ErrorCount(PTRANSFER_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern	char	Get_TD_ErrorCount(PTRANSFER_DESCRIPTOR _a0 __asm("a0"));

extern	void	Set_TD_Toggle(PTRANSFER_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern	char	Get_TD_Toggle(PTRANSFER_DESCRIPTOR _a0 __asm("a0"));

extern	void 	Set_TD_DelayInterrupt(PTRANSFER_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern	char	Get_TD_DelayInterrupt(PTRANSFER_DESCRIPTOR _a0 __asm("a0"));

extern	void 	Set_TD_DirectionPid(PTRANSFER_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern	char	Get_TD_DirectionPid(PTRANSFER_DESCRIPTOR _a0 __asm("a0"));

extern	void 	Set_TD_Rounding(PTRANSFER_DESCRIPTOR _a0 __asm("a0"), char _d0 __asm("d0"));
extern	char	Get_TD_Rounding(PTRANSFER_DESCRIPTOR _a0 __asm("a0"));

extern	void	Set_TD_CurrentBufferPointer(PTRANSFER_DESCRIPTOR _a0 __asm("a0"), char* _d0 __asm("d0"));
extern	char*	Get_TD_CurrentBufferPointer(PTRANSFER_DESCRIPTOR _a0 __asm("a0"));

extern	void 	Set_TD_NextTD(PTRANSFER_DESCRIPTOR _a0 __asm("a0"), PTRANSFER_DESCRIPTOR _d0 __asm("d0"));
extern	PTRANSFER_DESCRIPTOR	Get_TD_NextTD(PTRANSFER_DESCRIPTOR _a0 __asm("a0"));

extern	void	Set_TD_BufferEnd(PTRANSFER_DESCRIPTOR _a0 __asm("a0"), char* _d0 __asm("d0"));
extern	char*	Get_TD_BufferEnd(PTRANSFER_DESCRIPTOR _a0 __asm("a0"));

extern  ULONG	Get_TD_Control(PTRANSFER_DESCRIPTOR _a0 __asm("a0"));

#endif