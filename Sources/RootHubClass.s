
; Storm C Compiler
; Compiler:Stormc/Include/Usb/RootHubClass.c
	mc68030

	XREF	_HcRhPort_ClearPortOvercurrentIndicatorChange
	XREF	_HcRhPort_ClearPortResetstatusChange
	XREF	_HcRhPort_SetPortReset
	XREF	_HcRhPort_ClearPortSuspendSatatusChange
	XREF	_HcRhPort_ClearSuspendStatus
	XREF	_HcRhPort_SetPortSuspend
	XREF	_HcRhPort_ClearPortEnableStatusChange
	XREF	_HcRhPort_ClearPortEnable
	XREF	_HcRhPort_ClearConnectStatusChange
	XREF	_HcRhPort_ClearPortPower
	XREF	_HcRhPort_SetPortPower
	XREF	_HcRhClearOvercurrentIndicatorChange
	XREF	_HcRhClearGlobalPower
	XREF	_ConvertLong
	XREF	_WaitPort
	XREF	_GetMsg

	SECTION ":0",CODE


;UBYTE ClearHubFeature(PUSB_REQUESTER Req,
	XDEF	_ClearHubFeature
_ClearHubFeature
	move.b	$10(a7),d0
	move.l	4(a7),a0
L25
;  switch(FEATURE_SELECTOR)
	cmp.b	#0,d0
	beq.b	L26
	cmp.b	#1,d0
	beq.b	L27
	bra.b	L29
;  
;   
L26
;   case C_HUB_LOCAL_POWER:  HcRhClearGlo
	move.l	(a0),a0
	jsr	_HcRhClearGlobalPower
;b
	bra.b	L29
L27
;   case C_HUB_OVER_CURRENT: HcRhClearOve
	move.l	(a0),a0
	jsr	_HcRhClearOvercurrentIndicatorChange
;b
L28
;   default:
L29
;  return(1);
	moveq	#1,d0
	rts

;void ClearPortFeature(PUSB_REQUESTER Req,
	XDEF	_ClearPortFeature
_ClearPortFeature
	move.b	$14(a7),d0
	move.l	$10(a7),d1
	move.l	4(a7),a0
L30
;  switch(FEATURE_SELECTOR)
	cmp.l	#$11,d1
	beq	L36
	bhi.b	L41
	cmp.l	#$8,d1
	beq.b	L33
	bhi.b	L42
	cmp.l	#1,d1
	beq.b	L31
	cmp.l	#2,d1
	beq.b	L32
	bra.b	L40
L42
	cmp.l	#$10,d1
	beq.b	L34
	bra.b	L40
L41
	cmp.l	#$13,d1
	beq.b	L38
	bhi.b	L43
	blo.b	L37
L43
	cmp.l	#$14,d1
	beq.b	L35
	bra.b	L40
;  
;   
L31
;   case PORT_ENABLE:        HcRhPort_Cle
	move.l	(a0),a0
	jsr	_HcRhPort_ClearPortEnable
;b
	bra.b	L40
L32
;   case PORT_SUSPEND:       HcRhPort_Cle
	move.l	(a0),a0
	jsr	_HcRhPort_ClearSuspendStatus
;b
	bra.b	L40
L33
;   case PORT_POWER:         HcRhPort_Cle
	move.l	(a0),a0
	jsr	_HcRhPort_ClearPortPower
;b
	bra.b	L40
L34
;   case C_PORT_CONNECTION:  HcRhPort_Cle
	move.l	(a0),a0
	jsr	_HcRhPort_ClearConnectStatusChange
;b
	bra.b	L40
L35
;   case C_PORT_RESET:       HcRhPort_Cle
	move.l	(a0),a0
	jsr	_HcRhPort_ClearPortResetstatusChange
;b
	bra.b	L40
L36
;   case C_PORT_ENABLE:      HcRhPort_Cle
	move.l	(a0),a0
	jsr	_HcRhPort_ClearPortEnableStatusChange
;b
	bra.b	L40
L37
;   case C_PORT_SUSPEND:     HcRhPort_Cle
	move.l	(a0),a0
	jsr	_HcRhPort_ClearPortSuspendSatatusChange
;b
	bra.b	L40
L38
;   case C_PORT_OVER_CURRENT:HcRhPort_Cle
	move.l	(a0),a0
	jsr	_HcRhPort_ClearPortOvercurrentIndicatorChange
;b
L39
;   default:
L40
	rts

;UBYTE GetBusState(PUSB_REQUESTER Req,
	XDEF	_GetBusState
_GetBusState
L44
; return(3);
	moveq	#3,d0
	rts

;PHUB_DESCRIPTOR GetHubDescriptor(PUSB_REQUESTER Req,
	XDEF	_GetHubDescriptor
_GetHubDescriptor
	movem.l	d2/d3/a2,-(a7)
	move.l	$10(a7),a2
L45
; ULONG A=ConvertLong(Req->devicedata->HC->HcRhDescriptorA);
	move.l	(a2),a0
	move.l	$38(a0),a0
	move.l	$48(a0),d0
	jsr	_ConvertLong
	move.l	d0,d2
; ULONG B=ConvertLong(Req->devicedata->HC->HcRhDescriptorB);
	move.l	(a2),a0
	move.l	$38(a0),a0
	move.l	$4C(a0),d0
	jsr	_ConvertLong
; RHub.bNbrPorts           = A & 0xFF;
	move.l	d2,d1
	and.l	#$FF,d1
	move.b	d1,_RHub+2
; RHub.wHubCharacteristics = (A >> 8) & 0xFFFF;
	move.l	d2,d1
	moveq	#$8,d3
	lsr.l	d3,d1
	and.l	#$FFFF,d1
	move.w	d1,_RHub+4
; RHub.bPwrOn2PwrGood      = (A >>24) & 0xFF;
	moveq	#$18,d1
	lsr.l	d1,d2
	and.l	#$FF,d2
	move.b	d2,_RHub+6
; RHub.DeviceRemovable     = B & 0xFFFF;
	move.l	d0,d1
	and.l	#$FFFF,d1
	move.w	d1,_RHub+$8
; RHub.PortPwrCtrlMask     = (B >>16) & 0xFFFF;
	moveq	#$10,d1
	lsr.l	d1,d0
	and.l	#$FFFF,d0
	move.w	d0,_RHub+$A
; return(&RHub);
	move.l	#_RHub,d0
	movem.l	(a7)+,d2/d3/a2
	rts

;void FreeHubDescriptor(PHUB_DESCRIPTOR hub)
	XDEF	_FreeHubDescriptor
_FreeHubDescriptor
L46
; hub=NULL;
	rts

;ULONG GetHubStatus(PUSB_REQUESTER  Req,
	XDEF	_GetHubStatus
_GetHubStatus
	move.l	4(a7),a0
L47
; return(ConvertLong(Req->devicedata->HC->HcRhStatus));
	move.l	(a0),a0
	move.l	$38(a0),a0
	move.l	$50(a0),d0
	jsr	_ConvertLong
	rts

;ULONG GetPortStatus(PUSB_REQUESTER Req,
	XDEF	_GetPortStatus
_GetPortStatus
	move.b	$10(a7),d0
	move.l	4(a7),a0
L48
; return(ConvertLong(Req->devicedata->HC->HcRhPortStatus[(PortNumber
	move.l	(a0),a0
	move.l	$38(a0),a0
	lea	$54(a0),a0
	and.l	#$FF,d0
	lea	-4(a0),a0
	move.l	0(a0,d0.l*4),d0
	jsr	_ConvertLong
	rts

;UBYTE SetHubDescriptor(PUSB_REQUESTER Req,
	XDEF	_SetHubDescriptor
_SetHubDescriptor
L49
; return(1);
	moveq	#1,d0
	rts

;UBYTE SetHubFeature(PUSB_REQUESTER Req,
	XDEF	_SetHubFeature
_SetHubFeature
L50
; switch(FEATURE_SELECTOR)
; 
;  
;  default:
; return(1);
	moveq	#1,d0
	rts

;void SetPortFeature(PUSB_REQUESTER Req,
	XDEF	_SetPortFeature
_SetPortFeature
	move.b	$14(a7),d0
	move.l	$10(a7),d1
	move.l	4(a7),a0
L51
; switch (FEATURE_SELECTOR)
	cmp.l	#4,d1
	beq.b	L52
	bhi.b	L56
	cmp.l	#2,d1
	beq.b	L53
	bra.b	L55
L56
	cmp.l	#$8,d1
	beq.b	L54
	bra.b	L55
; 
;  
L52
;  case PORT_RESET:  HcRhPort_SetPortReset  (Req-
	move.l	(a0),a0
	jsr	_HcRhPort_SetPortReset
;b
	bra.b	L55
L53
;  case PORT_SUSPEND:HcRhPort_SetPortSuspend(Req-
	move.l	(a0),a0
	jsr	_HcRhPort_SetPortSuspend
;b
	bra.b	L55
L54
;  case PORT_POWER:  HcRhPort_SetPortPower  (Req-
	move.l	(a0),a0
	jsr	_HcRhPort_SetPortPower
;b
L55
;  deafult:
	rts

;void WaitCSC(PUSB_REQUESTER Req,
	XDEF	_WaitCSC
_WaitCSC
	move.l	a2,-(a7)
	move.l	$8(a7),a2
L57
; WaitPort(Req->UserPort);
	move.l	4(a2),-(a7)
	jsr	_WaitPort
	addq.w	#4,a7
; while(Req->DummyMess=GetMsg(Req->UserPort));
L58
	move.l	4(a2),-(a7)
	jsr	_GetMsg
	addq.w	#4,a7
	move.l	d0,$36(a2)
	bne.b	L58
L59
	move.l	(a7)+,a2
	rts

	SECTION ":1",DATA

	XDEF	_RHDevice
_RHDevice
	dc.b	$12,0
	dc.w	0
	dc.b	0,$9,0,0
	dc.w	0,0,0
	dc.b	0,0,0,1
	CNOP	0,4
	XDEF	_RHConfiguration
_RHConfiguration
	dc.b	$A,0
	dc.w	0
	dc.b	1,0,0,0,-6
	CNOP	0,4
	XDEF	_RHInterface
_RHInterface
	dc.b	$A,0,0,0,1,$9,0,0,0
	CNOP	0,4
	XDEF	_RHEndpoint
_RHEndpoint
	dc.b	$8,0,1,7
	dc.w	$8
	dc.b	-1
	CNOP	0,4
	XDEF	_RHub
_RHub
	dc.b	$C,$29,0
	ds.b	1
	dc.w	0
	dc.b	0,$FA
	dc.w	0,0

	END
