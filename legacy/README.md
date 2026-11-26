# legacy/ – build del codice originale 2001-2003 con gcc

Questa cartella contiene il **porting minimo** dei sorgenti originali (StormC 2.0 + Asm-One) verso la toolchain moderna (bebbo `m68k-amigaos-gcc` + `vasm`).
- `Sources/` resta **intatta**, come riferimento storico.
- `legacy/src` è nato come copia byte per byte (commit "legacy: verbatim copy"), quindi `git log -p legacy/src` mostra esattamente cosa ha cambiato il porting.

Il comportamento del codice **non** è stato corretto: anche i bug noti sono rimasti (vedi `Documents/ProjectReview.md` §1.3). Le correzioni spettano alla fase F2.

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
- `glue/roothub_data.c`: definisce `RHub`, che `RootHubClass.c` usa ma che era definito solo in `RootHubConstructors.h`, un header non più incluso e non più compilabile.
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
| 27 | `Usbdriver.c` / `Hcdriverstructs.h` | `IO_Command` è `UWORD` ma `CMD_KILL_USB` vale `-1`: il valore inviato diventa `0xFFFF` e `case CMD_KILL_USB` non corrisponde mai, quindi il kernel non riceveva mai il comando di chiusura (gcc: `-Wswitch-outside-range`). Non corretto |
| 28 | `RootHubConstructors.h` / `RootHubClass.c` | `RHub` non è definito da nessun sorgente compilato: il progetto non si linkava con questi sorgenti. Risolto con `glue/roothub_data.c` |
| – | `Usbdriver.c` | Conferma del bug #4: `IO_Error=NO_ERROR` prima del primo `case` non viene mai eseguito (gcc: `-Wswitch-unreachable`) |

## Limiti

- I binari sono **compilati, non provati**: servirebbe una Mediator con scheda OHCI, che non è più disponibile. Lo scopo è avere un riferimento compilabile e poter confrontare il comportamento.
- La build legacy non è nella CI a causa degli header Elbox. Si può aggiungere con lo stesso approccio di `hwreq.h`: un header `pci.library` indipendente, verificato contro quello originale in locale.
