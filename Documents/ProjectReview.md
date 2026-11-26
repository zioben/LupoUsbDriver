# LupoUsbDriver – Revisione del progetto e proposta di riorganizzazione

*Revisione del novembre 2026, sul branch `claude/project-review`. Copre lo stato del codice al commit `9eeefc7`.*

Questo documento ha quattro obiettivi:

1. scegliere SDK e toolchain per ricostruire la build;
2. riorganizzare repository, build e CI;
3. proporre un'architettura target per lo stack USB;
4. definire una roadmap a fasi, con priorità e rischi.

Ogni sezione si chiude con una **raccomandazione** precisa. Il codice esistente non è stato modificato: le uniche aggiunte sono questo documento e alcune bozze in `Documents/proposals/`, tutte marcate come bozze.

> **Aggiornamento (revisione 2):** la Mediator non è più disponibile. Il nuovo hardware target è **l'A1200 con una scheda USB PCMCIA a 2 o 4 porte nello slot PCMCIA (Gayle)**. Ne seguono due novità:
>
> - il progetto si organizza esplicitamente **a layer** (§4.0);
> - lo strato hardware si riprogetta per lo slot PCMCIA (§2.8).
>
> L'architettura dei livelli superiori resta valida, anzi la separazione in layer è proprio ciò che rende possibile il cambio di hardware senza riscrivere stack e classi.

---

## 0. Sintesi delle raccomandazioni

