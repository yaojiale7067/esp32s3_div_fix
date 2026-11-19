"""Read a few seconds of serial output (boot-log helper).

Usage: python tools/serial_probe.py COM5 [baud] [seconds] [noreset]
  noreset - do not pulse DTR; just listen to whatever the board is doing.
"""
import sys
import time

import serial

port = sys.argv[1] if len(sys.argv) > 1 else "COM5"
baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
seconds = float(sys.argv[3]) if len(sys.argv) > 3 else 8.0
do_reset = not (len(sys.argv) > 4 and sys.argv[4].lower() == "noreset")

ser = serial.Serial(port, baud, timeout=1)
time.sleep(0.3)
ser.reset_input_buffer()
if do_reset:
    # DTR -> EN: pulse low then release to force a real reset.
    ser.setDTR(True)
    time.sleep(0.15)
    ser.setDTR(False)

chunks = []
deadline = time.time() + seconds
while time.time() < deadline:
    chunks.append(ser.read(4096))
ser.close()

sys.stdout.write(b"".join(chunks).decode("utf-8", "replace"))
