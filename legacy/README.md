# legacy/ – build del codice originale 2001-2003 con gcc

Questa cartella contiene il **porting minimo** dei sorgenti originali (StormC 2.0 + Asm-One) verso la toolchain moderna (bebbo `m68k-amigaos-gcc` + `vasm`).
- `Sources/` resta **intatta**, come riferimento storico.
- `legacy/src` è nato come copia byte per byte (commit "legacy: verbatim copy"), quindi `git log -p legacy/src` mostra esattamente cosa ha cambiato il porting.

Il porting (fase F1) non ha cambiato il comportamento. I bug sono stati poi corretti nella **fase F2**, un commit per bug (vedi la tabella in fondo).

## Come si compila

```
make legacy            # dal Makefile principale, nel container della toolchain
```

Servono gli include dell'SDK in `third_party/sdk`, in particolare gli header e il `.fd` della `pci.library` della Mediator (© Elbox, non ridistribuibili). Per questo la build legacy **non gira nella CI** di GitHub.

Prodotti, in `build/legacy/bin/`:

| File | Origine (progetto StormC) |
|---|---|
| `C/UsbDriver` | `Usbdriver/UsbDriver.¶`: kernel HCD OHCI + `Asmhc.asm`, `AsmEndpoint.asm` |
| `Libs/lupousb.library` | `LupoUsbLibrary.¶` + `LupoUsbLibraryAsm.Asm` |
| `C/UsbStack` | `UsbStack.¶`: include il codice della libreria linkato staticamente, come nell'originale |
| `C/UsbRootHub` | `RootHub.¶` |
| `C/AddUsbFunction`, `C/UsbMouse` | omonimi |
| `C/USBShowRegs` | `UsbShowRegs/USBShowRegs.¶` + `Convert.asm` (ReAction) |
| `Prefs/LupoUsbPrefs` | `UsbPrefs/LupoUsbPrefs.¶` |

Non vengono compilati i file morti: `DefaultControlEndpoint.c`, `RootHubLib.c`, `UsbDevice.C`, `USB_Handler.c`, `EndpointOLD.c`.

## Cosa fa il porting