| Area | Raccomandazione |
|---|---|
| Compilatore principale | **bebbo `m68k-amigaos-gcc`** (cross da Linux, in container Docker), con `-m68020` e senza FPU |
| Compilatore secondario | **vbcc + vasm + vlink**. Va tenuto compilabile in CI perché rende possibile anche la build nativa su Amiga |
| SDK | **NDK 3.2 (R4)**. Target minimo a runtime: AmigaOS 3.0/3.1 per driver e librerie, 3.2 o ReAction per i tool con GUI |
| Header delle librerie | Un unico file **`.sfd`** come sorgente di verità. Prototipi, inline, pragma e LVO si generano con `sfdc` e `fd2pragma`. Lo scheletro di library/device va scritto a mano con le macro SDI |
| Assembly | Ridurlo al minimo. Gli accessori ai registri diventano funzioni C `static inline`. Quel che resta di asm si assembla con `vasmm68k_mot -Fhunk -devpac` |
| StormC / SAS/C | Non supportarli: i file progetto StormC restano solo nello snapshot storico |
| Build | **GNU make**, con selezione della toolchain (`TOOLCHAIN=gcc` o `vbcc`) e build fuori dal sorgente in `build/` |
| Repository | Tag `legacy-2003` sullo stato attuale, poi spostamento dei sorgenti originali in `legacy/` e nuovo albero `src/` `include/` `sfd/` `tools/` `tests/`. Eliminare dall'albero `.o`, `.debug`, eseguibili e `.info` |
| Encoding | Il codice usa già fine riga **LF** (nessun CR/LF: l'affermazione iniziale era sbagliata). Solo 9 file contengono caratteri Latin-1: vanno convertiti in UTF-8 nel nuovo albero, con `.gitattributes` che impone LF |
| CI | GitHub Actions su container con toolchain fissata per digest. Tre job: gcc, vbcc e test sull'host. Il pacchetto LHA si pubblica come artifact |
| Layer | Sei strati con interfacce definite (§4.0): **L0** accesso al bus (PCMCIA/Gayle, in futuro PCI) → **L1** driver del chip host controller → **L2** interfaccia HCD standard → **L3** USBD → **L4** class driver → **L5** applicazioni e tool |
| Hardware target | **Scheda USB PCMCIA a 16 bit nello slot dell'A1200** (§2.8). **Le due schede in possesso (Hamlet HUSB2CB, Digicom 8E4172) sono CardBus e non utilizzabili**; candidata: **Ratoc REX-CFU1U (CF+, SL811HS)** con adattatore CF→PCMCIA + hub USB esterno (§2.8.2bis). Le schede **CardBus (32 bit) non funzionano** nell'A1200. Una scheda 16 bit non può fare DMA, quindi il chip è per forza un controller a I/O programmato (PIO), non OHCI. **Primo passo: identificare il chip della scheda** |
| Architettura HCD | Driver per l'host controller come **Exec device** compatibile con l'interfaccia `devices/usbhardware.h`. Il primo è il device per il chip della scheda PCMCIA (per esempio `lupoisp116x.device` o `lupo811.device`). Il codice OHCI esistente diventa un HCD storico (`lupoohci.device`, per PCI). Il root hub è emulato dentro il device come un hub USB standard |
| Interrupt | ISR minima che invia un `Signal()` al task dell'unità. Basta polling, basta `WaitTOF()`, basta interrupt SOF permanente |
| Memoria / trasferimenti | Su PCMCIA niente DMA: l'HCD copia i dati da e verso il buffer interno del chip via I/O. I client non vedono mai la memoria hardware (oggi invece vedono la VRAM della Mediator). Per l'eventuale backend PCI resta il pool DMA con bounce buffer |
| USBD | `lupousb.library` v2: enumerazione, hub driver unico per root hub ed esterni, binding dinamico dei class driver |
| Class driver | `hid.class` generico (parser dei report descriptor con fallback sul boot protocol) per mouse, tastiera e joypad. Poi `hub.class`, poi mass storage |
| Test | Test unitari sull'host Linux per i livelli portabili, più un **HCD virtuale** (mock) da usare in emulatore. L'hardware reale serve solo per l'HCD |
| Roadmap | F0 conservazione → F1 build ricostruita → **F1.5 identificazione della scheda PCMCIA e layer L0** → F2 bug fix del legacy (senza test su hardware) → F3 HCD PCMCIA come device → F4 USBD v2 → F5 HID → F6 hub esterni → F7 bulk/mass storage → F8 altri controller |
| Rischio n. 1 | **Le schede PCMCIA disponibili sono CardBus (verificato)**: serve procurare una scheda compatibile (REX-CFU1U o equivalente SL811HS). Poi: un A1200 funzionante su cui provare (l'A1200 originale è morto), documentazione del chip, quirk di Gayle |

---

## 1. Stato attuale (verificato sul codice)

### 1.1 Inventario e build originale

I file progetto StormC sono quelli con estensione `.¶`, salvati nel repository come `*.Â` per un problema di encoding dei nomi durante l'upload. Contengono la vera configurazione di build:

| Progetto StormC | Sorgenti | Output |
|---|---|---|
| `Usbdriver/UsbDriver.¶` | `Usbdriver.c`, `Hcdriver.c`, `Interrupt.c`, `Endpoint.c`, `GfxMemory.c`, `Debugger.c` + `Asmhc.o`, `AsmEndpoint.o` | `C:UsbDriver` (processo "kernel") |
| `LupoUsbLibrary.¶` | `LupoUsbLibrary.c`, `LupoUsbLibraryAsm.Asm`, `LupoUsbLibrary.fd` | `Libs:lupousb.library` |
| `UsbStack.¶` | `UsbStack.c` | `C:UsbStack` |
| `RootHub.¶` | `RootHub.c`, `RootHubClass.c` + `Asmhc.o` | `C:UsbRootHub` |
| `AddUsbFunction.¶`, `UsbMouse.¶`, `USB_Handler.¶` | omonimi | `C:…` |
| `RootHubLib.¶`, `RootHubLibrary.¶`, `DefaultControlEndpoint.¶` | esperimenti precedenti | – |
| `UsbPrefs/LupoUsbPrefs.¶`, `UsbShowRegs/USBShowRegs.¶` | tool | – |

Osservazioni:

- Il README dice di aprire `LupoUsbLibrary.s`, ma quel file è **l'output assembly del compilatore StormC** (`; Storm C Compiler`), non il progetto.
- Gli include puntano a `Compiler:Usb/…` e `StormC:include`: gli header venivano copiati nella directory include del compilatore e inclusi come `<usb/HcDriverStructs.h>`.
- I nomi negli `#include` non rispettano le maiuscole/minuscole dei file. Per esempio `HcDriverStructs.h` contro `Hcdriverstructs.h`, `protos/interrupt_protos.h` contro `Protos/Interrupt_protos.h`, `headers/gfxmemory.h` contro `Headers/GfxMemory.h`. Su un filesystem Linux la build fallisce subito.
- Il codice usa estensioni StormC/C++ che non sono C standard:
  - inizializzatori globali con chiamate di funzione, come `struct Task *ThisTask=FindTask(NULL)` e `struct Library *UsbLibrary=OpenLibrary(...)` in `UsbStack.c`, `UsbMouse.c`, `RootHub.c`, `AddUsbFunction.c`, `USBShowRegs.c`;
  - tipi `struct` usati senza la parola chiave (`_GFX_MEMORY_LIST *NextNode` in `GfxMemory.c`);
  - `register __a0`, `__saveds`, `#pragma libbase`.
- Il progetto era configurato per `mc68030` e `mc68881`. `HcSetup()` usa davvero aritmetica floating point (`FrameInterval*0.9`), che basta sostituire con `*9/10`.
- `Usbdriver.c:173` contiene `};` seguito da `else`, che in C è un **errore di sintassi**. Il sorgente nel repository probabilmente non coincide esattamente con quello che ha prodotto i binari presenti.
- `ShowRegs` usa ReAction (`window.class` v44), quindi richiede OS 3.5 o successivo.
- Gli header di `pci.library` (`libraries/pci.h`, `clib/pci_lib_protos.h`, `pragma/pci_lib.h`) **non sono nel repository**: vengono dall'SDK Elbox della Mediator.
- Licenza del repository: MIT.

### 1.2 Correzioni all'analisi precedente

| Affermazione | Esito della verifica |
|---|---|
| Sorgenti in Latin-1 con fine riga CR/LF | **Falso per il fine riga**: nessun file tracciato contiene CR, sono tutti LF (il fine riga nativo di AmigaOS). Solo 9 file contengono byte Latin-1 (accenti nei commenti): `Endpoint.c`, `EndpointOLD.c`, `Endpoint_protos.h`, `Usbdriver.c`, `GfxMemory.c`, `Hcdriverstructs.h`, `Asmhc_protos.h`, `USBShowRegs.c`, `LupoUsbLibrary.c` |
| Libreria come wrapper di messaggi | Confermato. `SendUsbRequest` (asm) fa `PutMsg` sulla porta del kernel e poi `WaitPort`/`GetMsg` sulla porta del client |
| Root hub come emulazione della classe hub | **Solo in parte.** `RootHubClass.c` espone funzioni con nomi da classe hub (`ClearPortFeature`, `GetPortStatus` …) che però **scrivono direttamente nei registri OHCI** tramite `Req->devicedata`, cioè il puntatore all'HC ottenuto dal kernel. Non c'è emulazione via richieste di controllo USB, quindi un hub esterno richiederebbe un percorso di codice completamente diverso |
| Tutti i bug noti elencati | Tutti confermati (vedi §1.3) |

### 1.3 Bug confermati e nuovi rilievi

> **Stato (fase F2, novembre 2026):** i bug #1–#19, #23, #24, #27–#34 sono corretti nel porting in `legacy/src`, uno per commit (`legacy/README.md`). #20, #21, #22 e #25 sono scelte di architettura, risolte dal nuovo stack.

Bug già noti, tutti confermati:

| # | File | Problema |
|---|---|---|
| 1 | `Usbdriver/Endpoint.c` `QueueNormalRequest` | `Set_TD_Toggle(TD,3)` per i TD non di controllo forza DATA1 dal TD invece di usare il toggle carry dell'ED (valore 0) |
| 2 | `Endpoint.c` `OpenPipe` | Il quarto ramo controlla `Endpoint_Isochronous` invece di `Endpoint_Bulk`, e manca il `return` finale |
| 3 | `Endpoint.c` `CheckBandwidth` (riga ~614) | `dd->EDList[BestList]-=…` invece di `ListBandwidth` (corrompe un puntatore a ED) |
| 4 | `Usbdriver.c` | `mymess->IO_Error=NO_ERROR;` prima del primo `case`, quindi irraggiungibile: `IO_Error` resta quello del giro precedente |
| 5 | `Hcdriver.c` `HcFindDevice` | `while((i<32)\|\|(found==0))` gira all'infinito se la scheda non c'è, e `j` (UBYTE) va in overflow |
| 6 | `Usbdriver.c` | `if(HcSetup(...)) kill();` prosegue con strutture già liberate |
| 7 | `UsbStack.c` `InitFunction` | MPS dell'EP0 calcolato come `Buffer[6]<<8 + Buffer[7]`. `bMaxPacketSize0` è solo `Buffer[7]`, mentre `Buffer[6]` è `bDeviceProtocol` |
| 8 | `UsbMouse.c` | Qualificatore con `IEQUALIFIERB_MULTIBROADCAST`, che è un numero di bit e non una maschera. I pulsanti sono combinati in OR invece di generare un evento per transizione (`IECODE_xBUTTON` con o senza `IECODE_UP_PREFIX`) |
| 9 | `UsbMouse.c` `MouseTask` | Task Exec creato con `CreateTask` che chiama `Delay()` di DOS (vietato fuori da un processo). La reply port ha `mp_SigTask` = task principale. Lo stesso evento viene reiniettato a ogni tick anche senza dati nuovi |
| 10 | `LupoUsbLibrary.c` | `SetUsbState` non copia `usbstate` in `IO_Data`. `GetUsbBaseAddress` tratta come errore il valore di ritorno di successo (`==0`) |
| 11 | `Usbdriver.c` | Main loop in polling con `WaitTOF()`. Il `Wait()` sul segnale è commentato |

Nuovi rilievi:

| # | File | Problema | Gravità |
|---|---|---|---|
| 12 | `Usbdriver.c` + `Interrupt.c` | Un unico `ROOT_HUB_MESSAGE` statico viene rimesso con `PutMsg` a ogni cambio di stato, **anche se è ancora in coda** sulla porta del root hub (corrompe la lista della MsgPort). In più l'ISR lo modifica mentre il client lo legge | Alta |
| 13 | `Interrupt.c` | L'ISR restituisce sempre 1 e quindi rivendica l'interrupt anche quando la catena condivisa della Mediator non riguarda la scheda USB | Alta |
| 14 | `Interrupt.c` | `HccaDoneHead` non viene mascherato: il bit 0 (flag "altri interrupt pendenti") può finire nel puntatore al TD | Media |
| 15 | `Hcdriver.c` `HcStart` | Interrupt SOF abilitato in permanenza, cioè 1000 interrupt al secondo solo per incrementare un contatore | Media (prestazioni) |
| 16 | `Usbdriver.c` | `RemoveMediatorPciInterrupt()` chiamata senza argomento, mentre la funzione si aspetta `PHCD_DEVICE_DATA` | Alta |
| 17 | `LupoUsbLibrary.c` | `Get32BitFrameNumber` restituisce l'esito di `SendUsbRequest` invece di `IO_Data`. `FreeRootHubMessagePort` non ha `return` sul ramo di errore | Media |
| 18 | `Interrupt.c` / `Usbdriver.c` | Le variabili condivise con l'ISR (`AvvisaCambioStatoRootHub`, `DoneQueue*Head/Locked`) non sono `volatile`. Le variabili di lavoro dell'ISR sono globali | Media |
| 19 | vari | Allocazioni del tipo `while(!(p=AllocMem(...)));` vanno in loop infinito se la memoria finisce | Media |
| 20 | `RootHub.c` | Il processo root hub scrive i registri OHCI senza passare dal kernel, in concorrenza con l'ISR | Media |
| 21 | `Endpoint.c` | Nessuna gestione della cache (`CachePreDMA`/`CachePostDMA`). Funziona solo se la finestra di memoria della scheda grafica non è in cache. Su 040/060 con MMU è un rischio reale | Media |
| 22 | `AsmEndpoint.asm` | Indirizzo CPU usato come indirizzo di bus PCI (si fa solo lo swap dei byte). Presuppone una mappatura identica, vera sulla configurazione originale ma non garantita su altre busboard | Media |
| 23 | `UsbStack.c` | Nessun ritardo di recupero dopo `SET_ADDRESS` (2 ms da specifica). Enumerazione sincrona dentro il loop dei messaggi dello stack | Bassa |
| 24 | `UsbMouse.c` | Indirizzo della funzione cablato (`FUNCTIONADDRESS 2`). L'endpoint 1 e l'MPS 8 sono presunti invece di essere letti dal configuration descriptor | Bassa |
| 25 | prefs | `ENV:mediator/lupousb.prefs` è il dump binario di una struct (`ENV_DATA`). VendorID/DeviceID e la finestra di memoria della scheda grafica si impostano a mano | Bassa |
| 26 | `LupoUsbLibrary.fd` | `SetInterfaceFeature` dichiara 4 registri per 5 argomenti (manca `d1`): chiamata tramite la libreria, la funzione riceve un valore casuale. Emerso col porting a gcc e già corretto in `legacy/` | Media |
| 27 | `Usbdriver.c` | `IO_Command` è `UWORD` e `CMD_KILL_USB` vale `-1`: `case CMD_KILL_USB` non corrisponde mai, quindi il kernel non si chiude con `KillHost` | Media |
| 28 | `RootHubClass.c` | `RHub` non è definito in nessun sorgente compilato (solo in `RootHubConstructors.h`, non più incluso): i sorgenti attuali non si linkano senza un'aggiunta | Bassa |

### 1.4 Cosa c'è di buono

Va detto chiaramente: per un primo progetto del 2001-2003, scritto senza stack di riferimento open source, il lavoro è notevole:

- l'albero degli ED interrupt da 32 a 1 ms costruito correttamente;
- la done queue con inversione della lista;
- la gestione delle condizioni di errore e degli ED in halt;
- l'enumerazione funzionante con dispositivi reali;
- un allocatore DMA dedicato, necessario sulla Mediator.

La conoscenza di OHCI che c'è in `Endpoint.c` e `Hcdriver.c` è il patrimonio da salvare. L'infrastruttura intorno (processi, messaggi, iniezione input, build) va invece ripensata.

---

## 2. SDK e toolchain

### 2.1 Compilatori

| Opzione | Pro | Contro | Verdetto |
|---|---|---|---|
| **bebbo `m68k-amigaos-gcc`** (amiga-gcc, GCC moderno con binutils hunk e libnix) | C99/C11, ottimizzatore e diagnostica moderni (`-Wall -Wextra`, analizzatore statico), inline asm, `__builtin_bswap32`. Include `sfdc`, `fd2sfd`, `fd2pragma`, vasm e vlink. Immagini Docker pronte o facili da costruire. Molto usato nella scena 68k attuale | Solo cross-compilazione: un GCC nativo su 68k non è realistico. L'integrazione con libnix per librerie e device va evitata, scrivendo a mano lo scheletro | **Principale** |
| **vbcc + vasm + vlink** (Volker Barthelmann / Frank Wille) | Gira anche **nativamente** su Amiga. Leggero, sintassi `__reg("a0")`, ottimo supporto ai target Amiga e target NDK 3.2 pronto. vasm e vlink sono dello stesso ecosistema | Ottimizzatore meno aggressivo, diagnostica più povera, licenza non pienamente libera (gratuita per uso non commerciale) | **Secondario**, mantenuto compilabile in CI |
| StormC 3/4 | Era l'ambiente originale | Chiuso, abandonware, solo nativo, nessuna CI possibile | No (solo storico) |
| SAS/C 6.58 | Storico standard de facto | Chiuso, fermo dal 1996, solo nativo | No |
| GCC 2.95 (GeekGadgets) / ADE | – | Obsoleto | No |

**Raccomandazione:** bebbo gcc come toolchain principale e vbcc come secondaria. Tenerle entrambe in CI costa poco, impedisce di dipendere da estensioni di un solo compilatore (lo stesso problema che oggi blocca StormC) e permette la build nativa con vbcc su un Amiga reale o emulato. Va fissata una versione precisa di ciascuna toolchain, cioè l'immagine Docker per digest.

Flag consigliati:

- `-m68020 -msoft-float` (nessuna FPU richiesta);
- `-Os` o `-O2`;
- `-fomit-frame-pointer -Wall -Wextra -Werror=implicit-function-declaration`;
- `-noixemul` o libnix per i tool, `-nostartfiles -nostdlib` per library e device.

Un 68020 è il minimo realistico: è la CPU dell'A1200, che è anche il nuovo target. Un acceleratore aiuta, perché con il PIO su PCMCIA la CPU copia ogni byte.

### 2.2 NDK

| NDK | Note |
|---|---|
| **NDK 3.2 (R4)** | Mantenuto da Hyperion e scaricabile gratuitamente. Include file `.sfd`, header aggiornati, ReAction e autodoc. Compilare con l'NDK 3.2 **non** obbliga a richiedere OS 3.2 a runtime, purché non si usino funzioni più recenti della versione minima dichiarata |
| NDK 3.9 | Più vecchio, privo di correzioni successive. Nessun vantaggio rispetto al 3.2 |
| NDK 3.1 | Basterebbe per i driver, ma gli header sono meno completi |

**Raccomandazione: NDK 3.2.** Il minimo a runtime va dichiarato e controllato all'apertura delle librerie: Exec V39 (OS 3.0) per device e librerie dello stack, V44 (ReAction) o 3.2 per i tool con GUI. In alternativa i tool possono usare GadTools per girare anche su 3.0/3.1.

**Dipendenze esterne non coperte dall'NDK:**

- **`pci.library` (Elbox/Mediator)**: servono gli header e soprattutto il file `.fd` con gli offset LVO. Non sono nel repository e la licenza di ridistribuzione non è chiara. Proposta: una directory `third_party/` non versionata, popolata da uno script che scarica i file da una fonte ufficiale (sito Elbox o Aminet). In alternativa si scrive un header minimale con le sole 8 funzioni usate (`FindPciDevFunc`, `FindPciDevice`, `ReadConfigurationWord`, `WriteConfigurationWord`, `AddInterrupt`, `RemInterrupt`, `SetInterrupt`, `ClearInterrupt`), ma solo dopo aver verificato che gli offset siano documentati pubblicamente.
- **OpenPCI** (`openpci.library`, su Aminet): API PCI comune a più busboard. È il candidato per un secondo backend (vedi §4.4). Prima di adottarla vanno verificate la copertura dei modelli e la licenza degli header.
- **SDI headers** (`SDI_compiler.h`, `SDI_lib.h`): macro per i parametri a registro portabili tra gcc, vbcc e SAS/C. Piccole, libere e diffuse: vanno copiate in `include/sdi/`.

### 2.3 Generazione di una shared library o di un device

Lo stato attuale è un `.fd` scritto a mano, i protos con `register __a0` e la magia di StormC (`#pragma libbase`, init automatico della libreria). Nessuno di questi pezzi è portabile.

Flusso proposto:

```
sfd/lupousb_lib.sfd          ← unica sorgente di verità (convertita dal .fd con fd2sfd)
   │ sfdc
   ├── include/proto/lupousb.h
   ├── include/clib/lupousb_protos.h
   ├── include/inline/lupousb.h        (gcc)
   ├── include/lupousb/lupousb_lib.i   (LVO per l'assembly)
   └── (fd2pragma) include/pragmas/lupousb_pragmas.h, inline vbcc
```

- Lo **scheletro della libreria si scrive a mano in C**: una struttura `Resident` (romtag), una `InitTable` con la tabella delle funzioni, e le funzioni `LibInit`, `LibOpen`, `LibClose`, `LibExpunge` e `LibNull`. Per i parametri a registro si usano le macro SDI (`LIBFUNC`, `REG(a0, …)`). Lo stesso sorgente si compila con gcc e con vbcc, senza dipendere dall'auto-init di libnix o di StormC.
- Va compilato senza startup code e senza dati globali inizializzati da costruttori. Tutto lo stato sta nella `LibBase` estesa (`struct LupoUsbBase`), e A6 viene sempre passato esplicitamente. Questo elimina `__saveds` e il modello small-data.
- Un **device** (per esempio `lupoisp116x.device`) ha la stessa struttura, con in più `BeginIO` e `AbortIO`. Il suo `.sfd` contiene solo le funzioni standard.
- Alla libreria va assegnato un numero di **versione** (`$VER:` e `lib_Version`/`lib_Revision`).

### 2.4 Assembly 68k

- Ad oggi l'assembly comprende 1319 righe in `Asmhc.asm` e 627 in `AsmEndpoint.asm`, quasi tutte accessori del tipo leggi registro → swap → maschera → shift. **In C diventano funzioni `static inline` di poche righe**, che il compilatore espande senza costo di chiamata. Esempi: `ohci_readl(hc, OHCI_HCCONTROL)`, `ed_set_mps(ed, mps)`.
- Lo swap si fa con `__builtin_bswap32` su gcc, che genera `ror.w/swap/ror.w`. Su vbcc si usa una funzione inline asm con la stessa sequenza. Il tutto sta in un solo header, `src/common/endian.h`.
- Se qualcosa resta in assembly (per esempio una ISR ottimizzata a mano, che di solito non serve), si assembla con `vasmm68k_mot -Fhunk -devpac -m68020 -I<ndk>/include_i`. Il flag `-devpac` accetta la sintassi Devpac/Asm-One. Vanno tolti `INCDIR "Stormc:…"` e gli `INCLUDE "LVO.i"` locali a favore degli `_lib.i` generati.
- `SendUsbRequest` (asm) va eliminata: è una normale `DoIO`.

### 2.5 Build nativa o cross

- **Cross da Linux** con gcc in container: è la modalità di sviluppo e di CI.
- **Nativa** con vbcc e GNU make (disponibile per AmigaOS su Aminet): resta possibile grazie al job vbcc in CI, che impedisce di introdurre costrutti incompatibili.
- **StormC e SAS/C**: nessuna compatibilità attiva. I file `.¶` restano nello snapshot `legacy/`.

### 2.6 Altri host controller e altre busboard

| Componente | Situazione | Priorità |
|---|---|---|
| **Chip PIO su scheda PCMCIA a 16 bit (A1200)** | **Nuovo target** (§2.8) | 1 |
| OHCI su Mediator | Era l'oggetto del progetto originale; Mediator non più disponibile | legacy |
| OHCI su altre busboard PCI (Prometheus, G-REX, Mediator con altre versioni della `pci.library`) | Richiede l'astrazione della busboard (§4.4). Il nodo cruciale è **dove può fare DMA la scheda**: con la Mediator il busmaster PCI non raggiunge la RAM Amiga, da cui l'uso della VRAM. Va verificato caso per caso | 2 |
| UHCI (schede VIA) | Schedulazione molto diversa (frame list + QH/TD, tutto via software). È un nuovo HCD dietro la stessa interfaccia | 3 |
| EHCI (USB 2.0, es. NEC µPD720101 con companion OHCI) | Più complesso (QH/qTD, split transaction, TT). Ha senso solo dopo che OHCI e lo stack sono stabili | 4 |
| Controller non PCI (ISP1160/ISP1760/SL811 su clockport o Zorro) | HCD dedicati. Fuori dagli obiettivi | – |

**Contesto da tenere presente:** l'ecosistema Amiga ha già stack USB maturi, come Poseidon con il suo `pciusb.device` (OHCI/UHCI/EHCI su busboard PCI) e altri. Il valore di LupoUsbDriver è quindi soprattutto **didattico e di conservazione**: completare e capire uno stack scritto da zero. Proprio per questo conviene adottare un'interfaccia HCD compatibile, che dà due vantaggi concreti:

- il proprio HCD si può testare con uno stack collaudato;
- il proprio stack si può testare con un HCD collaudato.

### 2.7 Emulatori e strategia di test

| Strumento | Utilità | Limiti |
|---|---|---|
| **WinUAE** | Il più completo. Emula diverse busboard PCI (fra cui Mediator e Prometheus) e alcune schede PCI | A quanto mi risulta **non emula un controller USB OHCI/UHCI** collegabile: va verificato sulla versione corrente. Non sostituisce l'hardware per l'HCD |
| FS-UAE / Amiberry | Comodi su Linux, Mac e ARM, adatti alla regressione dei livelli alti | Supporto alle bridge PCI assente o limitato |
| QEMU | Emula OHCI | Nessuna macchina Amiga emulata: utile al massimo come riferimento per il comportamento di OHCI |
| Hardware reale | Indispensabile per l'HCD | Va procurato |

**Strategia raccomandata, su tre livelli:**

1. **Test unitari sull'host** (Linux, gcc nativo, in CI). Riguardano tutto ciò che è C portabile: parser di descrittori, parser dei report HID, traduzione HID→rawkey, logica di enumerazione e della macchina a stati dell'hub, schedulatore periodico/banda dell'HCD con registri finti in memoria.
2. **HCD virtuale** (`lupovirt.device`): un device con la stessa interfaccia dell'HCD che simula in software un root hub con un mouse o una tastiera HID. Permette di provare in emulatore (anche FS-UAE) caricamento librerie, enumerazione, binding delle classi e iniezione in `input.device`, senza hardware.
3. **Hardware reale** solo per l'HCD (oggi quello della scheda PCMCIA), con un tool di diagnostica (l'evoluzione di `UsbShowRegs`) e log su seriale.

Per il nuovo hardware vale lo stesso schema. WinUAE emula lo slot PCMCIA/Gayle dell'A1200 con alcuni tipi di scheda (memoria SRAM, ATA/CompactFlash e, se ricordo bene, una scheda di rete NE2000), ma **nessuna scheda USB**. L'emulatore è quindi utile per provare il layer L0 (gestione inserimento ed estrazione, lettura del CIS, finestre di memoria e I/O) con una scheda qualsiasi, non il driver del chip USB.

### 2.8 Nuovo hardware: scheda USB PCMCIA sull'A1200

#### 2.8.1 Vincoli dello slot PCMCIA dell'A1200

Questi vincoli vanno verificati sulla documentazione di Gayle e sui driver PCMCIA esistenti prima di scrivere codice, ma determinano già le scelte di progetto.

| Vincolo | Conseguenza |
|---|---|
| Lo slot è **PC Card a 16 bit** (PCMCIA 2.0, controller Gayle) | Le schede **CardBus a 32 bit** (riconoscibili dalla striscia dorata di massa sul connettore, a 3,3 V) **non funzionano** e sono anche chiavettate per non entrare. **La maggior parte delle schede USB PCMCIA in commercio, soprattutto USB 2.0, è CardBus** |
| **Niente DMA né bus master** su PC Card a 16 bit | Un controller **OHCI** (che legge da solo ED e TD in memoria) **non può funzionare** su questo slot. Una scheda USB a 16 bit usa per forza un controller **a I/O programmato** con buffer interno, dove la CPU copia i dati nel chip e il chip esegue il trasferimento |
| Spazio di memoria comune e di attributo in finestre fisse dello spazio di indirizzamento dell'A1200 (indicativamente da `$600000`), I/O in un'area separata (indicativamente da `$A20000`) | Accessi da fare con le convenzioni corrette di Gayle (accessi a 8 e 16 bit, ordine dei byte, eventuali quirk sugli indirizzi pari e dispari). Da verificare |
| La memoria Fast oltre i 4 MB sulla scheda trapdoor si sovrappone all'area PCMCIA | Configurazioni con 8 MB nella zona 24 bit rendono lo slot inutilizzabile. È un requisito da documentare per gli utenti |
| Interrupt della scheda instradato da Gayle verso un interrupt di livello basso condiviso | Serve un server di interrupt che controlli la sorgente e **restituisca 0 se l'interrupt non è suo** (lo stesso principio del bug #13) |
| Gestione dello slot tramite **`card.resource`** (`OwnCard`, `ReleaseCard`, `CardResetCard`, `CopyTuple`/`ReadCardTuple`, `GetCardMap`, `CardInterface`, `CardAccessSpeed`, …) | L0 deve prendere possesso dello slot in modo cooperativo (per esempio con `carddisk.device`), leggere il **CIS** per riconoscere la scheda e gestire l'**estrazione a caldo** in modo sicuro |
| Alcune schede richiedono il noto "PCMCIA reset fix" hardware sull'A1200 | Requisito da documentare per gli utenti |
| Banda: I/O a 16 bit attraverso Gayle, nell'ordine di poche centinaia di KB/s – 1–2 MB/s secondo CPU e modalità di accesso | Sufficiente per HID, hub e audio a bassa banda. **Il mass storage sarà lento** anche con un chip USB 2.0 |

#### 2.8.2 Quali chip si possono incontrare

| Famiglia di chip | Tipo | Note per il progetto |
|---|---|---|
| **Philips/NXP ISP1160/ISP1161** | Host USB 1.1 a I/O programmato con buffer interno (ATL e INTL organizzati in PTD). L'ISP1161 ha anche un lato device. 2 porte downstream integrate | Il blocco registri dell'host è **compatibile con OHCI** (HcControl, HcInterruptStatus, HcRhDescriptorA, HcRhPortStatus …), ma vi si accede **indirettamente** tramite una porta comando e una porta dati. Una parte significativa della conoscenza OHCI del progetto (root hub, stati, interrupt) si **riutilizza**. È il caso più favorevole |
| **ScanLogic/Cypress SL811HS** | Host USB 1.1 a I/O programmato molto semplice. 1 porta, con 2/4 porte ottenute tramite un chip hub sulla scheda | Ogni transazione è gestita a mano dalla CPU (un pacchetto alla volta, con interrupt a fine pacchetto). Driver semplice ma carico di CPU più alto. Lo scheduling (frame, NAK, toggle) è tutto software |
| **NXP ISP1760/ISP1761** | Host USB 2.0 a I/O programmato con buffer interno (PTD di tipo EHCI) | Più complesso. Utile solo se la scheda lo monta, perché la banda resta comunque limitata da Gayle |
| Chip OHCI/EHCI PCI (NEC µPD720101, VIA VT6212, …) | Bus master | Si trovano **solo su schede CardBus**: **non utilizzabili** sull'A1200 |

**Azione immediata (fase F1.5):** identificare la scheda in possesso:

1. guardare la sigla dei chip ed eventuale foto;
2. controllare se è CardBus (striscia dorata, chiavetta);
3. con la scheda inserita, fare il dump del CIS (manfid, card ID, stringhe produttore e prodotto, configurazioni I/O) con un piccolo tool basato su `card.resource`. È anche il primo programma del nuovo layer L0.

Se la scheda risulta CardBus, va procurata una scheda PC Card a 16 bit con chip ISP116x o SL811HS. In questo caso conviene preferire un ISP1161, per il riuso del modello OHCI.

#### 2.8.2bis Schede in possesso: esito della verifica (novembre 2026)

| Scheda | Indizi | Esito |
|---|---|---|
| **Hamlet HUSB2CB** ("CardBus to USB 2.0 PC Card", P/N CBU2N-N-HA) | "CardBus" scritto sull'etichetta, USB 2.0, striscia di massa dorata vicino al connettore | **CardBus: non utilizzabile sull'A1200** |
| **Digicom 8E4172** ("PcCard USB 2.0", 4 porte, 480 Mbit/s) | USB 2.0 a 480 Mbit/s richiede un controller EHCI bus master, possibile solo su CardBus; striscia di massa dorata vicino al connettore | **Quasi certamente CardBus: non utilizzabile sull'A1200** |

Entrambe montano quasi certamente un controller PCI OHCI/UHCI + EHCI (tipicamente NEC µPD72010x o VIA VT62xx). Lo si può verificare con `lspci` su un portatile Linux con slot CardBus. **Sull'A1200 non si possono usare**: lo slot accetta solo PC Card a 16 bit, a 5 V, di tipo I o II. Restano utili solo come riferimento su PC, oppure in futuro dietro un bridge PCI→CardBus su una busboard PCI, il che riporterebbe al codice OHCI legacy.

**Alternativa concreta trovata:** la **Ratoc REX-CFU1U**, una scheda **CompactFlash (CF+)** con host controller **SL811HS** (USB 1.1, full e low speed, una porta). Ci sono indizi pubblici del suo uso come scheda PCMCIA: Linux ha il driver `sl811_cs` ("RATOC USB HOST CF+ Card") e NetBSD ha `slhci_pcmcia`. Una scheda CF si inserisce nello slot dell'A1200 con un **adattatore passivo CF→PCMCIA** (lo stesso usato per le CF come disco). Restano da verificare:

- che la scheda accetti i 5 V (molte CF sono a doppia tensione);
- la configurazione I/O nel CIS;
- la reperibilità della scheda (usato).

Per avere 2 o 4 porte basta un **hub USB esterno** (anche alimentato) collegato all'unica porta: è comunque necessario per F6.

**Raccomandazione aggiornata:**

1. Primo driver L1: **`sl811`**, sulla REX-CFU1U con adattatore CF→PCMCIA.
2. Gli hub esterni diventano necessari già in F3–F4 per le porte multiple, e il bus è solo USB 1.1, adeguato a HID e hub.
3. I driver Linux e NetBSD sono **riferimento concettuale**: GPL e BSD. Non va copiato codice GPL; per il BSD andrebbe comunque rispettata la licenza e la scelta clean-room resta la più semplice.
4. Le alternative interne all'A1200, se il PCMCIA non dovesse funzionare, sono schede USB su **clockport** (per esempio RapidRoad), con un diverso L0 e un diverso L1.

Ricerca dell'esistenza di vere PC Card USB a 16 bit (non CF): **nessuna confermata**. Si trovano discussioni sulla fattibilità ma nessun prodotto verificato.

#### 2.8.3 Cosa si riutilizza del codice esistente

| Codice | Riuso con il nuovo hardware |
|---|---|
| `Endpoint.c` (ED/TD in memoria, done queue, albero interrupt) | **Non riutilizzabile direttamente** (niente DMA). La logica di scheduling periodico (intervalli, banda per frame) e di gestione degli errori si porta nel nuovo HCD come algoritmo |
| `Hcdriver.c`, `Asmhc.asm` (registri OHCI) | **Riutilizzabile concettualmente con ISP116x**, che ha registri OHCI-compatibili. Cambia l'accesso (indiretto via porte I/O PCMCIA, non mappato in memoria). La separazione tra L0 e L1 lo rende facile: si cambia solo `hc_readl`/`hc_writel` |
| `RootHub.c`/`RootHubClass.c` | La logica delle porte (connessione, reset, change bit) si riusa nell'emulazione del root hub dentro l'HCD |
| `GfxMemory.c` | Non serve più su PCMCIA. Può servire come allocatore del **buffer interno del chip** (ATL/INTL), che va gestito anch'esso a blocchi |
| `LupoUsbLibrary.c` (richieste Chapter 9), `UsbStack.c` (enumerazione) | Riutilizzabili nei layer L3 e L4, indipendenti dall'hardware |
| `UsbMouse.c` | Riutilizzabile come base di `hid.class` (dopo le correzioni) |

**Raccomandazione:** il legacy OHCI/Mediator si porta e si corregge **solo per compilare** (F1–F2), come riferimento e come possibile backend PCI futuro, **senza test su hardware**. Lo sviluppo attivo dell'HCD riparte sul chip della scheda PCMCIA, dietro la stessa interfaccia L2.

---

## 3. Organizzazione del repository e build

### 3.1 Conservazione dello stato storico

1. Creare il tag annotato **`legacy-2003`** sul commit attuale, prima di qualunque modifica.
2. Pubblicare i binari originali (`UsbDriver`, `UsbStack`, `lupousb.library`, `RootHub`, …) come **asset di una GitHub Release** "legacy-2003", invece di tenerli nell'albero.
3. Con `git mv Sources legacy/Sources` lo storico dei file resta seguibile con `git log --follow`. Lo snapshot legacy si lascia byte per byte com'è (Latin-1 incluso), così resta un riferimento fedele.

### 3.2 Struttura proposta

```
LupoUsbDriver/
├── README.md               (build aggiornata, stato, hardware supportato)
├── LICENSE                 (MIT)
├── CHANGELOG.md
├── Makefile                (entry point; include mk/*.mk)
├── mk/
│   ├── toolchain-gcc.mk
│   ├── toolchain-vbcc.mk
│   └── rules.mk
├── sfd/
│   └── lupousb_lib.sfd
├── include/                (header pubblici installati in SDK/)
│   ├── lupousb/            (usb.h: chapter 9, lupousb.h: API USBD, hid.h)
│   ├── devices/usbhardware.h   (vedi §4.2: header di interfaccia, non codice)
│   └── sdi/                (SDI_compiler.h, SDI_lib.h)
├── src/
│   ├── common/             (endian.h, debug.c/h, lists, util)
│   ├── bus/                (L0: pcmcia_gayle.c [primo], pci_mediator.c, pci_openpci.c)
│   ├── hcd/
│   │   ├── devskel/        (L2: romtag, BeginIO/AbortIO, unit task: comune a tutti gli HCD)
│   │   ├── isp116x/        (L1: chip della scheda PCMCIA, se ISP1160/1161)
│   │   ├── sl811/          (L1: alternativa, se SL811HS)
│   │   ├── ohci/           (L1 legacy: ohci_hw.h, ohci.c, ohci_sched.c, ohci_isr.c, ohci_roothub.c)
│   │   └── virt/           (L1 virtuale per i test)
│   ├── usbd/               (lupousb.library: core, enum, hub driver, binding)
│   └── classes/
│       ├── hid/            (hid.class: parser report, mouse, tastiera, joypad)
│       ├── hub/
│       └── msd/            (fase successiva)
├── tools/
│   ├── prefs/              (da UsbPrefs)
│   ├── showregs/           (da UsbShowRegs)
│   ├── cisdump/            (dump del CIS della scheda PCMCIA: primo tool di L0)
│   └── usbinfo/            (elenco dispositivi/descrittori, sostituisce le printf dello stack)
├── tests/host/             (test unitari sull'host Linux)
├── dist/                   (script Installer, icone .info *sorgenti* per il pacchetto)
├── third_party/            (non versionato: pci.library SDK ecc., popolato da script)
├── legacy/                 (snapshot originale 2001-2003, intoccato)
├── Documents/
└── .github/workflows/build.yml
```

### 3.3 Encoding e fine riga

- Come detto, i file sono già **LF**, che è anche il fine riga nativo di AmigaOS: nulla da cambiare.
- I 9 file con caratteri Latin-1 si convertono in **UTF-8** al momento del porting nel nuovo albero (`iconv -f latin1 -t utf-8`). Gli accenti compaiono solo nei commenti, quindi i compilatori non hanno problemi. Su un editor Amiga gli accenti UTF-8 appaiono come due caratteri, un difetto puramente estetico.
- Raccomandazione aggiuntiva: scrivere **i commenti del nuovo codice in inglese**, per aprire il progetto ad altri sviluppatori Amiga. Quelli esistenti si traducono man mano che i file vengono rifattorizzati. Documentazione e note di progetto possono restare in italiano.
- Un `.gitattributes` impone `eol=lf` e marca come binari `.info`, `.o`, `.lha` ecc. (bozza: `Documents/proposals/gitattributes.draft`).

### 3.4 `.gitignore` e file da eliminare

Bozza in `Documents/proposals/gitignore.draft`. Ignora `build/`, `*.o`, `*.debug`, `*.lnk`, `*.map`, `*.link`, gli eseguibili, `third_party/` e `_UAEFSDB.___`.

Le icone `.info` di Workbench dei **sorgenti** non servono: vanno eliminate. Le icone dei **prodotti** (tool, cassetto, Installer) vanno tenute come asset in `dist/` e marcate come binarie.

File morti o duplicati da non portare nel nuovo albero (restano solo in `legacy/`):

| File | Motivo |
|---|---|
| `DefaultControlEndpoint.c/.fd/.¶` | Vecchia API (class "DefaultControlEndpoint"). `DefaultControlEndpoint.h` contiene però le struct dei descrittori USB, che vanno portate in `include/lupousb/usb.h` |
| `RootHubLib.c`, `RootHubLib.¶`, `RootHubLibrary.¶`, `RootHubClass.fd` (vuoto), `RootHubConstructors.h` | Esperimenti di root hub come libreria |
| `UsbDevice.C` | Non usato da alcun progetto |
| `Usbdriver/EndpointOLD.c` | Versione precedente di `Endpoint.c` |
| `UsbStackCommands.c` | Vuoto |
| `USB_Handler.c` | Stub di 25 righe |
| `Sources/CardList.h` | Duplicato di `Usbdriver/Protos/CardList.h` |
| `Usbdriver/Usb.c.info` | Icona senza sorgente |
| `*.s` (`LupoUsbLibrary.s`, `RootHub.s`, `RootHubClass.s`, `UsbUtilities.s`) | Output del compilatore |
| `*.link`, `*.map`, `objects/` | Artefatti di build |
| `Usbdriver/objects/List.o` | Oggetto senza sorgente: va verificato se conteneva codice perduto |

### 3.5 Makefile o CMake

**Raccomandazione: GNU make.** Le ragioni:

- funziona identico in cross e nativamente su Amiga (GNU make esiste per AmigaOS, CMake no);
- gestisce bene due toolchain eterogenee: vbcc ha un driver (`vc`) con opzioni diverse da gcc, e con CMake servirebbero toolchain file e moduli custom;
- il progetto è piccolo (una decina di target).

La struttura è nella bozza `Documents/proposals/Makefile.draft`:

- `make TOOLCHAIN=gcc` (default) oppure `make TOOLCHAIN=vbcc`;
- oggetti in `build/$(TOOLCHAIN)/…`, cioè fuori dal sorgente;
- target `all`, `headers` (sfdc), `test` (test unitari sull'host), `dist` (archivio LHA con `Libs/`, `Devs/`, `Classes/USB/`, `C/`, `Prefs/`) e `clean`.

### 3.6 CI con GitHub Actions

Bozza in `Documents/proposals/build.yml.draft`. Va tenuta fuori da `.github/workflows/` finché non si decide di attivarla.

- **Immagine:** un'immagine Docker con bebbo gcc, NDK 3.2, vbcc, vasm, vlink e lha, **fissata per digest**. Si può partire da immagini pubbliche note della comunità (per esempio `amigadev/crosstools:m68k-amigaos`), verificandone versione e manutenzione. La scelta più solida a lungo termine è costruire la propria immagine (`docker/Dockerfile` nel repository), pubblicarla su GHCR e ricostruirla solo quando si aggiorna la toolchain.
- **Job:**
  1. `build-gcc`: `make TOOLCHAIN=gcc all dist`;
  2. `build-vbcc`: `make TOOLCHAIN=vbcc all`;
  3. `host-tests`: `make test` con gcc nativo su `ubuntu-latest`.
- **Artifact:** l'archivio LHA prodotto dal job gcc.
- **Release:** su tag `v*`, il pacchetto si allega alla release.
- **Dipendenze non ridistribuibili** (SDK della `pci.library`): si scaricano in CI da una fonte ufficiale con checksum verificato. Se non si può, si compila il backend Mediator solo localmente e la CI usa un backend stub, così il resto viene comunque compilato.

---

## 4. Architettura del codice

### 4.0 Modello a layer

Il progetto si suddivide in sei strati. Ognuno ha un'**interfaccia definita** verso quello superiore e **non conosce** i dettagli di quelli più in basso. La regola: **un cambio di hardware tocca solo L0 e L1**.

| Layer | Nome | Responsabilità | Componenti (target) | Interfaccia verso l'alto |
|---|---|---|---|---|
| **L0** | Accesso al bus | Possesso dello slot o della scheda, riconoscimento (CIS o PCI ID), mappatura delle finestre di memoria e I/O, accessi a 8/16/32 bit con il corretto ordine dei byte, installazione e rimozione dell'interrupt, notifica di inserimento ed estrazione, (solo PCI) memoria DMA e cache | `src/bus/pcmcia_gayle.c` (**primo backend**), in futuro `src/bus/pci_mediator.c`, `src/bus/pci_openpci.c` | `struct HwBus`: `read8/16/32`, `write8/16/32`, `read_fifo`/`write_fifo` (copia a blocchi), `add_irq`/`rem_irq`, `on_remove`, (opzionale) `dma_alloc`/`cpu_to_bus`/`dma_sync` |
| **L1** | Driver del chip (HCD core) | Programmazione del controller, scheduling dei trasferimenti, gestione del buffer interno o di ED/TD, interrupt del chip, emulazione del root hub | `src/hcd/isp116x/` o `src/hcd/sl811/` (**primo**), `src/hcd/ohci/` (legacy, per PCI), `src/hcd/virt/` (test) | Funzioni interne usate dallo scheletro del device: `submit(xfer)`, `abort(xfer)`, `rh_control()`, `rh_status()` |
| **L2** | Interfaccia HCD | Exec device standard: `OpenDevice`, `BeginIO`/`AbortIO`, unità, task dell'unità, coda delle richieste, timeout | `src/hcd/devskel/` (comune a tutti gli HCD) | **`devices/usbhardware.h`** (`IOUsbHWReq`, `UHCMD_*`) |
| **L3** | USBD | Enumerazione, indirizzi, configurazioni, hub driver unico, power budget, registro e binding delle classi, hot-plug | `lupousb.library` | API USBD (`sfd/lupousb_lib.sfd`) |
| **L4** | Class driver | HID (mouse, tastiera, joypad), hub, mass storage … | `hid.class`, `hub` (dentro USBD o separato), `msd.class` | Verso il sistema: `input.device`, Exec device SCSI/trackdisk |
| **L5** | Applicazioni e tool | Prefs, diagnostica, elenco dispositivi, log | `usbinfo`, `LupoUsbPrefs`, `showregs`, `usblog` | – |

Note sul modello:

- **L0 è puro accesso all'hardware e non sa nulla di USB.** Lo stesso `pcmcia_gayle.c` potrebbe servire un driver di rete PCMCIA. Questo lo rende testabile in WinUAE con una scheda PCMCIA qualsiasi.
- **L1 non sa nulla di Exec device né di PCMCIA.** Riceve un `struct HwBus *` e lo usa per tutti gli accessi. Per esempio, un driver ISP116x è identico sia che il chip stia su PCMCIA sia, ipoteticamente, su clockport: cambia solo L0.
- **L2 è il confine stabile del progetto.** Sopra L2 il codice non cambia, qualunque scheda si usi. Con `usbhardware.h` si può inoltre validare L0+L1+L2 con uno stack esistente, o L3+L4 con un HCD esistente.
- **L3 e L4 sono C portabile:** si testano sull'host Linux (parser, enumerazione, hub) e in emulatore con l'HCD virtuale.
- **Dipendenze ammesse solo verso il basso**, mai salti di livello. Il caso attuale del root hub che scrive nei registri da L4/L5 (`RootHubClass.c`) è esattamente ciò che il modello vieta.
- Ogni layer ha la propria directory, il proprio header di interfaccia (`include/lupousb/hwbus.h`, `devices/usbhardware.h`, `proto/lupousb.h`) e i propri test.

### 4.1 Valutazione dell'architettura attuale

```
 UsbMouse / RootHub / AddUsbFunction (processi)
          │ lupousb.library  (wrapper: compila HC_REQUEST → PutMsg → WaitPort)
          ▼                              ▼
 "Lupo Usb Port" (processo UsbDriver)    "Lupo Usb Stack" (processo UsbStack)
   dispatcher CMD_*, polling WaitTOF       tabella indirizzi, enumerazione
   ISR → done queue → ReplyMsg            lancia "run c:AddUsbFunction N"
          │
   registri OHCI + memoria VRAM (GfxMemory) esposta ai client
```

| Aspetto | Valutazione |
|---|---|
| Processo kernel con MsgPort | L'idea di serializzare l'accesso all'hardware in un task è giusta (è quello che fa un device Exec). La realizzazione però è un protocollo privato (`HC_REQUEST` con `IO_Command`/`IO_Data`/`IO_Buffer[]`): reinventa `IORequest` senza `BeginIO`/`AbortIO`/`CheckIO`, senza `OpenDevice` e senza la gestione standard delle unità |
| Libreria come wrapper di messaggi | Ogni chiamata è un round-trip sincrono: l'API asincrona di fatto non esiste e i client restano bloccati. Lo stato (porte del kernel trovate con `FindPort` senza `Forbid`) è fragile se il kernel termina |
| API a ED espliciti (`AllocEndpoint`/`OpenPipe` con `ENDPOINT_CONTROL`) | Espone ai client i dettagli di OHCI (velocità, formato, skip, toggle carry). Non sopravviverebbe all'aggiunta di UHCI o EHCI |
| Memoria DMA della scheda grafica esposta ai client | Necessaria sulla Mediator, ma `AllocUsbBuffer` lega ogni client all'hardware. Inoltre la finestra VRAM si configura a mano e rischia di sovrapporsi alla memoria usata da Picasso96/CyberGraphX |
| Root hub | Accesso diretto ai registri da un processo esterno, quindi niente hub esterni senza riscrittura |
| Binding delle classi | `Execute("run c:AddUsbFunction N")`: creativo, ma senza alcun meccanismo di matching |
| Interrupt | ISR corposa, interrupt SOF continuo, main loop in polling |
| Debug | `printf` sparse e contatori `IntCount[]`: utili ma non strutturati |

### 4.2 Architettura target

```
 ┌──────────────────────────────────────────────────────────────┐
 │ Applicazioni, tool (usbinfo, prefs), input.device, filesystem │
 └───────────────▲──────────────────────────────▲───────────────┘
                 │ eventi input                 │ (fase 7) exec device SCSI
 ┌───────────────┴───────────┐  ┌───────────────┴──────────────┐
 │ hid.class (mouse/kbd/joy) │  │ msd.class (BOT + SCSI)       │   Class driver
 └───────────────▲───────────┘  └───────────────▲──────────────┘   (caricati on demand)
                 │      API USBD (lupousb.library v2)            
 ┌───────────────┴──────────────────────────────┴──────────────┐
 │ USBD core: enumerazione, indirizzi, config, power budget,   │
 │ hub driver (root hub ed hub esterni con lo STESSO codice),  │
 │ registro e binding delle classi, notifiche hot-plug         │
 └───────────────▲──────────────────────────────────────────────┘
                 │ Interfaccia HCD = devices/usbhardware.h (IOUsbHWReq, UHCMD_*)
 ┌───────────────┴─────────────┐ ┌────────────────┐ ┌──────────────┐
 │ lupoisp116x.device /        │ │ lupoohci.device│ │ lupovirt.dev │  L1+L2: HCD
 │ lupo811.device (PCMCIA)     │ │ (legacy, PCI)  │ │ (test/mock)  │
 │ unit task + ISR, buffer chip│ └───────▲────────┘ └──────────────┘
 │ root hub emulato            │         │
 └───────────────▲─────────────┘         │
                 │ struct HwBus (include/lupousb/hwbus.h)
 ┌───────────────┴──────────────────┐ ┌──┴─────────────────────────────┐
 │ pcmcia_gayle.c (card.resource)   │ │ pci_mediator.c │ pci_openpci.c │  L0: bus
 │  ← PRIMO BACKEND                 │ │  (futuro / opzionale)          │
 └──────────────────────────────────┘ └────────────────────────────────┘
```

#### D1 – HCD come Exec device con interfaccia `usbhardware.h`

`devices/usbhardware.h` è l'interfaccia de facto tra stack USB e driver degli host controller su AmigaOS 3.x: è nata con Poseidon e la usano diversi driver di terze parti. Concettualmente funziona così (i dettagli vanno verificati sull'header ufficiale):

- un `struct IOUsbHWReq`, che estende `IORequest`, porta indirizzo del dispositivo, endpoint, direzione, MPS, intervallo, flag di velocità, `UsbSetupData`, buffer e lunghezza;
- i comandi principali sono `UHCMD_QUERYDEVICE`, `UHCMD_USBRESET`, `UHCMD_USBOPER`, `UHCMD_USBSUSPEND`/`RESUME`, `UHCMD_CONTROLXFER`, `UHCMD_INTXFER`, `UHCMD_BULKXFER` e `UHCMD_ISOXFER`;
- è **senza stato lato client**: non si allocano ED. Il device crea e mette in cache gli ED per la coppia (indirizzo, endpoint, direzione) e gestisce internamente toggle e banda;
- il **root hub è emulato dal device** come un normale hub USB: le richieste di controllo verso il suo indirizzo ricevono risposte da classe hub, e i cambi di stato delle porte arrivano come trasferimenti interrupt sul suo endpoint di status.

Perché adottarla:

1. **Validazione incrociata.** Il device PCMCIA si può provare con uno stack esistente e collaudato, e il nuovo USBD si può provare con un HCD esistente. È il modo più rapido per capire da che parte sta un bug. Va verificato se esistono già driver `usbhardware` per schede PCMCIA a 16 bit sull'A1200: se sì, sono un ottimo banco di prova per L3/L4; se no, il device sarebbe utile anche ad altri.
2. **Hub esterni gratis.** Root hub ed hub esterni passano dallo stesso hub driver (D6).
3. **Separazione netta.** UHCI ed EHCI diventano altri device dietro la stessa interfaccia, e l'USBD non cambia.
4. **Utilità per gli utenti.** Un device compatibile può servire anche a chi usa altri stack.

**Licenze:** si implementa l'interfaccia, non si copia codice. Poseidon è open source con licenza AROS Public License, mentre il progetto è MIT: niente codice copiato, nemmeno adattato. Per l'header ci sono due strade: usarlo come dipendenza esterna in `third_party/`, oppure riscriverlo dalla documentazione mantenendo la compatibilità binaria. Il secondo punto va deciso dopo aver verificato la licenza dell'header stesso.

*Alternativa scartata:* un'interfaccia HCD privata. Darebbe più libertà, ma niente validazione incrociata e un lavoro di compatibilità da fare comunque in futuro.

#### D2 – Interrupt: ISR minima e `Signal()` al task dell'unità

Il principio vale per qualunque chip: l'ISR sta in L0+L1, la logica nel task dell'unità. Su PCMCIA l'ISR si aggancia tramite L0 all'interrupt di Gayle (condiviso). Deve leggere lo stato del chip **via I/O PCMCIA**, fare l'ack e segnalare il task. Va anche protetta contro la scheda estratta: se la lettura restituisce `0xFFFF`, la scheda non c'è più e l'ISR deve restituire 0. Di seguito il caso OHCI come esempio concreto; per ISP116x i bit sono analoghi (registri OHCI-compatibili più gli interrupt del buffer ATL/INTL).

- **ISR (esempio OHCI):**
  1. legge `HcInterruptStatus & HcInterruptEnable`;
  2. se il risultato è 0, **restituisce 0**, perché la catena è condivisa;
  3. per WDH legge `HccaDoneHead & ~0xF`, lo accoda a una lista privata e azzera l'HCCA;
  4. fa l'ack dei bit gestiti;
  5. chiama `Signal(unit_task, SIGF_DONE | SIGF_RHSC | …)`;
  6. restituisce 1.

  Nessun'altra logica: niente lettura dei registri del root hub e niente messaggi.
- **Task dell'unità:** fa `Wait(port_sig | done_sig | rhsc_sig | timer_sig | SIGBREAKF_CTRL_C)`. Elabora le nuove `IOUsbHWReq` (`BeginIO` accoda sulla MsgPort dell'unità), la done queue (completamento con `ReplyMsg`), i cambi del root hub (completa l'interrupt transfer pendente sull'hub emulato) e i timeout, tramite `timer.device` (timeout NAK, ritardi di reset).
- Lo scambio di liste tra ISR e task si protegge con `Disable()`/`Enable()` per **poche istruzioni**. I campi condivisi sono `volatile`.
- **SOF** si abilita solo come one-shot quando serve sincronizzarsi col frame, per esempio per rimuovere un ED dalla lista periodica in sicurezza (skip, attesa di un frame, unlink). **Mai in permanenza.**
- `AbortIO` è supportato: si mette lo skip sull'ED, si attende il frame, si tolgono i TD della richiesta e si risponde con `IOERR_ABORTED`.

