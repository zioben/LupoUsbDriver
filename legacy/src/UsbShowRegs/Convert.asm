

	XDEF	_ConvertLong



_ConvertLong:
 ;
 ; D0 = $AABBCCDD
 ;
 ROL.W	#8,D0	;D0=$AABBDDCC
 SWAP	D0	;D0=$DDCCAABB
 ROL.W	#8,D0	;D0=$DDCCBBAA
 RTS