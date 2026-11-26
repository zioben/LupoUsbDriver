//-------------------------------------------------------------
//
//  Gestore Gfx Memory
//
//-------------------------------------------------------------

#include <exec/exec.h>
#include <pragma/exec_lib.h>

#include "headers/gfxmemory.h"

//------- struttura dati iniziale ------------------

typedef struct _GFX_MEMORY_LIST
{
  ULONG  GfxAddress;
  ULONG  BytesUsed;
  ULONG  NextAddress;
  struct _GFX_MEMORY_LIST *NextNode;
  struct _GFX_MEMORY_LIST *PrevNode;
  ULONG  Flags; 
}GfxMemoryList,*PGfxMemoryList;

//------- Nodo principale --------------------------

typedef struct _GFX_MEMORY_BASE
{
  ULONG  	 Board_Address;
  ULONG  	 Mem_Offset;
  ULONG		 Mem_Size;
  ULONG		 AvailMemory;
  ULONG  	 LastMemory;
  ULONG  	 LargestMemory;

  PGfxMemoryList GfxHeadMem;
}GfxMemoryBase,*PGfxMemoryBase;


 
//------- variabili globali ------------------------

GfxMemoryBase MyGfxBase=
{
	0,
	0,
	0,
	0,
	0,
	NULL
};


//------- Codice ----------------------------------


//----------------------------------------------------------------------------
//  Setup Memory address
//----------------------------------------------------------------------------

void SetupGfxMemory(ULONG BoardAddress,ULONG MemOffset,ULONG AvailMemory)
{
 
  MyGfxBase.Board_Address=BoardAddress;
  MyGfxBase.Mem_Offset=MemOffset;  
  MyGfxBase.AvailMemory=
  MyGfxBase.LargestMemory=
  MyGfxBase.LastMemory=
  MyGfxBase.Mem_Size=AvailMemory;
}




//----------------------------------------------------------------------------
//  Clear Memory
//----------------------------------------------------------------------------

void GfxMemoryClear(PGfxMemoryList list)
{
 BYTE *address;
 ULONG i;
 if(list)
  {
   address=(BYTE*)list->GfxAddress;
   for(i=0;i<list->BytesUsed;i++) address[i]=0;
  } 
}

//----------------------------------------------------------------------------
//  Crea la testa!
//----------------------------------------------------------------------------

void CreateGfxHead(PGfxMemoryList list)
{
  BYTE *appo;
  ULONG i;
  MyGfxBase.GfxHeadMem=list;
  list->GfxAddress=MyGfxBase.Board_Address+MyGfxBase.Mem_Offset;
  list->NextAddress=list->GfxAddress+list->BytesUsed;
  MyGfxBase.AvailMemory-=list->BytesUsed;
  MyGfxBase.LastMemory-=list->BytesUsed;
  // pulisci memoria se hai bisogno!
  if(list->Flags&MEMF_CLEAR)
   for (i=0,appo=(char*)list->GfxAddress;i<list->BytesUsed;i++)appo[i]=0;
}

//----------------------------------------------------------------------------
// Nuclearizza tutto il pool di memoria!
//----------------------------------------------------------------------------

void GfxKillList()
{
  PGfxMemoryList appo=MyGfxBase.GfxHeadMem;
  PGfxMemoryList next;
  if(appo)
   {
     while (appo)
      {
        next=appo->NextNode;
        FreeMem(appo,sizeof(GfxMemoryList));
        appo=next;
      }
     MyGfxBase.GfxHeadMem=NULL;
   }
  MyGfxBase.AvailMemory = 
  MyGfxBase.LastMemory  = 
  MyGfxBase.LargestMemory=
  MyGfxBase.Mem_Size=0;
}

//----------------------------------------------------------------------------
// Libera la lista dalla memoria
//----------------------------------------------------------------------------

void GfxFreeList(PGfxMemoryList list)
{
  if(list)
  {
    if(list->NextNode)    // non sono nella coda
      if (list->PrevNode) // sono in mezzo alla lista
           {
            list->NextNode->PrevNode=list->PrevNode;
            list->PrevNode->NextNode=list->NextNode;
           }
      else                // sono nella testa
           {
            list->NextNode->PrevNode=NULL;
            MyGfxBase.GfxHeadMem=list->NextNode;
           }
    else                  // sono nella coda
      if (list->PrevNode)
           {
            list->PrevNode->NextNode=NULL;
           }
      else                // ho solo la testa
           {
             MyGfxBase.GfxHeadMem=NULL;
           }
    if(!list->NextNode)MyGfxBase.LastMemory+=list->BytesUsed;
    MyGfxBase.AvailMemory+=list->BytesUsed;
    FreeMem(list,sizeof(GfxMemoryList));
    list=NULL;
  }
}

//----------------------------------------------------------------------------
// Calcola blocco libero + capiente!
//----------------------------------------------------------------------------

void GetLargestGfxBlock()
{
  ULONG BestBlock,appoblock;
  PGfxMemoryList appo=MyGfxBase.GfxHeadMem;
  if (appo)
   {
     BestBlock=appo->GfxAddress-(MyGfxBase.Board_Address+MyGfxBase.Mem_Offset);
     if (BestBlock<MyGfxBase.LastMemory) BestBlock=MyGfxBase.LastMemory;
     while(appo->NextNode)
      {
        appoblock=(appo->NextNode->GfxAddress)-(appo->NextAddress);
        appo=appo->NextNode;
        if(appoblock>BestBlock) 
                BestBlock=appoblock;
      }
     MyGfxBase.LargestMemory=BestBlock;
   }
  else 
   {
     MyGfxBase.AvailMemory=
     MyGfxBase.LastMemory=
     MyGfxBase.LargestMemory=MyGfxBase.Mem_Size;
   }
}

