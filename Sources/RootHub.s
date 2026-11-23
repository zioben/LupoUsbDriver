
; Storm C Compiler
; Compiler:Stormc/Include/Usb/RootHub.c
	mc68030

	XREF	_WaitCSC
	XREF	_SetPortFeature
	XREF	_GetPortStatus
	XREF	_GetHubStatus
	XREF	_ClearPortFeature
	XREF	_FreeUsbRequester
	XREF	_InitUsbRequester
	XREF	_AllocUsbRequester
	XREF	_SetupRhMessagePort
	XREF	_DeletePort
	XREF	_CreatePort
	XREF	_Delay
	XREF	_FindPort
	XREF	_GetMsg
	XREF	_printf
	XREF	_std__in
	XREF	_std__out
	XREF	_std__err

	SECTION ":0",CODE


	XDEF	_INIT_8_RootHub_c
_INIT_8_RootHub_c
L27
;struct MsgPort *ReplyPort =CreatePort(UsbKernelRhPortName,0L);
	clr.l	-(a7)
	move.l	#L26,-(a7)
	jsr	_CreatePort
	addq.w	#$8,a7
	move.l	d0,_ReplyPort
;PUSB_REQUESTER Req=NULL;
	rts

;void ProcessHubStatusRegister(ULONG HubStatus)
	XDEF	_ProcessHubStatusRegister
_ProcessHubStatusRegister
L28
	rts

;void ProcessPort(ULONG PortStatus,UBYTE i)
	XDEF	_ProcessPort
_ProcessPort
	movem.l	d2/d3,-(a7)
	move.l	$C(a7),d2
	move.b	$10(a7),d3
L29
; if(PortStatus & (1<<C_PORT_CONNECTION))
	move.l	d2,d0
	and.l	#$10000,d0
	beq.b	L32
L30
; 
;   ClearPortFeature(Req,NULL,NULL,C_PORT_CONNECTION,i);
	move.b	d3,-(a7)
	pea	$10.w
	clr.l	-(a7)
	clr.l	-(a7)
	move.l	_Req,-(a7)
	jsr	_ClearPortFeature
	add.w	#$12,a7
;   if(PortStatus & (1<<PORT_CONNECTION))
	move.l	d2,d0
	and.l	#1,d0
	beq.b	L32
L31
;   
;     Delay(20);
	pea	$14.w
	jsr	_Delay
	addq.w	#4,a7
;     SetPortFeature(Req,NULL,NULL,PORT_RESET,i);
	move.b	d3,-(a7)
	pea	4.w
	clr.l	-(a7)
	clr.l	-(a7)
	move.l	_Req,-(a7)
	jsr	_SetPortFeature
	add.w	#$12,a7
L32
; if(PortStatus & (1<<C_PORT_RESET))
	move.l	d2,d0
	and.l	#$100000,d0
	beq.b	L34
L33
;   
;     Delay(10);
	pea	$A.w
	jsr	_Delay
	addq.w	#4,a7
;     ClearPortFeature(Req,NULL,NULL,C_PORT_RESET,i);
	move.b	d3,-(a7)
	pea	$14.w
	clr.l	-(a7)
	clr.l	-(a7)
	move.l	_Req,-(a7)
	jsr	_ClearPortFeature
	add.w	#$12,a7
L34
; if(PortStatus & (1<<C_PORT_ENABLE))
	move.l	d2,d0
	and.l	#$20000,d0
	beq.b	L36
L35
;   
;     ClearPortFeature(Req,NULL,NULL,C_PORT_ENABLE,i);
	move.b	d3,-(a7)
	pea	$11.w
	clr.l	-(a7)
	clr.l	-(a7)
	move.l	_Req,-(a7)
	jsr	_ClearPortFeature
	add.w	#$12,a7
L36
; if(PortStatus & (1<<C_PORT_SUSPEND))
	move.l	d2,d0
	and.l	#$40000,d0
	beq.b	L38
