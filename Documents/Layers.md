# LupoUsbDriver – Schema degli strati di sviluppo

*Complemento a `ProjectReview.md` (§4.0). Hardware target: A1200 + scheda USB PCMCIA a 16 bit, 2/4 porte.*

## 1. Vista d'insieme

```
 ╔═══════════════════════════════════════════════════════════════════════════╗
 ║ L5  APPLICAZIONI E TOOL                                                   ║
 ║     cisdump · usbinfo · LupoUsbPrefs · showregs · usblog                  ║
 ╚══════════════════════════════════╤════════════════════════════════════════╝
                                    │ API pubbliche (proto/lupousb.h, ENV: prefs)
 ╔══════════════════════════════════╧════════════════════════════════════════╗
 ║ L4  CLASS DRIVER                                                          ║
 ║     hid.class (mouse · tastiera · joypad)   hub (in L3)   msd.class       ║
 ║        │ input.device (IND_WRITEEVENT)              │ Exec device SCSI    ║
 ╚════════╪════════════════════════╤═══════════════════╪═════════════════════╝
          ▼ AmigaOS (Intuition)    │ API USBD           ▼ filesystem
 ╔═════════════════════════════════╧═════════════════════════════════════════╗
 ║ L3  USBD – lupousb.library                                                ║
 ║     enumerazione · indirizzi · configurazioni · hub driver unico ·        ║
 ║     power budget · registro/binding classi · notifiche hot-plug           ║
 ╚══════════════════════════════════╤════════════════════════════════════════╝
                                    │ devices/usbhardware.h  (IOUsbHWReq, UHCMD_*)
 ╔══════════════════════════════════╧════════════════════════════════════════╗  ┐
 ║ L2  INTERFACCIA HCD – scheletro Exec device (devskel)                     ║  │
 ║     OpenDevice/BeginIO/AbortIO · unità · task dell'unità · code · timeout ║  │ un .device
 ╚══════════════════════════════════╤════════════════════════════════════════╝  │ per chip
                                    │ submit() · abort() · rh_control() · rh_status()
 ╔══════════════════════════════════╧════════════════════════════════════════╗  │
 ║ L1  DRIVER DEL CHIP (HCD core)                                            ║  │
 ║     isp116x  |  sl811  |  ohci (legacy PCI)  |  virt (test)               ║  │
 ║     reset · scheduling · buffer interno/PTD · toggle/NAK · root hub emul. ║  │
 ╚══════════════════════════════════╤════════════════════════════════════════╝  ┘
                                    │ struct HwBus (include/lupousb/hwbus.h)
 ╔══════════════════════════════════╧════════════════════════════════════════╗
 ║ L0  ACCESSO AL BUS                                                        ║
 ║     pcmcia_gayle.c (card.resource)   |   pci_mediator.c (opzionale)       ║
 ║     slot · CIS · finestre I/O · accessi 8/16 bit · FIFO · IRQ · estrazione║
 ╚══════════════════════════════════╤════════════════════════════════════════╝
                                    │ registri Gayle, area I/O/attributo PCMCIA
                          ┌─────────┴──────────┐
                          │  HARDWARE          │  A1200 · Gayle · PC Card 16 bit
                          │                    │  chip USB PIO (+ hub 2/4 porte)
                          └────────────────────┘

 ┌───────────────────────────────────────────────────────────────────────────┐
 │ TRASVERSALE (usato da tutti i layer, non dipende da nessuno)              │
 │ src/common: endian.h · debug/log · liste · util   |   build · CI · test   │
 └───────────────────────────────────────────────────────────────────────────┘
```

Le regole sono tre:

- **dipendenze solo verso il basso**, e solo verso il layer immediatamente inferiore;
- **ogni confine è un header** stabile e versionato;
- **un cambio di hardware tocca solo L0 e L1**.

## 2. Scheda dei layer

### L0 – Accesso al bus
| Voce | Descrizione |
|---|---|
| **Scopo** | Isolare tutto ciò che dipende dallo slot o dalla busboard. Non conosce USB |
| **Responsabilità** | `OwnCard`/`ReleaseCard`, lettura del CIS e riconoscimento della scheda, abilitazione della modalità I/O (registro COR), accessi a 8/16 bit con l'ordine dei byte corretto, copia a blocchi (`read_fifo`/`write_fifo`), installazione del server di interrupt su Gayle, notifica di inserimento/estrazione, `CardAccessSpeed` |
| **Interfaccia verso l'alto** | `include/lupousb/hwbus.h` → `struct HwBus`, `struct HwBusOps` |
| **Usa** | `card.resource`, Exec (interrupt), hardware Gayle |
| **Sorgenti** | `src/bus/pcmcia_gayle.c` (primo), `src/bus/pci_mediator.c` (opzionale, legacy) |
| **Dal legacy** | Nulla per PCMCIA. `HcFindDevice` e `GfxMemory.c` solo per il backend PCI |
| **Test** | WinUAE con una scheda PCMCIA emulata qualsiasi (inserimento, estrazione, CIS). A1200 reale con la scheda USB tramite `cisdump` |
| **Fatto quando** | `cisdump` stampa il CIS della scheda USB; inserimento ed estrazione ripetuti senza blocchi; si leggono i registri ID del chip |

