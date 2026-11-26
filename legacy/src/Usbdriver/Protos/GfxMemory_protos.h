//-------------------------------------------------------------
//
//  Gestore Gfx Memory
//
//-------------------------------------------------------------

#ifndef GFXMEMORY_PROTOS
#define GFXMEMORY_PROTOS	

#include "headers/gfxmemory.h"

//-------------------------------------------------------------------------
//	Prepara il framebuffer
//-------------------------------------------------------------------------

void SetupGfxMemory(ULONG BoardAddress,ULONG MemOffset,ULONG AvailMemory);

//-------------------------------------------------------------------------
//	Dealloca porzione di framebuffer
//-------------------------------------------------------------------------

void GfxFreeMem(char* MemAddress);

//-------------------------------------------------------------------------
//	Alloca porzione di framebuffer
//-------------------------------------------------------------------------

char *GfxAllocMem(ULONG Size,ULONG Flags);

//-------------------------------------------------------------------------
//	Dealloca il framebuffer
//-------------------------------------------------------------------------

void GfxKillList(void);

#endif