L37
;   
;     ClearPortFeature(Req,NULL,NULL,C_PORT_SUSPEND,i);
	move.b	d3,-(a7)
	pea	$12.w
	clr.l	-(a7)
	clr.l	-(a7)
	move.l	_Req,-(a7)
	jsr	_ClearPortFeature
	add.w	#$12,a7
L38
; if(PortStatus & (1<<C_PORT_OVER_CURRENT))
	and.l	#$80000,d2
	beq.b	L40
L39
;   
;     ClearPortFeature(Req,NULL,NULL,C_PORT_OVER_CURRENT,i);
	move.b	d3,-(a7)
	pea	$13.w
	clr.l	-(a7)
	clr.l	-(a7)
	move.l	_Req,-(a7)
	jsr	_ClearPortFeature
	add.w	#$12,a7
L40
	movem.l	(a7)+,d2/d3
	rts

;void ProcessHub()
	XDEF	_ProcessHub
_ProcessHub
	movem.l	d2/d3/a2,-(a7)
L41
;  while ( (HubMess=(PROOT_HUB_MESSAGE)GetMsg(Req->UserPort)) )
	bra	L49
L42
;  
;    if  (HubMess->Status_Change & 0x01)
	moveq	#0,d0
	move.w	$16(a2),d0
	and.l	#1,d0
	beq.b	L44
L43
;    
;           HubMess->Status_Change &=0xFFFE;
	and.w	#$FFFE,$16(a2)
