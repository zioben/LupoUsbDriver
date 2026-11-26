# LupoUsbDriver - build
#
#   make test          host unit/integration tests (gcc, ASan/UBSan)
#   make sim           host simulator demo (build/host/usbsim)
#   make amiga         AmigaOS binaries (needs m68k-amigaos-gcc in PATH)
#   make docker-amiga  same as 'amiga', inside the toolchain container
#   make dist          LHA archive of the Amiga binaries
#   make legacy        the original 2001-2003 code (legacy/), in the container;
#                      needs the SDK with pci.library headers in third_party/sdk
#   make clean
#
# Sources/ keeps the untouched 2001-2003 originals; legacy/ is their gcc port.

HOST_CC   ?= cc
AMIGA_CC  ?= m68k-amigaos-gcc
LHA       ?= lha
TOOLCHAIN_IMAGE ?= amigadev/crosstools:m68k-amigaos@sha256:c745917130aa76b62b069d9d42a2fce8bd91d1a7d580dde778fa77c6c5f38f95

B  := build
BH := $(B)/host
BA := $(B)/amiga

COMMON_SRC := src/common/util.c
SIM_SRC    := src/sim/vdev.c src/sim/vchip.c src/sim/vdev_hub.c src/sim/vdev_hid.c src/bus/virtbus.c
HCD_SRC    := src/hcd/rhemul/rhemul.c src/hcd/lvhc/lvhc.c
USBD_SRC   := src/usbd/usbd.c src/usbd/hub.c
HID_SRC    := src/classes/hid/hidparse.c src/classes/hid/hiddecode.c src/classes/hid/hiddrv.c
STACK_SRC  := $(COMMON_SRC) $(SIM_SRC) $(HCD_SRC) $(USBD_SRC) $(HID_SRC) src/usbd/be_direct.c

HEADERS    := $(wildcard include/lupousb/*.h src/amiga/*.h)

HOST_CFLAGS  := -std=c99 -g -O1 -Wall -Wextra -Werror -Iinclude
HOST_SAN     := -fsanitize=address,undefined -fno-omit-frame-pointer
AMIGA_CFLAGS := -m68020 -Os -fomit-frame-pointer -Wall -Wextra -Werror -Iinclude -noixemul
# Devices/libraries: no C library, no startup code, first function = entry point
AMIGA_MODULE := -nostartfiles -nostdlib -fno-toplevel-reorder -fno-builtin

.PHONY: all test sim amiga abi-check docker-amiga dist legacy clean

all: test

# ---------------------------------------------------------------- host
$(BH)/tests: tests/host/test_main.c $(STACK_SRC) src/common/os_host.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_SAN) -o $@ tests/host/test_main.c $(STACK_SRC) src/common/os_host.c

test: $(BH)/tests
	$(BH)/tests

$(BH)/usbsim: tools/usbsim/usbsim.c $(STACK_SRC) src/common/os_host.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(HOST_CC) $(HOST_CFLAGS) -o $@ tools/usbsim/usbsim.c $(STACK_SRC) src/common/os_host.c

sim: $(BH)/usbsim

# --------------------------------------------------------------- amiga
VIRT_DEV_SRC := src/amiga/lupovirt_dev.c src/amiga/os_amiga.c $(COMMON_SRC) $(SIM_SRC) $(HCD_SRC)
USBD_APP_SRC := src/amiga/lupousbd.c src/amiga/be_amiga.c src/amiga/hidsink_input.c \
                src/amiga/os_amiga.c $(COMMON_SRC) $(USBD_SRC) $(HID_SRC)

$(BA)/Devs/lupovirt.device: $(VIRT_DEV_SRC) $(HEADERS)
	@mkdir -p $(dir $@)
	$(AMIGA_CC) $(AMIGA_CFLAGS) $(AMIGA_MODULE) -o $@ $(VIRT_DEV_SRC) -lamiga -ldebug -lgcc

$(BA)/C/lupousbd: $(USBD_APP_SRC) $(HEADERS)
	@mkdir -p $(dir $@)
	$(AMIGA_CC) $(AMIGA_CFLAGS) -o $@ $(USBD_APP_SRC) -ldebug

# The host test suite rebuilt for 68k (big endian): runs under an
# AmigaOS CLI emulator such as vamos, or on a real Amiga.
$(BA)/C/lupotest: tests/host/test_main.c $(STACK_SRC) src/common/os_host.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(AMIGA_CC) $(AMIGA_CFLAGS) -Wno-format -o $@ tests/host/test_main.c $(STACK_SRC) src/common/os_host.c

# ABI check: our MIT licensed include/lupousb/hwreq.h must be binary
# compatible with the toolchain's devices/usbhardware.h (compile-time
# asserts), and no shipped source may include the third-party headers.
abi-check:
	$(AMIGA_CC) $(AMIGA_CFLAGS) -std=gnu11 -fsyntax-only tests/abi/hwreq_abi.c
	@if grep -rnE '#include *[<"]devices/(usbhardware|usb)\.h' src include; then \
	    echo "error: third-party usbhardware/usb headers included outside tests/abi"; exit 1; fi
	@echo "ABI check passed: hwreq.h matches usbhardware.device V2"

amiga: abi-check $(BA)/Devs/lupovirt.device $(BA)/C/lupousbd $(BA)/C/lupotest

docker-amiga:
	docker run --rm -v "$(CURDIR)":/work -w /work $(TOOLCHAIN_IMAGE) make amiga

legacy:
	docker run --rm -v "$(CURDIR)":/work -w /work $(TOOLCHAIN_IMAGE) make -f legacy/Makefile

dist: amiga
	@mkdir -p $(B)/dist
	cp README.md $(BA)/LupoUsbDriver.readme
	cd $(BA) && $(LHA) -aq2 ../dist/LupoUsbDriver.lha Devs C LupoUsbDriver.readme

clean:
	rm -rf $(B)
