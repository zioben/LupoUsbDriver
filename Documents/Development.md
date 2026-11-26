# LupoUsbDriver – Guida allo sviluppo (stack virtuale, fase 1)

*Novembre 2026. Complemento a `ProjectReview.md` (architettura e roadmap) e a `Layers.md` (schema dei layer).*

## 1. Stato

L'hardware reale non è ancora disponibile: le due schede PCMCIA in possesso sono CardBus e la scheda CF con SL811HS non si trova. Per questo **L0 e L1 girano su un chip USB simulato**, mentre **L2, L3 e L4 sono codice reale**: quando arriverà l'hardware si sostituiranno solo L0 e L1.

| Layer | Componente | File | Stato |
|---|---|---|---|
| L0 | Bus virtuale verso il chip simulato | `src/bus/virtbus.c`, `include/lupousb/hwbus.h` | Fatto, testato |
| (sim) | Chip **LVHC** simulato + hub, mouse, tastiera e gamepad virtuali | `src/sim/`, `include/lupousb/lvhc_regs.h`, `include/lupousb/vsim.h` | Fatto, testato |
| L1 | Driver del chip LVHC (motore delle transazioni, toggle, NAK/retry, scheduling interrupt) | `src/hcd/lvhc/lvhc.c` | Fatto, testato |
| L1 | Emulazione generica del root hub (riusabile per SL811/MAX3421/CH376) | `src/hcd/rhemul/rhemul.c` | Fatto, testato |
| L2 | `lupovirt.device`: Exec device con interfaccia `usbhardware.device` | `src/amiga/lupovirt_dev.c` | Fatto, provato in emulatore (FS-UAE + ROM AROS, vedi §4bis) |
| L3 | USBD: enumerazione, driver hub unico, binding delle classi | `src/usbd/` | Fatto, testato (host e 68k) |
| L3 | Backend USBD verso un `usbhardware.device` qualsiasi | `src/amiga/be_amiga.c` | Compila; da provare su AmigaOS |
| L4 | HID: parser dei report descriptor, decoder, mappa HID→rawkey, class driver | `src/classes/hid/` | Fatto, testato |
| L4 | Iniezione in `input.device` (mouse, tasti, rotellina) | `src/amiga/hidsink_input.c` | Compila; da provare su AmigaOS |
| L5 | `lupousbd` (demone L3+L4), `usbsim` (simulatore host) | `src/amiga/lupousbd.c`, `tools/usbsim/` | `usbsim` funziona; `lupousbd` compila |

## 2. Il chip simulato LVHC

LVHC ("Lupo Virtual Host Controller") è un chip **inventato ma realistico**, con la mappa dei registri in `include/lupousb/lvhc_regs.h`. È modellato sulla famiglia di chip adatti ai bus a 8/16 bit dell'Amiga (SL811HS, MAX3421E, CH376 in modalità raw):
- registri a 8 bit e FIFO dei pacchetti da 64 byte;
- **un solo motore seriale** che esegue una transazione alla volta (SETUP, IN, OUT);
- porte root senza funzione hub (il root hub è emulato in L1).

Il driver L1 per LVHC ha quindi la stessa struttura del futuro driver per il chip reale.

Il simulatore riproduce i comportamenti che contano per lo stack:
- macchina a stati del controllo (stadi SETUP, DATA, STATUS), `SET_ADDRESS` applicato dopo lo status;
- data toggle con rilevamento dei duplicati, endpoint halt e STALL;
- NAK quando un dispositivo HID non ha dati nuovi;
- reset delle porte con temporizzazione (10 ms), hub esterni con instradamento per indirizzo;
- dispositivi low speed: una transazione senza il flag low speed va in timeout;
- inserimento ed estrazione a caldo, ed estrazione dell'intera scheda (`VirtBus.removed`).

Il tempo è un clock in millisecondi (`os_time_ms()`): sull'host è simulato, quindi i test sono deterministici; su Amiga viene da `timer.device`.

## 3. Comandi