### L1 – Driver del chip
| Voce | Descrizione |
|---|---|
| **Scopo** | Far eseguire trasferimenti USB al chip specifico |
| **Responsabilità** | Reset e inizializzazione, gestione del buffer interno (PTD ATL/INTL per ISP116x, una transazione alla volta per SL811HS), toggle, NAK, retry, scheduling periodico per frame, interrupt del chip (ISR minima + `Signal()`), emulazione del root hub sulle porte del chip, gestione della scheda estratta |
| **Interfaccia verso l'alto** | Header interno `src/hcd/hcd_core.h`: `hcd_init`, `hcd_submit(xfer)`, `hcd_abort(xfer)`, `hcd_rh_control()`, `hcd_rh_status()`, `hcd_isr()` |
| **Usa** | Solo `struct HwBus` (L0) e `src/common` |
| **Sorgenti** | `src/hcd/isp116x/` oppure `src/hcd/sl811/`, `src/hcd/ohci/` (legacy), `src/hcd/virt/` (mock) |
| **Dal legacy** | Logica del root hub (`RootHub.c`), algoritmi di intervallo e banda (`Endpoint.c`). Con ISP116x anche la conoscenza dei registri OHCI (`Hcdriver.c`) |
| **Test** | Sull'host, con un chip finto (registri e buffer simulati in RAM); su A1200 reale |
| **Fatto quando** | Reset della porta, `GET_DESCRIPTOR` sull'indirizzo 0 e un interrupt transfer da un mouse completati sull'hardware |

### L2 – Interfaccia HCD (Exec device)
| Voce | Descrizione |
|---|---|
| **Scopo** | Esporre ogni driver L1 come Exec device standard, il confine stabile del progetto |
| **Responsabilità** | Romtag, `Init`/`Open`/`Close`/`Expunge`, `BeginIO`/`AbortIO`, unità, task dell'unità in `Wait()` (porta, segnali dall'ISR, `timer.device`), validazione e accodamento delle `IOUsbHWReq`, comandi `UHCMD_*`, statistiche |
| **Interfaccia verso l'alto** | `devices/usbhardware.h` |
| **Usa** | L1 tramite `hcd_core.h`, Exec, `timer.device` |
| **Sorgenti** | `src/hcd/devskel/`, comune a tutti; ogni chip produce un `.device` (`lupoisp116x.device`, `lupovirt.device`, …) |
| **Dal legacy** | Il dispatcher di `Usbdriver.c` come idea; il protocollo `CMD_*` sparisce |
| **Test** | Con `lupovirt.device` in emulatore; con uno stack di riferimento esistente sopra il device reale |
| **Fatto quando** | Uno stack di terze parti, oppure L3, enumera un dispositivo tramite il device |

### L3 – USBD (`lupousb.library`)
| Voce | Descrizione |
|---|---|
| **Scopo** | Gestire il bus USB in modo indipendente dall'hardware |
| **Responsabilità** | Un task per bus. Enumerazione con i tempi della specifica (debounce, reset, MPS dal byte 7, `SET_ADDRESS` + 2 ms). Tabella indirizzi, parser di descrittori e configurazioni. **Hub driver unico** (root hub ed esterni), power budget, registro e binding dei class driver, notifiche hot-plug, API sincrona e asincrona per le pipe |
| **Interfaccia verso l'alto** | `sfd/lupousb_lib.sfd` → `proto/lupousb.h` (handle opachi: `UsbDevice`, `UsbInterface`, `UsbPipe`) |
| **Usa** | L2 tramite `OpenDevice` e `IOUsbHWReq` |
| **Sorgenti** | `src/usbd/` |
| **Dal legacy** | Richieste Chapter 9 di `LupoUsbLibrary.c`, enumerazione di `UsbStack.c` (con il bug dell'MPS corretto) |
| **Test** | Sull'host (parser, macchina a stati dell'hub, enumerazione su HCD finto), in emulatore con `lupovirt.device` |
| **Fatto quando** | Un dispositivo collegato, anche dietro un hub, viene enumerato e passato al class driver giusto, e rimosso senza perdite di memoria |