#### D3 – Gestione dei trasferimenti e della banda interna all'HCD

**Chip PCMCIA a I/O programmato (target):**

- **ISP116x:** l'HCD costruisce i PTD (intestazione del trasferimento + dati) e li copia nel buffer ATL (control e bulk) o INTL (interrupt) del chip con `write_fifo` di L0. A fine transazione legge stato e dati. Il buffer interno è piccolo (pochi KB), quindi l'HCD lo gestisce come un pool a blocchi e spezza i trasferimenti lunghi.
- **SL811HS:** una transazione alla volta, programmata dalla CPU. Il task (o l'ISR, per i soli passi brevi) avanza una macchina a stati per ogni trasferimento attivo, con priorità al periodico all'inizio di ogni frame (SOF).
- In entrambi i casi **toggle, NAK, retry e intervalli periodici sono gestiti in software dall'HCD**. La logica di scheduling del vecchio `Endpoint.c` (intervalli, banda per frame) si riusa come algoritmo.

**OHCI (legacy, per un eventuale backend PCI):**

- La logica attuale di `Endpoint.c` (albero interrupt, liste control e bulk, done queue, gestione dell'halt) si porta quasi tale e quale in `ohci_sched.c`, correggendo i bug #1–#3 e #14.
- Gli ED si creano su richiesta per (indirizzo, endpoint, direzione, velocità, MPS) e si distruggono a `UHCMD_USBRESET` o quando l'USBD segnala la rimozione del dispositivo.
- Il toggle sta sempre nel **carry dell'ED**. Per i TD non di controllo il campo toggle del TD vale 0.
- Il controllo di banda periodica si fa all'apertura del primo trasferimento interrupt su un endpoint, con lo stesso algoritmo attuale corretto.

#### D4 – Astrazione del bus (layer L0) e memoria

Interfaccia L0 comune a tutti i backend:

```c
/* include/lupousb/hwbus.h – bozza concettuale */
struct HwBus;                       /* opaco: un'istanza per slot/scheda */
struct HwBusOps {
    UBYTE  (*read8)  (struct HwBus *, ULONG off);
    UWORD  (*read16) (struct HwBus *, ULONG off);
    void   (*write8) (struct HwBus *, ULONG off, UBYTE v);
    void   (*write16)(struct HwBus *, ULONG off, UWORD v);
    void   (*read_fifo) (struct HwBus *, ULONG off, APTR dst, ULONG len); /* copia a blocchi, ciclo stretto */
    void   (*write_fifo)(struct HwBus *, ULONG off, CONST_APTR src, ULONG len);
    BOOL   (*add_irq)(struct HwBus *, struct Interrupt *);
    void   (*rem_irq)(struct HwBus *, struct Interrupt *);
    BOOL   (*present)(struct HwBus *);                 /* scheda ancora inserita? */
    void   (*set_remove_hook)(struct HwBus *, struct Hook *); /* notifica estrazione */
    /* solo bus con DMA (PCI): NULL su PCMCIA */
    APTR   (*dma_alloc)(struct HwBus *, ULONG size, ULONG align);
    void   (*dma_free) (struct HwBus *, APTR);
    ULONG  (*cpu_to_bus)(struct HwBus *, APTR);
    void   (*dma_sync) (struct HwBus *, APTR, ULONG, int dir);
};
```

**Backend `pcmcia_gayle.c` (primo):**

- **Possesso dello slot** con `OwnCard()` di `card.resource`, con handler di inserimento, estrazione e stato. `ReleaseCard()` alla chiusura. Deve convivere con `carddisk.device` e con altri driver PCMCIA.
- **Riconoscimento** tramite il CIS (`CopyTuple`): manfid e card ID, `CISTPL_CONFIG`/`CISTPL_CFTABLE_ENTRY` per la configurazione I/O. Si scrive il registro COR della scheda per abilitare la modalità I/O. Una tabella delle schede note associa la scheda al driver L1 corretto (ISP116x, SL811HS, …).
- **Accessi** nell'area I/O e attributo di Gayle con le convenzioni corrette di larghezza e ordine dei byte. È tutto da verificare sulla documentazione hardware dell'A1200 e confrontando driver PCMCIA open source per A1200 (come concetto, senza copiare codice). `read_fifo`/`write_fifo` sono cicli stretti di `move.w`, l'unico punto dove un po' di assembly o di inline asm può valere la pena.
- **Estrazione a caldo:** all'avviso di estrazione L0 marca il bus come assente e da quel momento tutte le letture restituiscono un valore sentinella. L1 annulla tutte le richieste con un errore specifico (in `usbhardware`, il device si dichiara "rimosso") e L3 vede la scomparsa di tutti i dispositivi del bus. **Mai** accedere alla scheda dopo l'estrazione: su Amiga può bloccare il sistema.
- **Velocità di accesso** impostata con `CardAccessSpeed()` secondo il CIS o il datasheet del chip.

**Backend PCI (eventuale, per il codice OHCI legacy):** stessa interfaccia più le funzioni DMA, secondo lo schema seguente.

```c
/* bozza concettuale delle funzioni aggiuntive per un backend PCI */
struct PciBusOps {
    BOOL   (*find_controller)(struct PciBus *, ULONG classcode, ULONG index, struct PciFunc *out);
    APTR   (*map_bar)(struct PciFunc *, int bar);
    BOOL   (*add_irq)(struct PciFunc *, struct Interrupt *);
    void   (*rem_irq)(struct PciFunc *, struct Interrupt *);
    APTR   (*dma_alloc)(struct PciBus *, ULONG size, ULONG align); /* memoria raggiungibile dal busmaster */
    void   (*dma_free)(struct PciBus *, APTR);
    ULONG  (*cpu_to_bus)(struct PciBus *, APTR);                    /* niente più "indirizzo CPU == indirizzo bus" */
    APTR   (*bus_to_cpu)(struct PciBus *, ULONG);
    void   (*dma_sync)(struct PciBus *, APTR, ULONG, int dir);       /* CachePreDMA/PostDMA o no-op */
    ULONG  flags;                                                    /* es. BUS_REGS_BYTESWAPPED */
};
```

- **Ricerca del controller** per class code PCI (`0x0C0310` OHCI, `0x0C0300` UHCI, `0x0C0320` EHCI) invece di VendorID/DeviceID impostati a mano. Più controller corrispondono a più unità del device.
- **Mediator:** `dma_alloc` usa la finestra VRAM attuale (l'allocatore di `GfxMemory.c`, ripulito). Si tiene la configurazione manuale come fallback, ma si cerca un modo per farsi assegnare la regione dal sistema RTG invece di sovrapporsi alla cieca. Su busboard che possono fare DMA in RAM Amiga, `dma_alloc` diventa `AllocMem` più `CachePreDMA`/`CachePostDMA`.
- **Bounce buffer:** i client passano buffer in memoria normale e l'HCD copia da e verso il pool DMA. Per HID (8–64 byte) il costo è trascurabile. Per il mass storage si valuterà più avanti un percorso a copia zero sulle busboard che lo permettono. Il bounce gestisce anche il vincolo OHCI dei buffer dei TD, che possono attraversare al massimo un confine di pagina da 4 KB.
- **Endianness:**
  - i registri si leggono e scrivono **solo** con `ohci_readl`/`ohci_writel`, che fanno lo swap a seconda del flag del backend (alcune bridge possono fare lo swap in hardware);
  - i descrittori in memoria (ED, TD, HCCA) si accedono **solo** con helper `le32`;
  - le costanti dei bit si definiscono nell'ordine logico della specifica OHCI, non pre-swappate come oggi (`HC_INT_RHSC 1<<30` è in realtà il bit 6 dopo lo swap). Le macro vanno sempre tra parentesi.

#### D5 – Configurazione

- Le prefs passano da dump binario a **testo** (`ENVARC:USB/lupousb.prefs`, coppie chiave=valore) o a un chunk IFF versionato. Contengono solo ciò che non si può rilevare: override per schede PCMCIA non riconosciute dal CIS, finestra DMA (solo backend Mediator), override del frame interval, livello di log.
- Il tool prefs legge e scrive lo stesso formato e mostra i controller rilevati.

#### D6 – USBD (`lupousb.library` v2)

- **Handle opachi** (`struct UsbDevice *`, `struct UsbInterface *`, `struct UsbPipe *`) al posto delle struct interne esposte.
- **Trasferimenti asincroni** sul modello IORequest (invio con reply port, `Wait` sul segnale, `Abort`), più wrapper sincroni comodi (`UsbControl()`, `UsbGetDescriptor()`, …). Le richieste standard del Chapter 9 già scritte in `LupoUsbLibrary.c` si portano qui come funzioni di utilità sopra `UsbControl()`.
- **Un task USBD per bus** gestisce l'enumerazione in serie, con un lock sull'indirizzo 0, rispettando i tempi della specifica:
  1. debounce di 100 ms dopo la connessione;
  2. reset della porta di 10–20 ms;
  3. `GET_DESCRIPTOR` da 8 byte → MPS dell'EP0 dal **byte 7**;
  4. `SET_ADDRESS` + 2 ms;
  5. descrittore completo;
  6. configurazione e stringhe.
- **Hub driver unico:** il root hub (emulato dall'HCD) e gli hub esterni sono la stessa cosa per l'USBD (status change endpoint, `GET_PORT_STATUS`, `SET_FEATURE(PORT_RESET)`, …).
- **Binding delle classi:** un registro dei class driver, cioè librerie in `LIBS:USB/` o `SYS:Classes/USB/`, ciascuna con un piccolo `.sfd` di entry point (`AttemptBinding(dev, iface)`, `ReleaseBinding(binding)`, `GetAttrs`). L'USBD prova i class driver per interfaccia (classe/sottoclasse/protocollo, oppure VID/PID) e carica quello giusto con `OpenLibrary`. È un modello concettualmente simile a quello degli stack esistenti, ma con un'API propria.
- Le **notifiche hot-plug** passano da una porta pubblica a cui `usbinfo` e altri tool possono abbonarsi.

#### D7 – Class driver HID

- Un **parser dei report descriptor** generico (item globali, locali e principali, usage page e usage) è puro C portabile, testabile sull'host con report descriptor reali catturati.
- C'è un **fallback sul boot protocol** (`SET_PROTOCOL(0)`) per mouse e tastiera.
- **Mouse:** `IECLASS_RAWMOUSE` con delta relativi, **un evento per ogni transizione di pulsante** (`IECODE_LBUTTON`, `IECODE_LBUTTON|IECODE_UP_PREFIX`, …) e `IEQUALIFIER_RELATIVEMOUSE` più i qualificatori dei pulsanti tenuti premuti. La rotellina segue la convenzione NewMouse (`IECLASS_NEWMOUSE` e rawkey `0x7A`/`0x7B`).
- **Tastiera:** tabella usage HID → rawkey Amiga, auto-repeat lasciato a `input.device`, LED tramite `SET_REPORT`.
- **Joypad (Gravis e generici):** in una prima fase si mappa su eventi configurabili (tasti o mouse) e si espone un'API di lettura. Sostituire `lowlevel.library`/`ReadJoyPort()` è un argomento a parte (patch di sistema): da valutare dopo.
- **Un task per binding**, con l'interrupt transfer sempre pendente e `Wait()` sul completamento. Gli eventi si iniettano con `IND_WRITEEVENT` dallo stesso task, che deve essere un processo o usare solo Exec: niente `Delay()` e niente task di supporto.

#### D8 – Logging e debug

- Macro `LOG(level, fmt, …)` compilate via in release (`-DNDEBUG`). Il backend è selezionabile:
  - `kprintf` su seriale (`debug.lib`), indispensabile per ISR e avvio;
  - ring buffer in memoria leggibile da un tool `usblog` (o da Sashimi).
- I contatori (`IntCount[]`) si tengono ma con nomi, esposti con `UHCMD_QUERYDEVICE` o con una funzione di statistica e mostrati da `showregs`.
- `Debugger.c` (dump di ED e TD) diventa una funzione di debug dell'HCD invocabile dal tool.

#### D9 – Robustezza e convenzioni

- Niente `while(!AllocMem(...));`: ogni allocazione si controlla e l'errore si propaga.
- Niente `FindPort` senza `Forbid`. Si usano `OpenDevice`/`OpenLibrary`, che gestiscono il conteggio di aperture ed expunge.
- `Expunge` va implementato correttamente: si rifiuta finché ci sono aperture o richieste pendenti.
- Il codice si compila senza warning con `-Wall -Wextra` su gcc e vbcc.

### 4.3 Confronto sintetico

| Area | Attuale | Target |
|---|---|---|
| Hardware | OHCI su Mediator PCI (non più disponibile) | Scheda USB PCMCIA a 16 bit sull'A1200; OHCI/PCI come backend opzionale |
| Stratificazione | Confini sfumati (il root hub tocca i registri, i client vedono la VRAM) | Sei layer L0–L5 con interfacce definite |
| Driver HC | Processo con protocollo privato | Exec device con interfaccia `usbhardware.h` |
| Interrupt | ISR corposa, SOF continuo, polling con `WaitTOF` | ISR minima + `Signal()`, task in `Wait()`, SOF on demand |
| ED/TD | Esposti ai client | Interni all'HCD |
| DMA | VRAM esposta ai client, configurazione manuale | Pool interno, bounce buffer, backend per busboard |
| Endianness | Accessori asm, costanti pre-swappate | Helper C inline, costanti come da specifica |
| Root hub | Registri toccati da un processo esterno | Emulato nell'HCD come hub standard |
| Hub esterni | Impossibili senza riscrittura | Stesso hub driver |
| Classi | `run c:AddUsbFunction N` | Registro e binding dinamico di librerie class |
| HID | Mouse cablato a indirizzo 2, 200 letture | `hid.class` generico con parser dei report |
| Config | Struct binaria in `ENV:` | Rilevamento automatico + prefs testuali |
| Test | Solo su hardware | Host + HCD virtuale + hardware |

### 4.4 Altri bus e busboard

L'astrazione L0 di D4 è il punto d'innesto. **Il primo backend è `pcmcia_gayle.c`.** I backend PCI sono opzionali e servono solo a riattivare il codice OHCI legacy. Se un giorno verranno fatti, l'ordine consigliato è:

1. `pci_mediator.c` (porting di quanto esiste);
2. `openpci.c`, se la verifica conferma che copre Mediator, Prometheus e G-REX con un'unica API;
3. backend specifici solo dove OpenPCI non arriva.

Per ogni backend la domanda chiave è sempre **"dove può fare DMA il busmaster?"**, e la risposta va documentata in `Documents/Busboards.md`.

---

## 5. Roadmap

| Fase | Contenuto | Criterio di uscita | Priorità | Rischio |
|---|---|---|---|---|
| **F0 – Conservazione** | Tag `legacy-2003`, release con i binari originali, `.gitattributes`/`.gitignore`, spostamento in `legacy/`, rimozione di binari e `.info` dall'albero | Repository pulito, storico preservato | Altissima | Basso |
| **F1 – Build ricostruita** | Container con toolchain, Makefile, SFD dal `.fd`, scheletro della libreria scritto a mano. Porting **minimo** dei sorgenti legacy a C standard: niente inizializzatori globali con chiamate, include con maiuscole corrette, `};else`, float, `struct` mancanti. CI verde con gcc e vbcc | Tutti i componenti legacy compilano in CI | Alta | **Medio-alto**: header e `.fd` della `pci.library`, sostituzione dell'auto-init StormC |
| **F1.5 – Scheda PCMCIA e layer L0** | Identificazione della scheda (CardBus o 16 bit, sigla del chip, dump del CIS). Datasheet del chip. `include/lupousb/hwbus.h` e `src/bus/pcmcia_gayle.c` (`card.resource`, CIS, finestra I/O, interrupt, estrazione). Tool `cisdump`. Prove di L0 in WinUAE con una scheda PCMCIA emulata | Chip identificato e documentato. `cisdump` funziona su A1200 reale; inserimento ed estrazione senza blocchi | **Altissima** | **Alto**: scheda forse CardBus, documentazione di Gayle, quirk dello slot, A1200 funzionante da procurare |
| **F2 – Bug fix sul legacy** | Bug #1–#25 (prima #6, #12, #13, #16, poi #1–#5, #7–#11, #14, #17), **solo per compilare e come riferimento**: senza Mediator non si può provare su hardware | Il legacy compila pulito con gcc e vbcc | Media (scesa: non più testabile) | Basso |
| **F3 – HCD PCMCIA come device** | Scheletro del device L2 (`devskel`) con `usbhardware.h`. Driver L1 del chip (`isp116x` o `sl811`): reset, root hub emulato sulle 2/4 porte (incluso il chip hub della scheda, se presente), trasferimenti control e interrupt. ISR + `Signal()`. `lupovirt.device` per i test | Enumerazione e mouse funzionanti sull'A1200 con la scheda PCMCIA, idealmente anche con uno stack di riferimento esistente sopra il device | Alta | Alto (chip nuovo, debug solo su hardware) |
| **F4 – USBD v2** | `lupousb.library` v2: enumerazione, hub driver unico, binding, notifiche, `usbinfo`. Test sull'host per enumerazione e descrittori. **Può procedere in parallelo a F1.5 e F3** grazie a `lupovirt.device` | Con `lupovirt.device` in emulatore e con il device PCMCIA su hardware, un dispositivo viene enumerato e bindato | Alta | Medio |
| **F5 – HID generico** | `hid.class`: parser dei report, mouse (con rotellina), tastiera, joypad, prefs di mapping | Mouse e tastiera generici usabili in Workbench; joypad Gravis mappabile | Media-alta | Medio |
| **F6 – Hub esterni** | Il `hub.class`/hub driver sugli hub fisici: power budget, dispositivi low-speed dietro hub full-speed (preambolo PRE gestito da OHCI), over-current | Mouse e tastiera dietro un hub USB 1.1/2.0 | Media | Medio |
| **F7 – Bulk e mass storage** | Pipe bulk, `msd.class` (Bulk-Only Transport + SCSI transparent), esposto come Exec device stile `scsi.device` (`HD_SCSICMD`, `TD_*`, NSD 64 bit) per montare chiavette con FFS/SFS/PFS o FAT95 | Lettura e scrittura affidabili di una chiavetta | Media | Alto: banda limitata dal PIO su Gayle, buffer interno del chip piccolo, gestione errori |
| **F8 – Altri chip e bus** | Secondo driver L1 (SL811HS o ISP116x, quello non fatto in F3), eventuale ISP1760, backend PCI con il codice OHCI legacy, UHCI | Stack funzionante su un secondo chip o bus | Bassa | Alto |

Attività trasversali da fare in ogni fase:

- README e `Documents/` aggiornati;
- `CHANGELOG.md`;
- versionamento semantico delle librerie;
- pacchetto LHA con script Installer;
- eventuale pubblicazione su Aminet al raggiungimento di F5.

### 5.1 Rischi principali e mitigazioni

| Rischio | Impatto | Mitigazione |
|---|---|---|
| **La scheda USB PCMCIA disponibile è CardBus** | Inutilizzabile sull'A1200 | Verificarlo subito (F1.5). In caso, cercare una PC Card a 16 bit con ISP1160/1161 o SL811HS |
| **Chip della scheda poco documentato** | Driver L1 difficile | Cercare i datasheet prima di tutto. Preferire schede con chip documentati (ISP116x ha datasheet pubblici e registri OHCI-compatibili) |
| **A1200 funzionante da procurare**, con il PCMCIA reset fix se serve e senza memoria Fast sovrapposta all'area PCMCIA | Blocca F1.5 (parte hardware) e F3 | Procurarlo presto. Nel frattempo test sull'host, HCD virtuale e prove di L0 in WinUAE |
| Quirk dello slot PCMCIA (accessi, interrupt condivisi, estrazione a caldo) | Blocchi del sistema | L0 isolato e testato per primo, con un tool dedicato (`cisdump`). Studio dei driver PCMCIA esistenti come riferimento concettuale |
| Banda limitata dal PIO su Gayle | Mass storage lento | Aspettative documentate. HID e hub sono l'obiettivo principale; il mass storage è un bonus |
| Header e `.fd` della `pci.library` non ridistribuibili | Solo il legacy OHCI non compila in CI | Il problema riguarda ormai solo il backend PCI opzionale: backend stub in CI, download locale |
| Nessun emulatore offre una scheda USB | Nessun test dell'HCD in emulatore | HCD virtuale e test unitari con un chip finto (registri e buffer simulati in memoria) per L1 |
| Licenze (Poseidon con APL, progetto MIT) | Contaminazione del codice | Implementazione clean-room dell'interfaccia, nessun codice copiato; header di interfaccia come dipendenza esterna o riscritto dalla documentazione dopo verifica |
| (solo backend PCI opzionale) Sovrapposizione VRAM tra USB e RTG sulla Mediator | Corruzione grafica o DMA | Regione riservata documentata. Studiare l'allocazione tramite il sistema RTG |
| Cache su 040/060 | Dati DMA incoerenti | `dma_sync` nel backend. Test con cache attive su un 040/060 |
| Ambito eccessivo (EHCI, mass storage) | Progetto mai concluso | Fasi con criteri di uscita. F5 è già un traguardo pubblicabile |

### 5.2 Prossimi passi concreti

1. ~~Identificare la scheda USB PCMCIA~~ fatto: entrambe CardBus. **Procurare una Ratoc REX-CFU1U (o un'altra CF+ USB con SL811HS) e un adattatore passivo CF→PCMCIA**, verificando il funzionamento a 5 V. Il primo driver L1 è `sl811`.
2. Rivedere questo documento e decidere su D1 (`usbhardware.h`), sul modello a layer (§4.0) e sulla toolchain.
3. **F0:** tag `legacy-2003`, attivazione di `.gitattributes`/`.gitignore` dalle bozze, `git mv Sources legacy/Sources`.
4. **F1:** Dockerfile della toolchain, Makefile dalla bozza, conversione `.fd` → `.sfd`, scheletro di `lupousb.library` e prima build CI del legacy portato (con backend PCI stub).
5. **F1.5:** procurare i datasheet del chip, scrivere `hwbus.h` e `pcmcia_gayle.c`, e il tool `cisdump`.
6. In parallelo, procurarsi un A1200 funzionante su cui provare.

---

## Appendice A – Bozze aggiunte

| File | Contenuto | Destinazione finale |
|---|---|---|
| `Documents/proposals/gitignore.draft` | `.gitignore` proposto | `/.gitignore` |
| `Documents/proposals/gitattributes.draft` | `.gitattributes` proposto | `/.gitattributes` |
| `Documents/proposals/Makefile.draft` | Struttura del Makefile con toolchain selezionabile | `/Makefile` + `mk/` |
| `Documents/proposals/build.yml.draft` | Workflow di GitHub Actions | `/.github/workflows/build.yml` |

Vedi anche `Documents/Layers.md` per lo schema dettagliato degli strati di sviluppo.

Sono **bozze**: non sono attive e vanno adattate quando il nuovo albero `src/` esisterà.