| Comando | Cosa fa |
|---|---|
| `make test` | Compila e lancia i test sull'host (gcc con ASan/UBSan): 54 controlli, compresa l'assenza di leak |
| `make sim && ./build/host/usbsim [-v]` | Simulatore: mostra enumerazione, albero dei dispositivi ed eventi di input decodificati |
| `make docker-amiga` | Compila i binari Amiga nel container della toolchain (bebbo gcc 16 + NDK 3.2) |
| `make dist` (nel container) | Crea `build/dist/LupoUsbDriver.lha` |
| `tests/emu/run.sh [secondi]` | Avvia un A1200 emulato (FS-UAE, ROM AROS) che esegue `lupousbd` su `lupovirt.device` e dà un verdetto PASS/FAIL (§4bis) |
| `vamos -C 68020 build/amiga/C/lupotest` | Esegue la stessa suite di test **compilata per 68k** sotto vamos (amitools): verifica il big-endian |

Binari Amiga prodotti:
- `Devs/lupovirt.device`: da copiare in `DEVS:`. L'unità 0 ha i dispositivi fermi; l'unità 1 è in modalità demo, con il mouse virtuale che percorre un quadrato.
- `C/lupousbd`: `lupousbd [DEVICE=nome.device] [UNIT=n] [VERBOSE] [NOINPUT] [TIMEOUT=s] [LOG=file]`. `TIMEOUT` esce dopo n secondi (stampando prima l'albero), `LOG` scrive il log anche su file, con il tempo in ms. Ctrl-F stampa l'albero dei dispositivi, Ctrl-C esce. Funziona con qualunque `usbhardware.device`, non solo con `lupovirt.device`.
- `C/lupotest`: la suite di test per 68k.

## 4. CI

`.github/workflows/build.yml` esegue a ogni push cinque job:
1. **test sull'host**;
2. **build Amiga** nel container, fissato per digest, con l'archivio LHA e i binari come artifact;
3. **test 68k sotto vamos**;
4. **Amiga emulato** (FS-UAE + ROM AROS): `tests/emu/run.sh`, con i log come artifact `emulator-logs`;
5. **release**: sui tag `v*` allega l'LHA alla release.

## 4bis. Test in emulatore

`tests/emu/run.sh` prepara una directory usata come hard disk (`build/emu/hd`, con `C/lupousbd`, `C/mousepos`, `Devs/lupovirt.device`) e avvia FS-UAE senza finestra (Xvfb) con la ROM AROS integrata, quindi senza Kickstart protetto da copyright. La Startup-Sequence esegue tre prove:

| Prova | Comando | Controllo |
|---|---|---|
| 1-idle | `lupousbd UNIT=0 TIMEOUT=5 NOINPUT` | 5 dispositivi enumerati e legati al driver, chiusura pulita |
| 2-demo | `lupousbd UNIT=1 TIMEOUT=5 NOINPUT` | come sopra, con il traffico interrupt del mouse demo |
| 3-input | `lupousbd UNIT=1 TIMEOUT=8` | come sopra, e il puntatore di Intuition (letto da `mousepos` prima e dopo) si è spostato |

La porta seriale emulata va in `build/emu/serial.log` (output di debug del device). L'emulazione è *cycle exact* a velocità reale di un 68020/A1200: FS-UAE senza questi parametri esegue la CPU alla velocità dell'host e nasconde i problemi di prestazioni.

Problemi trovati solo così (corretti):
- **livelock**: il chip simulato rialzava l'interrupt a ogni accesso finché restavano bit di cambiamento delle porte; il task dell'unità (priorità 5) monopolizzava la CPU e `lupousbd` non avanzava. Ora l'interrupt scatta solo sui bit nuovi (fronte), come su un chip reale;
- **CPU insufficiente in modalità demo**: l'orologio veniva letto con `GetSysTime()` a ogni accesso ai registri, con tick di 2 ms. Ora c'è un orologio manuale avanzato una volta per ciclo (`virtbus_tick()`), con tick di 5 ms.

Esito (locale e su GitHub): tutte le prove PASS, enumerazione completa in circa 2,4 s di tempo emulato, il mouse USB sposta il puntatore.

Le bozze `Documents/proposals/Makefile.draft` e `build.yml.draft` sono superate da `Makefile` e dal workflow reale.

## 5. Regole di progetto emerse

- **Uno `UsbXfer` resta `XS_PENDING` finché la callback `done` non è stata chiamata.** L'HCD tiene il risultato in `hcd_status` e lo copia in `status` solo subito prima della callback. ASan aveva trovato un uso dopo il ritorno nei trasferimenti sincroni quando questa regola non c'era.
- **Le callback di completamento non bloccano**: niente trasferimenti sincroni né ritardi. Registrano l'evento; il lavoro lento (debounce, reset delle porte, enumerazione) si fa in `usbd_process()`.
- **Il root hub è un hub come gli altri**: si presenta all'indirizzo 0 dopo il bus reset (come richiede il modello `usbhardware.device`) e viene gestito dallo stesso driver hub degli hub esterni.
- **Il gestore di interrupt (L0→L1) si limita a segnalare**: sul device Amiga fa solo un `Signal()` al task dell'unità.
- Gli header `devices/usbhardware.h` e `devices/usb.h` (© Chris Hodges, "All Rights Reserved") vengono **usati dalla toolchain, non copiati** nel repository. Il core portabile non ne dipende: i valori di `XS_*`/`XF_*` coincidono numericamente con `UHIOERR_*`/`UHFF_*`, e solo L2 e il backend Amiga includono l'header.
- I commenti del nuovo codice sono in inglese, la documentazione in italiano.

## 6. Prossimi passi

1. ~~Provare `lupovirt.device` e `lupousbd` su AmigaOS~~: fatto in emulatore con ROM AROS (§4bis). Resta da provare con Kickstart 3.x originale (ROM non distribuibile, quindi a mano) e su hardware.
2. **Joystick/gamepad**: oggi il decoder funziona ma il sink Amiga si limita a registrare nel log. Va deciso come presentarlo al sistema (patch di `lowlevel.library`, emulazione della porta joystick, API propria).
3. **Impacchettare L3 come `lupousb.library`** con class driver caricati dinamicamente (registro delle classi in `LIBS:USB/` o `SYS:Classes/USB/`).
4. **Trasferimenti bulk** già supportati da L1 e L3; poi `msd.class` (mass storage) con un dispositivo di memoria simulato.
5. **Hardware reale:** scegliere il chip (MAX3421E o CH376 su clockport, vedi `ProjectReview.md`) e scrivere un nuovo L0 e un nuovo L1, riusando `rhemul.c` e i test esistenti come riferimento.
6. **Include dell'SDK:** ricevuti (novembre 2026) ed estratti in `third_party/sdk/` (escluso da git, non va mai committato). Contengono:
   - gli include C e assembly di AmigaOS 3.1 (exec 40.x) nella versione distribuita con StormC, con FD, `proto`, `pragma`, `inline` e ReAction;
   - **gli header di `pci.library` della Mediator** (`libraries/pci.h`, `clib/Pci_lib_protos.h`, `fd/pci_lib.fd`, © Elbox, "All Rights Reserved"). Erano l'unica dipendenza mancante per ricostruire la build del codice legacy (fase F1) e per un eventuale backend L0 PCI;
   - `resources/card.h` e `cardres_lib.fd` per `card.resource`, già presenti anche nell'NDK 3.2 del container.

   Non contengono i ROM Kernel Manual né i libri: in `Manuals/` ci sono solo le guide "Changes 3.0".
   In `third_party/docs/` ci sono inoltre (sempre fuori da git) il **RKRM DOS** (PDF) e gli **esempi del RKRM Devices** (`rkrm_devices_examples/`).

   Confronto di `lupovirt.device` con lo scheletro ufficiale `sampledevice/ramdev.device.asm`:
   - **allineati:** Open incrementa `lib_OpenCnt` prima di qualunque attesa e marca la richiesta `NT_REPLYMSG`; BeginIO imposta `NT_MESSAGE` e azzera `IOF_QUICK` quando accoda; Close mette `io_Unit`/`io_Device` a -1 ed esegue l'expunge differito; Expunge non attende mai (viene chiamato dall'allocatore di memoria).
   - **differenza consapevole:** alla chiusura dell'ultima unità Close aspetta, con un segnale, la terminazione del task dell'unità. ramdev invece fa `RemTask` diretto. Attendere in Close interrompe temporaneamente il Forbid di `CloseDevice`; è una pratica diffusa, ma va tenuta d'occhio nelle prove su AmigaOS.
   - `input/Set_Mouse.c` conferma l'uso di `IND_WRITEEVENT` fatto in `hidsink_input.c`. `gameport/Absolute_Joystick.c` è il riferimento per decidere come presentare i joystick USB.
   La build del nuovo stack continua a usare l'NDK 3.2 del container (necessario in CI); gli include 3.1 servono come riferimento e per il codice legacy.

