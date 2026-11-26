# LupoUsbDriver - build of the original 2001-2003 code (legacy/src) with
# m68k-amigaos-gcc + vasm instead of StormC 2.0 + Asm-One.
#
# Run from the repository root, inside the toolchain container:
#   make legacy            (top-level Makefile, uses docker)
#   make -f legacy/Makefile
#
# Needs the user's SDK copy in third_party/sdk (pci.library headers and FD
# from Elbox, not redistributable, therefore not in git and not in CI).

AMIGA_CC ?= m68k-amigaos-gcc
VASM     ?= vasmm68k_mot
# include directories searched by m68k-amigaos-gcc -noixemul (libnix, NDK, ...)
SYS_INC  ?= $(shell echo | $(AMIGA_CC) -noixemul -xc -E -v - 2>&1 | sed -n '/<...> search starts/,/End of search/p' | grep '^ /' | tr -d ' ' | tr '\n' ':')
SDK      ?= third_party/sdk

L   := legacy
SRC := $(L)/src
O   := build/legacy
GEN := $(O)/gen
INC := $(O)/inc
OUT := $(O)/bin

# The code is compiled as it was written (C89 with StormC habits): warnings
# are reported but only real errors stop the build.
CFLAGS := -m68020 -O1 -noixemul -std=gnu89 -fpermissive -fno-strict-aliasing -fcommon \
          -D__saveds= -Wno-unknown-pragmas \
          -I$(INC) -I$(L)/compat -I$(GEN) -I$(SRC)
ASFLAGS := -Fhunk -devpac -m68020 -quiet -nowarn=62

ifeq ($(wildcard $(SDK)/Include/fd/pci_lib.fd),)
$(error $(SDK) missing: copy the SDK includes (with pci.library) to third_party/sdk)
endif

PROGS := $(OUT)/C/UsbDriver $(OUT)/C/UsbStack $(OUT)/C/UsbRootHub \
         $(OUT)/C/AddUsbFunction $(OUT)/C/UsbMouse $(OUT)/C/USBShowRegs \
         $(OUT)/Prefs/LupoUsbPrefs $(OUT)/Libs/lupousb.library

.PHONY: all headers
all: $(PROGS)
	@echo "legacy build: done"; ls -l $(PROGS)

# ---------------------------------------------------------------- headers
$(GEN)/inline/pci.h: $(SDK)/Include/fd/pci_lib.fd
	@mkdir -p $(GEN)/inline
	# The Elbox FD lists ReadMemOffset() without a register list: add "()".
	sed 's/^ReadMemOffset()$$/ReadMemOffset()()/' $< > $(GEN)/pci_lib.fd
	LC_ALL=C fd2sfd -o $(GEN)/pci_lib.sfd $(GEN)/pci_lib.fd "$(SDK)/Include/clib/Pci_lib_protos.h"
	LC_ALL=C sfdc --mode=macros --target=m68k-amigaos -o $@ $(GEN)/pci_lib.sfd

$(GEN)/inline/lupousb.h: $(SRC)/LupoUsbLibrary.fd $(SRC)/Protos/LupoUsbLibrary_protos.h
	@mkdir -p $(GEN)/inline
	sed 's/__saveds//g' $(SRC)/Protos/LupoUsbLibrary_protos.h > $(GEN)/lupousb_protos.h
	LC_ALL=C fd2sfd -o $(GEN)/lupousb_lib.sfd $< $(GEN)/lupousb_protos.h 2>&1 | grep -v "is not in the FD file" || true
	LC_ALL=C sfdc --mode=macros --target=m68k-amigaos -o $@ $(GEN)/lupousb_lib.sfd

$(GEN)/functable.c: $(SRC)/LupoUsbLibrary.fd $(L)/tools/fdtable.py
	@mkdir -p $(GEN)
	python3 $(L)/tools/fdtable.py $< $@

$(INC)/.stamp: $(GEN)/inline/pci.h $(GEN)/inline/lupousb.h $(L)/tools/mkinc.py $(wildcard $(L)/compat/*.h $(L)/compat/*/*.h)
	rm -rf $(INC)
	python3 $(L)/tools/mkinc.py $(SRC) $(L)/compat $(GEN) $(INC) "$(SYS_INC)" $(SDK)/Include
	@touch $@

