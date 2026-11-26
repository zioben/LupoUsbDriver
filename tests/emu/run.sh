#!/bin/sh
# LupoUsbDriver - integration test in an emulated Amiga (FS-UAE + AROS ROM).
#
# Boots an A1200 with the AROS Kickstart replacement built into FS-UAE (no
# copyrighted ROM needed) from a host directory used as hard disk, runs
#   mousepos; lupousbd UNIT=1 TIMEOUT=n; mousepos
# and prints the log. Unit 1 of lupovirt.device moves the virtual mouse, so
# the pointer position must change if USB events reach input.device.
#
# Needs: fs-uae, xvfb-run, and the Amiga binaries (make docker-amiga).
# usage: tests/emu/run.sh [seconds of emulation]
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
EMU=$ROOT/build/emu
HD=$EMU/hd
SECS=${1:-90}

rm -rf "$EMU"
mkdir -p "$HD/C" "$HD/Devs" "$HD/S"
cp "$ROOT/build/amiga/Devs/lupovirt.device" "$HD/Devs/"
cp "$ROOT/build/amiga/C/lupousbd" "$ROOT/build/amiga/C/mousepos" "$HD/C/"

cat > "$HD/S/Startup-Sequence" <<'EOF'
C:mousepos before >SYS:emu.log
C:lupousbd UNIT=0 TIMEOUT=5 NOINPUT LOG=SYS:emu-1-idle.log
C:lupousbd UNIT=1 TIMEOUT=5 NOINPUT LOG=SYS:emu-2-demo.log
C:lupousbd UNIT=1 TIMEOUT=8 LOG=SYS:emu-3-input.log
C:mousepos after >>SYS:emu.log
EOF

cat > "$EMU/test.fs-uae" <<EOF
[fs-uae]
amiga_model = A1200
fast_memory = 8192
hard_drive_0 = $HD
hard_drive_0_label = LupoTest
fullscreen = 0
automatic_input_grab = 0
initial_input_grab = 0
video_sync = 0
serial_port = $EMU/ttyAmiga
log_autoscale = 0
EOF

cd "$EMU"
# Amiga serial port -> pseudo terminal -> serial.log (debug output of the
# device: KPutStr). FS-UAE needs a tty, not a plain file.
socat -u pty,raw,echo=0,link="$EMU/ttyAmiga" "OPEN:$EMU/serial.log,creat,append" &
SOCAT=$!
sleep 1
# The emulator runs until killed: give it time to boot and run the test.
HOME=$EMU xvfb-run -a timeout "$SECS" fs-uae "$EMU/test.fs-uae" >"$EMU/fs-uae.out" 2>&1 || true

kill $SOCAT 2>/dev/null || true
[ -f "$HD/emu.log" ] || { echo "no log: the test did not run"; tail -20 "$EMU/fs-uae.out"; exit 1; }
for f in "$HD"/emu*.log; do
    echo "=== $(basename "$f") (Amiga side) ==="
    cat "$f"
done
echo "=== serial (device debug output, after boot) ==="
tr -d '\r' < "$EMU/serial.log" 2>/dev/null | sed -n '/lupousb/,$p' | head -40 || true