//----------------------------------------------------------------------------
// Ottieni un indirizzo valido di memoria grafica
//----------------------------------------------------------------------------

void GetGfxAddress(PGfxMemoryList list)
{
  PGfxMemoryList  appo=MyGfxBase.GfxHeadMem;
  ULONG StartBlock     =MyGfxBase.Board_Address+MyGfxBase.Mem_Offset;
  ULONG BlockOffset    =appo->GfxAddress-StartBlock;
  ULONG found=0;
  // Controlla il blocco iniziale
  if (BlockOffset>list->BytesUsed)
   {  // C'e' abbastanza spazio all'inizio!!!
      list->GfxAddress=StartBlock;
      list->NextAddress=StartBlock+list->BytesUsed;
      // ovviamente questa è la ns. nuova testa
      list->NextNode=MyGfxBase.GfxHeadMem;
      MyGfxBase.GfxHeadMem->PrevNode=list;
      MyGfxBase.GfxHeadMem=list;
      MyGfxBase.AvailMemory-=list->BytesUsed;
   }
  else
   {  // non c'e' memoria sufficiente all'inizio!
      while (!found)
      {
        if(!appo->NextNode)
          {  //sono alla fine della lista
             appo->NextNode=list;
             StartBlock=appo->NextAddress;
             if(list->Flags & MEMF_256) StartBlock=(StartBlock+255)&0xFFFFFF00;
             if(list->Flags & MEMF_128) StartBlock=(StartBlock+127)&0xFFFFFF80;
             if(list->Flags & MEMF_064) StartBlock=(StartBlock+63) &0xFFFFFFC0;
             if(list->Flags & MEMF_032) StartBlock=(StartBlock+31) &0xFFFFFFE0;
             if(list->Flags & MEMF_016) StartBlock=(StartBlock+15) &0xFFFFFFF0;
             list->GfxAddress=StartBlock;
             list->NextAddress=StartBlock+list->BytesUsed;
             list->NextNode=NULL;
             list->PrevNode=appo;
             found=1;
             MyGfxBase.AvailMemory-=list->BytesUsed;
             MyGfxBase.LastMemory-=list->BytesUsed;
          }
        else
          {  //trova 1 buco utile!
            BlockOffset=appo->NextNode->GfxAddress-appo->NextAddress;
            if (BlockOffset>list->BytesUsed)
              {  // trovato 1 posticino
                 StartBlock=appo->NextAddress;
                 if(list->Flags & MEMF_256) StartBlock=(StartBlock+255)&0xFFFFFF00;
	         if(list->Flags & MEMF_128) StartBlock=(StartBlock+127)&0xFFFFFF80;
                 if(list->Flags & MEMF_064) StartBlock=(StartBlock+63) &0xFFFFFFC0;
                 if(list->Flags & MEMF_032) StartBlock=(StartBlock+31) &0xFFFFFFE0;
                 if(list->Flags & MEMF_016) StartBlock=(StartBlock+15) &0xFFFFFFF0;
                 if(!(StartBlock+list->BytesUsed>appo->NextNode->GfxAddress)) 
                   {  //tutto ok! iseriamo nella lista
                      list->GfxAddress=StartBlock;
                      list->NextAddress=StartBlock+list->BytesUsed;
                      list->NextNode=appo->NextNode;
                      list->PrevNode=appo;
                      appo->NextNode->PrevNode=list;
                      appo->NextNode=list;
 		      found=1;
                      MyGfxBase.AvailMemory-=list->BytesUsed;
                   }
                 else appo=appo->NextNode;
              }
            else appo=appo->NextNode;
          }
      }
   }
}


//----------------------------------------------------------------------------
//  Libera memoria grafica
//----------------------------------------------------------------------------


void GfxFreeMem(char* Address)
{
  PGfxMemoryList appo=MyGfxBase.GfxHeadMem;
  ULONG addr=(ULONG)Address;
  ULONG found=0;
  while((appo) && (!found))
    {
      if(appo->GfxAddress==addr)
        { 
         GfxFreeList(appo);
         GetLargestGfxBlock();
         found=1;
        }
      else appo=appo->NextNode;
    }
 // if(!appo) printf("\n\n\nGfxFreeMem Warning: memory chunk not found\n\n");
}


//----------------------------------------------------------------------------
//  Alloca la memoria grafica!
//----------------------------------------------------------------------------

  
char *GfxAllocMem(ULONG bytes,ULONG flags)
{
  PGfxMemoryList list;
  ULONG mem;
     if (bytes%2)bytes++;
     // c'e' abbastanza GfxBemory?
     mem=bytes;
     if (flags & MEMF_016) mem+=16;
     if (flags & MEMF_032) mem+=32;
     if (flags & MEMF_064) mem+=64;
     if (flags & MEMF_128) mem+=128;
     if (flags & MEMF_256) mem+=256;
     if (!(mem<MyGfxBase.AvailMemory))  
              return(NULL);
     if (!(mem<MyGfxBase.LargestMemory))
              return(NULL);
     // se si alloca struttura e inizializza!
     if (!(list=AllocMem(sizeof(GfxMemoryList),MEMF_PUBLIC|MEMF_CLEAR)))
              return(NULL);
     list->Flags=flags;
     list->BytesUsed=bytes;
     // concatena in lista
     if (MyGfxBase.GfxHeadMem) 
           GetGfxAddress(list);
     else            
           CreateGfxHead(list);
     if(list->Flags&MEMF_CLEAR) GfxMemoryClear(list);
     // calcola il blocco ram + capiente
     GetLargestGfxBlock();
  return((char*)list->GfxAddress);
}