### L4 – Class driver
| Voce | Descrizione |
|---|---|
| **Scopo** | Dare un significato ai dispositivi e collegarli ad AmigaOS |
| **Responsabilità** | `hid.class`: parser dei report descriptor, fallback sul boot protocol, mouse (`IECLASS_RAWMOUSE`, un evento per transizione, rotellina NewMouse), tastiera (usage HID → rawkey, LED), joypad (mapping configurabile). Poi `msd.class` (BOT + SCSI → Exec device stile `scsi.device`) |
| **Interfaccia verso l'alto** | Verso il sistema: `input.device`, Exec device a blocchi. Verso L3: entry point di binding (`AttemptBinding`/`ReleaseBinding`) |
| **Usa** | Solo L3 (API USBD) e i servizi di AmigaOS |
| **Sorgenti** | `src/classes/hid/`, `src/classes/msd/` |
| **Dal legacy** | `UsbMouse.c` come base, con le correzioni dei bug #8, #9 e #24 |
| **Test** | Parser HID sull'host con report descriptor reali; in emulatore con il mouse finto di `lupovirt.device` |
| **Fatto quando** | Un mouse e una tastiera USB qualsiasi sono usabili nel Workbench |

### L5 – Applicazioni e tool
| Voce | Descrizione |
|---|---|
| **Scopo** | Diagnostica, configurazione, informazioni all'utente |
| **Componenti** | `cisdump` (L0: primo tool da scrivere), `showregs` (L1: registri del chip e statistiche), `usbinfo` (L3: albero dei dispositivi e descrittori), `LupoUsbPrefs` (prefs testuali in `ENVARC:USB/`), `usblog` (lettura del ring buffer di log) |
| **Usa** | Le API pubbliche del layer che diagnosticano |
| **Dal legacy** | `UsbPrefs/`, `UsbShowRegs/` (interfaccia ReAction riutilizzabile) |

### Trasversale – infrastruttura
| Voce | Descrizione |
|---|---|
| `src/common` | `endian.h` (swap inline), `debug.h` (`LOG(level, …)` con `kprintf` su seriale o ring buffer, rimosso in release), liste, utilità |
| Build | GNU make, `TOOLCHAIN=gcc` o `vbcc`, `sfdc` per gli header, `make dist` → LHA |
| CI | Job gcc, vbcc e test sull'host su GitHub Actions |
| Test | `tests/host/` (unitari), `lupovirt.device` (integrazione in emulatore), A1200 reale (hardware) |

## 3. Flusso di un dato: il mouse si muove

```
 mouse USB ──(interrupt IN, ogni 10 ms)──► chip USB sulla PC Card
   L1  chip segnala "transazione completata" ─► IRQ PCMCIA ─► Gayle
   L0  server di interrupt: scheda presente? sorgente nostra? ─► chiama hcd_isr()
   L1  hcd_isr(): legge lo stato del chip, fa l'ack, Signal(task dell'unità)   [poche µs]
   L2  task dell'unità si sveglia: L1 copia i dati dal buffer del chip (read_fifo via L0),
       aggiorna il toggle, completa la IOUsbHWReq ─► ReplyMsg
   L3  (pipe interrupt aperta dal class driver) la richiesta torna al task di hid.class
   L4  hid.class: report → dx, dy, pulsanti → InputEvent IECLASS_RAWMOUSE ─► input.device
       poi rimette subito in coda la richiesta di interrupt successiva
   OS  Intuition muove il puntatore
```

## 4. Ordine di sviluppo e dipendenze

```
          ┌──────────────── Trasversale: build, CI, common, test host ────────────────┐
          ▼                                                                            │
   [L0 pcmcia_gayle + cisdump] ──► [L1 driver chip] ──► [L2 devskel] ──┐              │
                                                                       ├─► [L4 hid] ──► [L5 tool]
   [L1 virt (mock)] ──► [L2 devskel] ──► [L3 USBD + hub] ──────────────┘
         ▲ ramo "senza hardware": può partire subito, in parallelo
```

| Passo | Layer | Richiede hardware? | Dipende da |
|---|---|---|---|
| 1 | Trasversale (build, CI, common) | No | – |
| 2 | L0 `pcmcia_gayle` + `cisdump` | Parziale: WinUAE, poi A1200 + scheda | 1 |
| 3 | L2 `devskel` + L1 `virt` | No | 1 |
| 4 | L3 USBD + hub driver | No (host + `lupovirt`) | 3 |
| 5 | L4 `hid.class` | No (host + `lupovirt`) | 4 |
| 6 | L1 driver del chip reale | **Sì** | 2, 3 |
| 7 | Integrazione: L0+L1 reali sotto L2–L4 già pronti | **Sì** | 5, 6 |
| 8 | L4 `msd.class`, secondo chip, backend PCI | Sì | 7 |

Il ramo senza hardware (passi 3–5) permette di portare quasi tutto lo stack a un buon livello mentre si identifica e si procura la scheda. L'hardware serve solo per i layer L0 e L1.
