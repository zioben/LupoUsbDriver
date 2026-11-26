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
# CI artifacts lose the x bit; the directory hard disk maps it to the
# Amiga 'e' protection flag
chmod +x "$HD/C/"* "$HD/Devs/"*

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
# The emulator never exits by itself: stop it when the last command of the
# Startup-Sequence has written its line, or after SECS seconds.
HOME=$EMU xvfb-run -a fs-uae "$EMU/test.fs-uae" >"$EMU/fs-uae.out" 2>&1 &
EMUPID=$!
t=0
while [ $t -lt "$SECS" ] && ! grep -q "mousepos after" "$HD/emu.log" 2>/dev/null; do
    sleep 2; t=$((t + 2))
done
sleep 2
pkill -f "fs-uae $EMU/test.fs-uae" 2>/dev/null || true
wait $EMUPID 2>/dev/null || true
kill $SOCAT 2>/dev/null || true
echo "emulator stopped after ${t}s"

[ -f "$HD/emu.log" ] || { echo "no log: the test did not run"; tail -20 "$EMU/fs-uae.out"; exit 1; }
for f in "$HD"/emu*.log; do
    echo "=== $(basename "$f") (Amiga side) ==="
    cat "$f"
done
echo "=== serial (device debug output, after boot) ==="
tr -d '\r' < "$EMU/serial.log" 2>/dev/null | sed -n '/lupousb/,$p' | grep -v TaskRes | head -20 || true

# ---------------------------------------------------------------- verdict
fail=0
check() { if eval "$2"; then echo "PASS: $1"; else echo "FAIL: $1"; fail=1; fi; }
for run in 1-idle 2-demo 3-input; do
    f="$HD/emu-$run.log"
    check "$run: 5 devices enumerated and bound" \
        "[ \$(grep -c 'bound to' '$f' 2>/dev/null) -eq 5 ]"
    check "$run: clean shutdown" "grep -q 'device addr 1 removed' '$f' 2>/dev/null"
done
before=$(sed -n 's/^mousepos before: //p' "$HD/emu.log")
after=$(sed -n 's/^mousepos after: //p' "$HD/emu.log")
check "USB mouse moved the Intuition pointer ($before -> $after)" \
    "[ -n '$after' ] && [ '$before' != '$after' ]"
exit $fail
