#!/usr/bin/env python3
"""Clear RB3 Gen2 WSA883x PA faults.

Run with sudo. Stop playback and other SoundWire debug tools first.
Uses stock SoundWire debugfs (kernel U taint); leaves mixer settings unchanged.
"""
import os
from pathlib import Path
import re
import sys
import tempfile

AMPS = ("sdw:1:0:0217:0202:00:1", "sdw:1:0:0217:0202:00:2")
DEBUG = Path("/sys/kernel/debug/soundwire/master-1-0")


def put(path, value):
    path.write_text(f"{value}\n")


def transfer(dev, read, count, firmware=None):
    # Column-0 commands access codec registers directly. These debugfs command
    # settings are shared across slaves, so other debug tools must stay stopped.
    for name, value in (("command", read), ("command_type", 0),
                        ("start_address", 0x3410), ("num_bytes", count)):
        put(dev / name, value)
    if firmware is not None:
        put(dev / "firmware_file", firmware)
    put(dev / "go", 1)


def state(dev):
    # Read only PA control, status, error and bypass (0x3410..0x3416).
    # A short live read avoids stale regcache values and large register dumps.
    transfer(dev, 1, 7)
    text = (dev / "read_buffer").read_text()
    pairs = re.findall(r"^address (0x[0-9a-f]+) val (0x[0-9a-f]{2})$", text, re.M)
    if len(text.splitlines()) != 7 or [int(a, 16) for a, _ in pairs] != list(range(0x3410, 0x3417)):
        raise RuntimeError(f"{dev.name}: invalid SoundWire response")
    return bytes(int(v, 16) for _, v in pairs)


def require_idle():
    # AGM owns this backend PCM; resetting while it is open could disturb an
    # active stream. This is a safety check, not protection against concurrent starts.
    if Path("/proc/asound/card0/pcm0p/sub0/status").read_text().strip() != "closed":
        raise RuntimeError("Stop agmplay before recovering the amplifiers")


def recover(dev):
    require_idle()
    before = state(dev)
    if before[0] & 1:
        raise RuntimeError(f"{dev.name}: PA is enabled; refusing reset")
    if not (before[3] & 0x70 or before[4]):
        print(f"{dev.name}: no fault")
        return
    if before[6]:
        raise RuntimeError(f"{dev.name}: unexpected protection bypass")

    # PA_FSM_CTL bit 4 clears the latched fault when pulsed low/high/low.
    # Preserve other control bits; never bypass protection or disable watchdogs.
    ctl = before[0] & ~0x10
    # The stock debug write API takes firmware-file bytes, not a register value.
    # Each temporary file supplies one byte and is removed after the sequence.
    with tempfile.TemporaryDirectory(prefix="agmrecover-", dir="/lib/firmware") as tmp:
        for i, value in enumerate((ctl, ctl | 0x10, ctl)):
            blob = Path(tmp) / f"{i}.bin"
            blob.write_bytes(bytes([value]))
            transfer(dev, 0, 1, blob.relative_to("/lib/firmware"))

    # A successful write alone does not prove that the hardware fault cleared.
    after = state(dev)
    if after[0] != ctl or after[3] & 0x70 or after[4] or after[6] != before[6]:
        raise RuntimeError(f"{dev.name}: recovery failed (registers: {after.hex(' ')})")
    print(f"{dev.name}: fault cleared")


def main():
    if os.geteuid() != 0:
        raise RuntimeError("Run with sudo")
    require_idle()
    # Resume the macro, controller and amps before touching registers, and keep
    # them awake throughout recovery. Driver resume restores regcache normally.
    devices = [Path("/sys/devices/platform/soc@0") / name
               for name in ("3240000.codec", "3250000.soundwire")]
    devices += [Path("/sys/bus/soundwire/devices") / amp for amp in AMPS]
    for dev in devices:
        put(dev / "power/control", "on")
    for amp in AMPS:
        recover(DEBUG / amp)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError) as exc:
        sys.exit(f"agmrecover: {exc}")