;      	  ProcessHubStatusRegister(GetHubStatus(Req,NULL,NUL
	clr.l	-(a7)
	clr.l	-(a7)
	move.l	_Req,-(a7)
	jsr	_GetHubStatus
	add.w	#$C,a7
	move.l	d0,-(a7)
	jsr	_ProcessHubStatusRegister
	addq.w	#4,a7
L44
;    for (i=1;
	moveq	#1,d2
	bra.b	L48
L45
;	    if  ((HubMess->Status_Change)&(1<<i))
	moveq	#0,d0
	move.w	$16(a2),d0
	moveq	#0,d1
	move.b	d2,d1
	moveq	#1,d3
	asl.l	d1,d3
	and.l	d3,d0
	beq.b	L47
L46
;	    
;	        HubMess->Status_Change&=~(1<<i);
	moveq	#0,d0
	move.b	d2,d0
	moveq	#1,d1
	asl.l	d0,d1
	move.l	d1,d0
	not.l	d0
	move.w	d0,d1
	and.w	d1,$16(a2)
;   	     ProcessPort(GetPortStatus(Req,NULL,NULL,i),i);
	move.b	d2,-(a7)
	move.b	d2,-(a7)
	clr.l	-(a7)
	clr.l	-(a7)
	move.l	_Req,-(a7)
	jsr	_GetPortStatus
	add.w	#$E,a7
	move.l	d0,-(a7)
	jsr	_ProcessPort
	addq.w	#6,a7
L47
	addq.b	#1,d2
L48
	moveq	#0,d1
	move.b	d2,d1
	moveq	#0,d0
	move.w	$14(a2),d0
	addq.l	#1,d0
	cmp.l	d0,d1
	blt.b	L45
L49
	move.l	_Req,a0
	move.l	4(a0),-(a7)
	jsr	_GetMsg
	addq.w	#4,a7
	move.l	d0,a2
	cmp.w	#0,a2
	bne	L42
L50
	movem.l	(a7)+,d2/d3/a2
	rts

;void main(void)
	XDEF	_main
_main
L61
;  ULONG esci=0;
;  if ((Req=AllocUsbRequester()))
	jsr	_AllocUsbRequester
	move.l	d0,_Req
	tst.l	_Req
	beq	L73
L62
;  
;    printf("Hub Port Detected\n");
	move.l	#L51,-(a7)
	jsr	_printf
	addq.w	#4,a7
;    if(InitUsbRequester(Req,ReplyPort))
	move.l	_Req,a0
	move.l	_ReplyPort,a1
	jsr	_InitUsbRequester
	tst.b	d0
	beq	L71
L63
;    
;      if(SetupRhMessagePort(Req,ReplyPort))
	move.l	_Req,a0
	move.l	_ReplyPort,a1
	jsr	_SetupRhMessagePort
	tst.b	d0
	beq	L69
L64
;      
;        printf("Powering ports!\n");
	move.l	#L52,-(a7)
	jsr	_printf
	addq.w	#4,a7
;        SetPortFeature(Req,NULL,NULL,PORT_POWER,1);
	move.b	#1,-(a7)
	pea	$8.w
	clr.l	-(a7)
	clr.l	-(a7)
	move.l	_Req,-(a7)
	jsr	_SetPortFeature
	add.w	#$12,a7
;    	  SetPortFeature(Req,NULL,NULL,PORT_POWER,2);
	move.b	#2,-(a7)
	pea	$8.w
	clr.l	-(a7)
	clr.l	-(a7)
	move.l	_Req,-(a7)
	jsr	_SetPortFeature
	add.w	#$12,a7
;   	  printf("now Looping\n");
	move.l	#L53,-(a7)
	jsr	_printf
	addq.w	#4,a7
;    	  while(FindPort(UsbKernelHcPortName))
	bra.b	L67
L65
;    	  
;    	     printf("Waiting hub\n");
	move.l	#L54,-(a7)
	jsr	_printf
	addq.w	#4,a7
;        	  WaitCSC(Req,NULL);
	clr.l	-(a7)
	move.l	_Req,-(a7)
	jsr	_WaitCSC
	addq.w	#$8,a7
;      	  printf("Processing hub\n");
	move.l	#L55,-(a7)
	jsr	_printf
	addq.w	#4,a7
;      	  if(FindPort(UsbKernelHcPortName)) 
	move.l	#L56,-(a7)
	jsr	_FindPort
	addq.w	#4,a7
	tst.l	d0
	beq.b	L67
L66
;      	  if(FindPort(UsbK
	jsr	_ProcessHub
L67
	move.l	#L56,-(a7)
	jsr	_FindPort
	addq.w	#4,a7
	tst.l	d0
	bne.b	L65
L68
;    	  printf("Usb Kernel Killed, now Quitting\n");
	move.l	#L57,-(a7)
	jsr	_printf
	addq.w	#4,a7
	bra.b	L74
L69
;    	else printf("Cannot Setup Root Hub Port\n");
	move.l	#L58,-(a7)
	jsr	_printf
	addq.w	#4,a7
L70
	bra.b	L74
L71
;     else printf("Cannot Init Usb Requester\n");
	move.l	#L59,-(a7)
	jsr	_printf
	addq.w	#4,a7
L72
	bra.b	L74
L73
;  else printf("Cannot Alloate Usb Requester (Kernel not found)
	move.l	#L60,-(a7)
	jsr	_printf
	addq.w	#4,a7
L74
;  if (ReplyPort) 
	tst.l	_ReplyPort
	beq.b	L76
L75
;  if (ReplyPort) DeletePort(ReplyPort);
	move.l	_ReplyPort,-(a7)
	jsr	_DeletePort
	addq.w	#4,a7
L76
;  if (Req)
	tst.l	_Req
	beq.b	L78
L77
;  if (Req)FreeUsbRequester(Req);
	move.l	_Req,a0
	jsr	_FreeUsbRequester
L78
	moveq	#0,d0
	rts

L60
	dc.b	'Cannot Alloate Usb Requester (Kernel not found)',$A,0
L59
	dc.b	'Cannot Init Usb Requester',$A,0
L58
	dc.b	'Cannot Setup Root Hub Port',$A,0
L51
	dc.b	'Hub Port Detected',$A,0
L56
	dc.b	'Lupo Usb Port',0
L26
	dc.b	'Lupo Usb Root Hub Port',0
L52
	dc.b	'Powering ports!',$A,0
L55
	dc.b	'Processing hub',$A,0
L57
	dc.b	'Usb Kernel Killed, now Quitting',$A,0
L54
	dc.b	'Waiting hub',$A,0
L53
	dc.b	'now Looping',$A,0

	SECTION ":1",DATA

	XDEF	_ReplyPort
_ReplyPort
	ds.l	1
	XDEF	_Req
_Req
	dc.l	0

	END