## 7. Licenza degli header `usbhardware.h` / `usb.h` (verificata il 26.11.2026)

- **Versione nella toolchain** (`amigadev/crosstools`): `usbhardware.h` 2.1 (14.02.07), "(C) 2002-2007 Chris Hodges, All Rights Reserved", senza alcuna licenza. È quella usata oggi dalla build, e **non va copiata** nel repository.
- **Versione AROS** (`aros-development-team/AROS`, `compiler/include/devices/usbhardware.h` 3.3 e `usb.h` 3.2): l'intestazione dice ancora "All Rights Reserved", ma nello stesso repository il file `rom/usb/LEGAL` contiene la dichiarazione di Chris Hodges del 26.08.2009: *"I, Chris Hodges, hereby provide my Poseidon sources and related files in the AROS Repository under the terms of the AROS Public License."* Il file `LEGAL` alla radice di AROS non elenca eccezioni per questi header. Quindi **le copie AROS sono distribuite con licenza APL 1.1**; la dicitura "All Rights Reserved" è un residuo storico.
- **APL 1.1** (derivata dalla MPL 1.1) è un copyleft **a livello di file**:
  - un file APL può stare in un progetto MIT come parte di una "Larger Work";
  - quel file resta APL, con le sue note di copyright e il testo della licenza accanto;
  - le modifiche a quel file vanno rese disponibili come sorgente.

  Il nostro codice resta MIT. (Non è una consulenza legale.)