headers: $(INC)/.stamp

# ---------------------------------------------------------------- objects
$(O)/obj/%.o: $(SRC)/%.c $(INC)/.stamp
	@mkdir -p $(dir $@)
	$(AMIGA_CC) $(CFLAGS) -c -o $@ $< 2>> $(O)/warnings.log || { cat $(O)/warnings.log | tail -40; exit 1; }

$(O)/obj/%.o: $(SRC)/%.asm
	@mkdir -p $(dir $@)
	$(VASM) $(ASFLAGS) -o $@ $<

$(O)/obj/%.o: $(SRC)/%.Asm
	@mkdir -p $(dir $@)
	$(VASM) $(ASFLAGS) -o $@ $<

$(O)/obj/glue/%.o: $(L)/glue/%.c
	@mkdir -p $(dir $@)
	$(AMIGA_CC) -m68020 -O1 -Wall -fno-toplevel-reorder -c -o $@ $<

$(O)/obj/gen/functable.o: $(GEN)/functable.c
	@mkdir -p $(dir $@)
	$(AMIGA_CC) -m68020 -O1 -c -o $@ $<

# ---------------------------------------------------------------- programs
DRIVER_OBJ := $(addprefix $(O)/obj/Usbdriver/,Usbdriver.o Hcdriver.o Interrupt.o Endpoint.o GfxMemory.o Debugger.o) \
              $(O)/obj/Assembly/Asmhc.o $(O)/obj/Assembly/AsmEndpoint.o
LIBCODE_OBJ := $(O)/obj/LupoUsbLibrary.o $(O)/obj/LupoUsbLibraryAsm.o

$(OUT)/C/UsbDriver: $(DRIVER_OBJ)
	@mkdir -p $(dir $@)
	$(AMIGA_CC) -m68020 -noixemul -o $@ $^ -lamiga -ldebug -lm

# UsbStack links the library code statically, as in the StormC project
$(OUT)/C/UsbStack: $(O)/obj/UsbStack.o $(LIBCODE_OBJ)
	@mkdir -p $(dir $@)
	$(AMIGA_CC) -m68020 -noixemul -o $@ $^ -lamiga

$(O)/obj/glue/roothub_data.o: $(L)/glue/roothub_data.c $(INC)/.stamp
	@mkdir -p $(dir $@)
	$(AMIGA_CC) $(CFLAGS) -c -o $@ $< 2>> $(O)/warnings.log

$(OUT)/C/UsbRootHub: $(O)/obj/RootHub.o $(O)/obj/RootHubClass.o $(O)/obj/glue/roothub_data.o $(O)/obj/Assembly/Asmhc.o
	@mkdir -p $(dir $@)
	$(AMIGA_CC) -m68020 -noixemul -o $@ $^ -lamiga

$(OUT)/C/AddUsbFunction: $(O)/obj/AddUsbFunction.o
	@mkdir -p $(dir $@)
	$(AMIGA_CC) -m68020 -noixemul -o $@ $^ -lamiga

$(OUT)/C/UsbMouse: $(O)/obj/UsbMouse.o
	@mkdir -p $(dir $@)
	$(AMIGA_CC) -m68020 -noixemul -o $@ $^ -lamiga

$(OUT)/C/USBShowRegs: $(O)/obj/UsbShowRegs/USBShowRegs.o $(O)/obj/UsbShowRegs/Convert.o
	@mkdir -p $(dir $@)
	$(AMIGA_CC) -m68020 -noixemul -o $@ $^ -lamiga

$(OUT)/Prefs/LupoUsbPrefs: $(O)/obj/UsbPrefs/LupoUsbPrefs.o
	@mkdir -p $(dir $@)
	$(AMIGA_CC) -m68020 -noixemul -o $@ $^ -lamiga

# The library: skeleton first (entry point), then the original code
$(OUT)/Libs/lupousb.library: $(O)/obj/glue/libinit.o $(O)/obj/gen/functable.o $(LIBCODE_OBJ)
	@mkdir -p $(dir $@)
	$(AMIGA_CC) -m68020 -nostartfiles -nostdlib -o $@ $^ -lamiga -lgcc
