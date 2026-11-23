
; Storm C Compiler
; Compiler:Usb/LupoUsbLibrary.c
	mc68030
	mc68881
	XREF	_SysBase
	XREF	_DOSBase

	SECTION ":0",CODE


;ULONG __saveds GetUsbError(register __a0 PUSB_REQUESTER Req)
	XDEF	_GetUsbError
_GetUsbError
L73
	move.l	$22(a0),d0
	rts

;ULONG __saveds Get32BitFrameNumber(register __a0 PUSB_REQUESTER Req)
	XDEF	_Get32BitFrameNumber
_Get32BitFrameNumber
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L74
;    Req->UsbRequest.IO_Command=CMD_GET_32BIT_FN;
	move.l	a2,a0
	move.w	#$1001,$20(a0)
;    PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;    WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;    while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L75
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L75
L76
;    if(Req->UsbRequest.IO_Error==0) 
	tst.l	$22(a2)
	bne.b	L78
L77
	move.l	$26(a2),d0
	movem.l	(a7)+,a2/a6
	rts
L78
	moveq	#-1,d0
	movem.l	(a7)+,a2/a6
	rts

;PHCD_DEVICE_DATA __saveds GetUsbBaseAddress(register __a0 PUSB_REQUE
	XDEF	_GetUsbBaseAddress
_GetUsbBaseAddress
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L79
;  Req->UsbRequest.IO_Command=CMD_GET_USB_BASE;
	move.l	a2,a0
	move.w	#3,$20(a0)
;  PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;  WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;  while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L80
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L80
L81
;  if(Req->UsbRequest.IO_Error==0) 
	tst.l	$22(a2)
	bne.b	L83
L82
	move.l	$26(a2),d0
	movem.l	(a7)+,a2/a6
	rts
L83
	moveq	#0,d0
	movem.l	(a7)+,a2/a6
	rts

;void __saveds   KillHost(register __a0 PUSB_REQUESTER Req)
	XDEF	_KillHost
_KillHost
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L84
;  Req->UsbRequest.IO_Command=CMD_KILL_USB;
	move.l	a2,a0
	move.w	#$FFFF,$20(a0)
;  PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;  WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;  while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L85
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L85
L86
	movem.l	(a7)+,a2/a6
	rts

;void __saveds   StartHost(register __a0 PUSB_REQUESTER Req)
	XDEF	_StartHost
_StartHost
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L87
;  Req->UsbRequest.IO_Command=CMD_START_HOST;
	move.l	a2,a0
	move.w	#$64,$20(a0)
;  PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;  WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;  while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L88
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L88
L89
	movem.l	(a7)+,a2/a6
	rts

;void __saveds   StopHost(register __a0 PUSB_REQUESTER Req)
	XDEF	_StopHost
_StopHost
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L90
;  Req->UsbRequest.IO_Command=CMD_STOP_HOST;
	move.l	a2,a0
	move.w	#$65,$20(a0)
;  PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;  WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;  while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L91
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L91
L92
	movem.l	(a7)+,a2/a6
	rts

;void __saveds   PauseHost(register __a0 PUSB_REQUESTER Req)
	XDEF	_PauseHost
_PauseHost
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L93
;  Req->UsbRequest.IO_Command=CMD_PAUSE_HOST;
	move.l	a2,a0
	move.w	#$66,$20(a0)
;  PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;  WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;  while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L94
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L94
L95
	movem.l	(a7)+,a2/a6
	rts

;UBYTE __saveds   SetUsbState(register __a0 PUSB_REQUESTER Req,
	XDEF	_SetUsbState
_SetUsbState
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L96
;  Req->UsbRequest.IO_Command=CMD_SET_USB_STATE;
	move.l	a2,a0
	move.w	#$B,$20(a0)
;  PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;  WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;  while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L97
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L97
L98
;  if(Req->UsbRequest.IO_Error==0) 
	tst.l	$22(a2)
	bne.b	L100
L99
	moveq	#1,d0
	movem.l	(a7)+,a2/a6
	rts
L100
	moveq	#0,d0
	movem.l	(a7)+,a2/a6
	rts

;ULONG __saveds   GetUsbState(register __a0 PUSB_REQUESTER Req)
	XDEF	_GetUsbState
_GetUsbState
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L101
;  Req->UsbRequest.IO_Command=CMD_GET_USB_STATE;
	move.l	a2,a0
	move.w	#$C,$20(a0)
;  PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;  WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;  while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L102
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L102
L103
;  if(Req->UsbRequest.IO_Error==0) 
	tst.l	$22(a2)
	bne.b	L105
L104
	move.l	$26(a2),d0
	movem.l	(a7)+,a2/a6
	rts
L105
	moveq	#-1,d0
	movem.l	(a7)+,a2/a6
	rts

;char * __saveds   AllocUsbMem(register __a0 PUSB_REQUESTER Req,
	XDEF	_AllocUsbMem
_AllocUsbMem
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L106
;  Req->UsbRequest.IO_Command=CMD_ALLOCMEM;
	move.l	a2,a0
	move.w	#1,$20(a0)
;  Req->UsbRequest.IO_Data=BufferLeng;
	move.l	a2,a0
	move.l	d0,$26(a0)
;  PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;  WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;  while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L107
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L107
L108
;  if(Req->UsbRequest.IO_Error==0)
	tst.l	$22(a2)
	bne.b	L110
L109
	move.l	$26(a2),d0
	movem.l	(a7)+,a2/a6
	rts
L110
	moveq	#0,d0
	movem.l	(a7)+,a2/a6
	rts

;void __saveds   FreeUsbMem(register __a0 PUSB_REQUESTER Req,
	XDEF	_FreeUsbMem
_FreeUsbMem
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L111
;  Req->UsbRequest.IO_Command=CMD_FREEMEM;
	move.l	a2,a0
	move.w	#2,$20(a0)
;  Req->UsbRequest.IO_Data=(ULONG)Mem;
	move.l	a2,a0
	move.l	a1,$26(a0)
;  PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;  WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;  while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L112
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L112
L113
;  Mem=NULL;
	movem.l	(a7)+,a2/a6
	rts

;PUSB_REQUESTER AllocUsbRequester(void)
	XDEF	_AllocUsbRequester
_AllocUsbRequester
	move.l	a6,-(a7)
L115
; if(KernelPort==NULL) 
	move.l	_KernelPort,a0
	cmp.w	#0,a0
	bne.b	L117
L116
; if(KernelPort==NULL) KernelPort=FindPort(UsbK
	move.l	_SysBase,a6
	lea	L114(pc),a1
	jsr	-$186(a6)
	move.l	d0,_KernelPort
L117
; if(StackPort ==NULL) 
	move.l	_StackPort,a0
	cmp.w	#0,a0
	bne.b	L119
L118
; if(StackPort ==NULL) StackPort =FindPort(UsbK
	move.l	_SysBase,a6
	lea	L114(pc),a1
	jsr	-$186(a6)
	move.l	d0,_StackPort
L119
; if(KernelPort && StackPort)
	tst.l	_KernelPort
	beq.b	L122
L120
	tst.l	_StackPort
	beq.b	L122
L121
	move.l	_SysBase,a6
	moveq	#$3A,d0
	move.l	#$10001,d1
	jsr	-$C6(a6)
	move.l	(a7)+,a6
	rts
L122
	moveq	#0,d0
	move.l	(a7)+,a6
	rts

;UBYTE __saveds   InitUsbRequester(register __a0 PUSB_REQUESTER Req,r
	XDEF	_InitUsbRequester
_InitUsbRequester
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L123
; if(UserPort==NULL) 
	cmp.w	#0,a1
	bne.b	L125
L124
	moveq	#0,d0
	movem.l	(a7)+,a2/a6
	rts
L125
; if(Req==NULL) 
	move.l	a2,a0
	cmp.w	#0,a0
	bne.b	L127
L126
	moveq	#0,d0
	movem.l	(a7)+,a2/a6
	rts
L127
; Req->UserPort  =UserPort;
	move.l	a2,a0
	move.l	a1,4(a0)
; Req->UsbRequest.message.mn_ReplyPort=UserPort;
	move.l	a2,a0
	move.l	a1,$1A(a0)
; Req->UsbRequest.IO_Command=CMD_GET_USB_BASE;
	move.l	a2,a0
	move.w	#3,$20(a0)
; PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
; WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
; while(Req->DummyMess=GetMsg(Req->UserPort));
L128
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	bne.b	L128
L129
; Req->devicedata=(PHCD_DEVICE_DATA)Req->UsbRequest.IO_Data;
	move.l	a2,a1
	move.l	$26(a2),(a1)
; Req->UsbRequest.IO_Command=0;
	move.l	a2,a0
	clr.w	$20(a0)
; Req->UsbRequest.IO_Data=0;
	move.l	a2,a0
	clr.l	$26(a0)
	moveq	#1,d0
	movem.l	(a7)+,a2/a6
	rts

;void __saveds   FreeUsbRequester(register __a0 PUSB_REQUESTER Req)
	XDEF	_FreeUsbRequester
_FreeUsbRequester
	move.l	a6,-(a7)
	move.l	a0,a1
L130
; if (Req) 
	cmp.w	#0,a1
	beq.b	L132
L131
; if (Req) FreeMem(Req,sizeof(USB_REQUESTER));
	move.l	_SysBase,a6
	moveq	#$3A,d0
	jsr	-$D2(a6)
L132
	move.l	(a7)+,a6
	rts

;PHCD_ENDPOINT __saveds   AllocEndpoint(register __a0 PUSB_REQUESTER 
	XDEF	_AllocEndpoint
_AllocEndpoint
	movem.l	d2/a2/a3/a6,-(a7)
	move.l	a0,a3
L133
;  Req->UsbRequest.IO_Command=CMD_ALLOC_ENDPOINT;
	move.l	a3,a0
	move.w	#$3E8,$20(a0)
;  Req->UsbRequest.IO_Buffer[0]=type;
	move.l	d0,$2A(a3)
;  Req->UsbRequest.IO_Buffer[1]=rate;
	lea	$2A(a3),a0
	move.l	d1,4(a0)
;  Req->UsbRequest.IO_Buffer[2]=(ULONG)cont;
	lea	$2A(a3),a0
	move.l	a1,$8(a0)
;  if(type==Endpoint_Control)       
	cmp.l	#1,d0
	bne.b	L135
L134
;  if(type==Endpoint_Control)     
	lea	$2A(a3),a0
	move.l	#$3F,4(a0)
L135
;  if(type==Endpoint_Bulk)          
	cmp.l	#2,d0
	bne.b	L137
L136
;  if(type==Endpoint_Bulk)        
	lea	$2A(a3),a0
	move.l	#$40,4(a0)
L137
;  if(type==Endpoint_Isochronous)   
	cmp.l	#$8,d0
	bne.b	L139
L138
;  if(type==Endpoint_Isochronous) 
	lea	$2A(a3),a0
	clr.l	4(a0)
L139
;  PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a3),a1
	jsr	-$16E(a6)
;  WaitPort(Req->UserPort);
	move.l	a3,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;  while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L140
	move.l	a3,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a3,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L140
L141
;  if(Req->UsbRequest.IO_Error==0)
	tst.l	$22(a3)
	bne.b	L143
L142
	move.l	$26(a3),d0
	movem.l	(a7)+,d2/a2/a3/a6
	rts
L143
	moveq	#0,d0
	movem.l	(a7)+,d2/a2/a3/a6
	rts

;void __saveds   FreeEndpoint(register __a0 PUSB_REQUESTER Req,
	XDEF	_FreeEndpoint
_FreeEndpoint
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L144
;  if (Endpoint)
	cmp.w	#0,a1
	beq.b	L147
L145
;  
;    Req->UsbRequest.IO_Command=CMD_FREE_ENDPOINT;
	move.l	a2,a0
	move.w	#$3E9,$20(a0)
;    Req->UsbRequest.IO_Data=(ULONG)Endpoint;
	move.l	a2,a0
	move.l	a1,$26(a0)
;    PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;    WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;    while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L146
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L146
L147
	movem.l	(a7)+,a2/a6
	rts

;void __saveds   GetEndpointData(register __a0 PUSB_REQUESTER Req,
	XDEF	_GetEndpointData
_GetEndpointData
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L148
; if (Endpoint)
	cmp.w	#0,a1
	beq.b	L151
L149
;  
;    Req->UsbRequest.IO_Command=CMD_GET_ENDPOINT_DATA;
	move.l	a2,a0
	move.w	#$3EA,$20(a0)
;    Req->UsbRequest.IO_Data=(ULONG)Endpoint;
	move.l	a2,a0
	move.l	a1,$26(a0)
;    PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;    WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;    while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L150
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L150
L151
	movem.l	(a7)+,a2/a6
	rts

;void __saveds   SetEndpointData(register __a0 PUSB_REQUESTER Req,
	XDEF	_SetEndpointData
_SetEndpointData
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L152
; if (Endpoint)
	cmp.w	#0,a1
	beq.b	L155
L153
;  
;    Req->UsbRequest.IO_Command=CMD_SET_ENDPOINT_DATA;
	move.l	a2,a0
	move.w	#$3EB,$20(a0)
;    Req->UsbRequest.IO_Data=(ULONG)Endpoint;
	move.l	a2,a0
	move.l	a1,$26(a0)
;    PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;    WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;    while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L154
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L154
L155
	movem.l	(a7)+,a2/a6
	rts

;UBYTE __saveds   OpenPipe(register __a0 PUSB_REQUESTER Req,
	XDEF	_OpenPipe
_OpenPipe
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L156
; if(ed)
	cmp.w	#0,a1
	beq.b	L162
L157
; 
;    Req->UsbRequest.IO_Command=CMD_OPEN_PIPE;
	move.l	a2,a0
	move.w	#$4B0,$20(a0)
;    Req->UsbRequest.IO_Data=(ULONG)ed;
	move.l	a2,a0
	move.l	a1,$26(a0)
;    PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;    WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;    while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L158
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L158
L159
;    if (Req->UsbRequest.IO_Error==0) 
	tst.l	$22(a2)
	bne.b	L161
L160
	moveq	#1,d0
	movem.l	(a7)+,a2/a6
	rts
L161
	moveq	#0,d0
	movem.l	(a7)+,a2/a6
	rts
L162
	moveq	#0,d0
	movem.l	(a7)+,a2/a6
	rts

;void __saveds   ClosePipe(register __a0 PUSB_REQUESTER Req,
	XDEF	_ClosePipe
_ClosePipe
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L163
; if(ed)
	cmp.w	#0,a1
	beq.b	L166
L164
; 
;    Req->UsbRequest.IO_Command=CMD_CLOSE_PIPE;
	move.l	a2,a0
	move.w	#$4B1,$20(a0)
;    Req->UsbRequest.IO_Data=(ULONG)ed;
	move.l	a2,a0
	move.l	a1,$26(a0)
;    PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;    WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;    while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L165
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L165
L166
	movem.l	(a7)+,a2/a6
	rts

;PUSBD_REQUEST __saveds   AllocUsbdRequest(register __a0 PUSB_REQUEST
	XDEF	_AllocUsbdRequest
_AllocUsbdRequest
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L167
;  Req->UsbRequest.IO_Command=CMD_ALLOC_USBD_REQUEST;
	move.l	a2,a0
	move.w	#$44C,$20(a0)
;  Req->UsbRequest.IO_Buffer[0]=BufferLeng;
	move.l	d0,$2A(a2)
;  PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;  WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;  while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L168
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L168
L169
;  if(Req->UsbRequest.IO_Error==0) 
	tst.l	$22(a2)
	bne.b	L171
L170
	move.l	$26(a2),d0
	movem.l	(a7)+,a2/a6
	rts
L171
	moveq	#0,d0
	movem.l	(a7)+,a2/a6
	rts

;void __saveds   FreeUsbdRequest(register __a0 PUSB_REQUESTER Req,
	XDEF	_FreeUsbdRequest
_FreeUsbdRequest
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L172
;  if (usb)
	cmp.w	#0,a1
	beq.b	L175
L173
;  
;    Req->UsbRequest.IO_Command=CMD_FREE_USBD_REQUEST;
	move.l	a2,a0
	move.w	#$44D,$20(a0)
;    Req->UsbRequest.IO_Data=(ULONG)usb;
	move.l	a2,a0
	move.l	a1,$26(a0)
;    PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;    WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;    while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L174
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L174
L175
	movem.l	(a7)+,a2/a6
	rts

;void __saveds   AbortUsbdRequest(register __a0 PUSB_REQUESTER Req,
	XDEF	_AbortUsbdRequest
_AbortUsbdRequest
	movem.l	a2/a6,-(a7)
	move.l	a0,a2
L176
;  if (usb)
	cmp.w	#0,a1
	beq.b	L179
L177
;  
;    Req->UsbRequest.IO_Command=CMD_ABORT_USBD_REQUEST;
	move.l	a2,a0
	move.w	#$44E,$20(a0)
;    Req->UsbRequest.IO_Data=(ULONG)usb;
	move.l	a2,a0
	move.l	a1,$26(a0)
;    PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a2),a1
	jsr	-$16E(a6)
;    WaitPort(Req->UserPort);
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;    while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L178
	move.l	a2,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a2,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L178
L179
	movem.l	(a7)+,a2/a6
	rts

;void __saveds   FreeUsbdRequestBuffer(register __a0 PUSB_REQUESTER R
	XDEF	_FreeUsbdRequestBuffer
_FreeUsbdRequestBuffer
	move.l	a2,-(a7)
	move.l	a1,a2
L180
;  if (usb)
	cmp.w	#0,a2
	beq.b	L182
L181
;  
;  	  FreeUsbMem(Req,usb->Buffer);
	move.l	$10(a2),a1
	jsr	_FreeUsbMem
;  	  usb->BufferLength=0;
	clr.l	$14(a2)
;  	  usb->BufferTotalLength=0;
	clr.l	$18(a2)
L182
	move.l	(a7)+,a2
	rts

;ULONG __saveds   AllocUsbdRequestBuffer(register __a0 PUSB_REQUESTER
	XDEF	_AllocUsbdRequestBuffer
_AllocUsbdRequestBuffer
	movem.l	d2/a2/a3,-(a7)
	move.l	d0,d2
	move.l	a0,a3
	move.l	a1,a2
L183
;  if (usb)
	cmp.w	#0,a2
	beq.b	L187
L184
;  
;    FreeUsbMem(Req,usb->Buffer);
	move.l	a3,a0
	move.l	$10(a2),a1
	jsr	_FreeUsbMem
;    usb->BufferTotalLength=0;
	clr.l	$18(a2)
;    usb->BufferLength=0;
	clr.l	$14(a2)
;    if((usb->Buffer=AllocUsbMem(Req,BufferLeng))) 
	move.l	d2,d0
	move.l	a3,a0
	jsr	_AllocUsbMem
	move.l	d0,$10(a2)
	beq.b	L186
L185
;    if((usb->Buffe
	move.l	d2,$18(a2)
L186
	move.l	$18(a2),d0
	movem.l	(a7)+,d2/a2/a3
	rts
L187
	moveq	#0,d0
	movem.l	(a7)+,d2/a2/a3
	rts

;void __saveds   SetupUsbdRequest(register __a0 PUSBD_REQUEST usb,
	XDEF	_SetupUsbdRequest
_SetupUsbdRequest
	movem.l	d2-d5,-(a7)
L188
;  usb->Setup[0]=a;
	move.b	d0,$1C(a0)
;  usb->Setup[1]=b;
	lea	$1C(a0),a1
	move.b	d1,1(a1)
;  usb->Setup[2]=c&0xFF;
	moveq	#0,d0
	move.w	d2,d0
	and.l	#$FF,d0
	lea	$1C(a0),a1
	move.b	d0,2(a1)
;  usb->Setup[3]=c>>8;
	and.l	#$FFFF,d2
	moveq	#$8,d0
	asr.l	d0,d2
	lea	$1C(a0),a1
	move.b	d2,3(a1)
;  usb->Setup[4]=d&0xFF;
	moveq	#0,d0
	move.w	d3,d0
	and.l	#$FF,d0
	lea	$1C(a0),a1
	move.b	d0,4(a1)
;  usb->Setup[5]=d>>8;
	and.l	#$FFFF,d3
	moveq	#$8,d0
	asr.l	d0,d3
	lea	$1C(a0),a1
	move.b	d3,5(a1)
;  usb->Setup[6]=e&0xFF;
	moveq	#0,d0
	move.w	d4,d0
	and.l	#$FF,d0
	lea	$1C(a0),a1
	move.b	d0,6(a1)
;  usb->Setup[7]=e>>8;
	moveq	#0,d0
	move.w	d4,d0
	moveq	#$8,d1
	asr.l	d1,d0
	lea	$1C(a0),a1
	move.b	d0,7(a1)
;  usb->BufferLength=e;
	and.l	#$FFFF,d4
	move.l	d4,$14(a0)
	movem.l	(a7)+,d2-d5
	rts

;UBYTE __saveds   SendUsbdRequest(register __a0 PUSB_REQUESTER Req,
	XDEF	_SendUsbdRequest
_SendUsbdRequest
	movem.l	a2/a3/a6,-(a7)
	move.l	a0,a3
	move.l	a2,a0
L189
;  Req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
	move.l	a3,a2
	move.w	#$44F,$20(a2)
;  Req->UsbRequest.IO_Data=(ULONG)usb;
	move.l	a0,d0
	move.l	a3,a0
	move.l	d0,$26(a0)
;  Req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
	move.l	a1,$2A(a3)
;  PutMsg(KernelPort,(struct Message*)&Req->UsbRequest);
	move.l	_KernelPort,a0
	move.l	_SysBase,a6
	lea	$C(a3),a1
	jsr	-$16E(a6)
;  WaitPort(Req->UserPort);
	move.l	a3,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$180(a6)
;  while( (Req->DummyMess=GetMsg(Req->UserPort)) !=NULL);
L190
	move.l	a3,a1
	move.l	_SysBase,a6
	move.l	4(a1),a0
	jsr	-$174(a6)
	move.l	a3,a1
	move.l	d0,$8(a1)
	move.l	d0,a0
	cmp.w	#0,a0
	bne.b	L190
L191
;  if(Req->UsbRequest.IO_Error==0) 
	tst.l	$22(a3)
	bne.b	L193
L192
	moveq	#1,d0
	movem.l	(a7)+,a2/a3/a6
	rts
L193
	moveq	#0,d0
	movem.l	(a7)+,a2/a3/a6
	rts

L114
	dc.b	'Lupo Usb Port',0

	SECTION ":2",BSS

	XDEF	_KernelPort
_KernelPort
	ds.l	1
	XDEF	_StackPort
_StackPort
	ds.l	1

	END