- **Differenze tecniche** tra le due versioni:
  - nella versione AROS `struct IOUsbHWReq` è la **V3** (campi USB3/xHCI in coda), mentre `IOUsbHWReqV2` corrisponde alla struttura della toolchain;
  - non c'è più `#pragma pack(2)`, ininfluente su m68k dove l'allineamento è già a 2 byte;
  - i valori di `UHCMD_*`, `UHIOERR_*`, `UHFF_*` e `UHA_*` sono gli stessi, con alcune aggiunte (`UHIOERR_BABBLE` = 13, nuovi flag e comandi ISO).

  Se si adotta la versione AROS, `lupovirt.device` deve controllare `mn_Length >= sizeof(struct IOUsbHWReqV2)` e non `sizeof(struct IOUsbHWReq)`, per continuare ad accettare client Poseidon V2.

**Scelta adottata (26.11.2026): header indipendente.** `include/lupousb/hwreq.h` (MIT) definisce `struct LuHwReq` e le costanti `LUHCMD_*`, `LUHIOERR_*`, `LUHDIR_*`, `LUHF_*`, `LUHS_*` e `LUHA_*`, con nomi nostri e **la stessa disposizione binaria della V2** (90 byte; 76 per la V1). È stata ricavata dall'interfaccia pubblica a fini di interoperabilità, senza copiarne il testo.
- `tests/abi/hwreq_abi.c` include sia `hwreq.h` sia il `devices/usbhardware.h` della toolchain e controlla con `_Static_assert` dimensioni, offset di tutti i campi e valori di tutte le costanti. Controlla anche che i codici `XS_*`, `XF_*` e `XD_*` del core portabile coincidano.
- `make abi-check` (incluso in `make amiga`, quindi nella CI) compila il controllo e verifica che nessun sorgente in `src/` o `include/` includa gli header di terzi.
- Una prova negativa con un campo alterato fa fallire la build, indicando gli offset discordanti.
- `lupovirt.device` accetta richieste di almeno `LUHWREQ_SIZE_V2` byte, perché usa `lhr_DriverPrivate1/2`.

Gli header AROS (APL) restano un'alternativa possibile; con l'header indipendente non servono.