**Strumenti di build** (nessuna modifica ai sorgenti):
- `tools/mkinc.py`: AmigaOS non distingue maiuscole e minuscole nei nomi dei file, e i sorgenti includono lo stesso header con grafie diverse (`<usb/HcDriverStructs.h>`, `<usb/hcdriverstructs.h>`, `"Usb/..."`). Lo script crea in `build/legacy/inc` un link per ogni grafia, risolto senza distinguere le maiuscole: prima `legacy/compat`, poi la cartella del file, poi `legacy/src` (anche come `usb/...`), poi gli header generati, infine l'SDK.
- `compat/pragma/*_lib.h`: sostituiscono i file `#pragma amicall` di StormC con gli inline di gcc (`proto/*.h`).
- `compat/lupousblibrary_lib.h`: le chiamate dei programmi client a `lupousb.library`, tramite `inline/lupousb.h` generato da `LupoUsbLibrary.fd` con `fd2sfd` + `sfdc`.
- `compat/pragma/pci_lib.h`: le chiamate a `pci.library`, tramite `inline/pci.h` generato dal `.fd` Elbox. Al `.fd` va aggiunto `()` a `ReadMemOffset()`, solo nella copia generata in `build/`.
- `tools/fdtable.py` + `glue/libinit.c`: StormC generava da solo romtag, init/open/close/expunge e la tabella delle funzioni della libreria (`#pragma libbase`). Qui sono scritti esplicitamente, e la tabella segue l'ordine del `.fd`.
- (F1) `glue/roothub_data.c` definiva `RHub`; dalla F2 (#28) la definizione è in `RootHubClass.c` e il file è stato rimosso.
- Opzioni di compilazione:
  - `-std=gnu89 -fpermissive`, perché il codice è C89 con abitudini StormC;
  - `-fcommon`, per le variabili globali non inizializzate fuse dal linker come nel C di allora (`StackPort` è definita sia in `UsbStack.c` sia in `LupoUsbLibrary.c`);
  - `-D__saveds=`, perché non si usa il modello small-data.

**Modifiche ai sorgenti** (meccaniche, tutte visibili con `git log -p legacy/src`):
1. Parametri a registro: `register __a0 TIPO nome` diventa `TIPO nome __asm("a0")` (`tools/regconv.py`, 583 righe in 8 file).
2. Inizializzatori globali con chiamate a funzione, validi solo in C++ come li compilava StormC (`OpenLibrary`, `CreatePort`, `FindTask` in 6 file): diventano `NULL` e vengono assegnati all'inizio di `main()`, nello stesso ordine.
3. Nomi di struct usati senza `struct` (C++): `Hcdriverstructs.h`, `GfxMemory.c`.
4. Errori di sintassi presenti nei sorgenti, segno che non coincidono esattamente con i binari del 2003: `};` prima di `else` in `Usbdriver.c` e `;` mancante in `Debugger.c`. Inoltre `extern PciDevice` diventa `extern struct PciDevice` in `Interrupt.c`.
5. `LupoUsbLibraryAsm.Asm`: `END` in colonna 0 diventa ` END`. Asm-One lo riconosceva come direttiva, vasm come etichetta, e proseguiva leggendo gli `INCLUDE` messi dopo `END`.
6. `LupoUsbLibrary.fd`: `SetInterfaceFeature` dichiarava 4 registri per 5 argomenti. È un **bug reale** (vedi sotto), corretto perché `fd2sfd` rifiuta il file.

## Bug dell'originale emersi dal porting

| # | Dove | Problema |
|---|---|---|
| 26 | `LupoUsbLibrary.fd` | `SetInterfaceFeature(...)(a0,a1,a2,d0)`: manca `d1`. Chiamata tramite la libreria, la funzione riceveva in `NumInterface` un valore casuale. **Corretto** nel `.fd` |
| 27 | `Usbdriver.c` / `Hcdriverstructs.h` | `IO_Command` è `UWORD` ma `CMD_KILL_USB` vale `-1`: il valore inviato diventa `0xFFFF` e `case CMD_KILL_USB` non corrisponde mai, quindi il kernel non riceveva mai il comando di chiusura (gcc: `-Wswitch-outside-range`) |
| 28 | `RootHubConstructors.h` / `RootHubClass.c` | `RHub` non è definito da nessun sorgente compilato: il progetto non si linkava con questi sorgenti |
| – | `Usbdriver.c` | Conferma del bug #4: `IO_Error=NO_ERROR` prima del primo `case` non viene mai eseguito (gcc: `-Wswitch-unreachable`) |

## Limiti

- I binari sono **compilati, non provati**: servirebbe una Mediator con scheda OHCI, che non è più disponibile. Lo scopo è avere un riferimento compilabile e poter confrontare il comportamento.
- La build legacy non è nella CI a causa degli header Elbox. Si può aggiungere con lo stesso approccio di `hwreq.h`: un header `pci.library` indipendente, verificato contro quello originale in locale.

## Fase F2: correzione dei bug

Un commit per bug (`git log --oneline -- legacy/src`). Dopo ogni commit la build legacy è stata ricompilata. Alla fine la build pulita non produce avvisi, a parte un header Elbox e la ridefinizione di `__saveds`. Le correzioni **non sono state provate su hardware**: la Mediator non è disponibile.

| # | Correzione | Dove |
|---|---|---|
| 1 | Il toggle dei TD interrupt/bulk viene dal carry dell'ED (prima: sempre DATA1) | `Endpoint.c` |
| 2 | `OpenPipe` apre le pipe bulk e restituisce sempre un valore | `Endpoint.c` |
| 3 | Contabilità della banda periodica: si sottrae da `ListBandwidth` (prima da `EDList`, corrompendo un puntatore); `AviableBandwidth` diminuisce | `Endpoint.c` |
| 4 | `IO_Error` azzerato prima dello `switch` dei comandi | `Usbdriver.c` |
| 5 | `HcFindDevice` termina dopo i 32 slot (`&&` al posto di `\|\|`) | `Hcdriver.c` |
| 6 | Uscita pulita se `HcSetup` fallisce; `kill()` libera tutto anche senza scheda (prima restava la porta pubblica) | `Usbdriver.c` |
| 7 | MPS dell'EP0 dal byte 7 del descrittore, letto subito dopo `GET_DESCRIPTOR` | `UsbStack.c` |
| 8, 9, 24 | `UsbMouse`: un evento per transizione dei pulsanti, qualificatori corretti, niente task di supporto, indirizzo da riga di comando, velocità della pipe, Ctrl-C | `UsbMouse.c` |
| 10 | `SetUsbState` passa lo stato; `GetUsbBaseAddress` controlla il successo nel verso giusto | `LupoUsbLibrary.c` |
| 11 | Il kernel attende con `Wait()` il segnale della ISR o della porta (prima: polling con `WaitTOF()`) | `Usbdriver.c`, `Interrupt.c` |
| 12 | Il messaggio del root hub non viene mai accodato due volte; il processo root hub lo legge a interrupt disabilitati | `Usbdriver.c`, `RootHub.c` |
| 13 | La ISR restituisce 0 se l'interrupt non è della scheda OHCI (catena Mediator condivisa) | `Interrupt.c` |
| 14 | Il bit 0 di `HccaDoneHead` viene mascherato | `Interrupt.c` |
| 15 | Niente interrupt SOF (1000/s inutili) | `Hcdriver.c` |
| 16 | `RemoveMediatorPciInterrupt(HostController)`: prima veniva chiamata senza argomento, con prototipo `(void)` | `Usbdriver.c`, `Interrupt_protos.h` |
| 17 | `Get32BitFrameNumber` restituisce il frame; `FreeRootHubMessagePort` restituisce 0 con argomenti nulli | `LupoUsbLibrary.c` |
| 18 | `volatile` sui dati condivisi con la ISR; variabili della ISR locali | `Hcdriverstructs.h`, `Interrupt.c`, `Usbdriver.c` |
| 19 | Nessun ciclo infinito sulle allocazioni (13 punti): gli errori arrivano al chiamante. I TD di una richiesta sono riservati prima di toccare l'ED. Le richieste isocrone sono rifiutate. Corretto anche #30 | vari |
| 23 | Attesa di recupero dopo `SET_ADDRESS` | `UsbStack.c` |
| 27 | `CMD_KILL_USB` = `0xFFFF` | `Hcdriverstructs.h` |
| 28 | `RHub` definito in `RootHubClass.c` | `RootHubClass.c` |
| 29 | Rimosso il ciclo che toglieva dalla porta richieste senza rispondere (client bloccati) | `Usbdriver.c` |
| 30 | `OpenControlPipe`/`OpenBulkPipe` restituivano un valore indefinito in caso di successo | `Endpoint.c` (con #19) |
| 31 | `GetDeviceDescriptor` convertiva `iSerialNumber` (un byte, azzerato) invece di `bcdDevice` | `LupoUsbLibrary.c` |
| 32 | "Pipe non aperta" segnalata anche in `IO_Error`: prima il client attendeva per sempre | `Usbdriver.c` |
| 33 | `FreeTransferDescriptor` legge il TD solo dopo il controllo `NULL` | `Endpoint.c` |
| 34 | `AddUsbFunction`: `OpenAll() & FunctionNumber` (AND bit a bit) e `argv[1]` letto senza argomenti | `AddUsbFunction.c` |

**Non corretti nel codice storico**, perché sono scelte di architettura: il nuovo stack (`src/`) li risolve per costruzione.
- **#20:** il processo root hub scrive direttamente nei registri OHCI. Nel nuovo stack il root hub è emulato dentro l'HCD.
- **#21:** nessuna gestione della cache per il DMA. Nel nuovo stack è previsto `dma_sync` nel layer L0.
- **#22:** indirizzi CPU usati direttamente come indirizzi del bus PCI. Nel nuovo stack è previsto `cpu_to_bus` nel layer L0.
- **#25:** prefs salvate come dump binario di una struct. Il nuovo stack prevede prefs testuali e il riconoscimento automatico della scheda.
