#!/usr/bin/env python3
"""
udp_pad.py - PC keyboard -> ESP32 UDP keypad bridge for gbemu.

Captures key presses on this machine and sends 2-byte UDP packets to the
ESP32 board running gbemu's udp_pad service.

Wire format (matches components/udp_pad/udp_pad.h):
    byte 0 = button id  (1..8, see KEYMAP below)
    byte 1 = 0 release / 1 press

Default keyboard map:
    Arrow keys   -> D-Pad
    Z            -> A
    X            -> B
    Enter        -> START
    Backspace    -> SELECT
    Esc / Ctrl+C -> quit

Requires: pip install pynput
"""

import argparse
import socket
import sys

try:
    from pynput import keyboard
except ImportError:
    sys.exit("pip install pynput   # required")

# Mirror of components/udp_pad/udp_pad.h
PAD = {
    "UP": 1, "DOWN": 2, "LEFT": 3, "RIGHT": 4,
    "A": 5, "B": 6, "START": 7, "SELECT": 8,
}

KEYMAP = {
    keyboard.Key.up:        PAD["UP"],
    keyboard.Key.down:      PAD["DOWN"],
    keyboard.Key.left:      PAD["LEFT"],
    keyboard.Key.right:     PAD["RIGHT"],
    keyboard.Key.enter:     PAD["START"],
    keyboard.Key.backspace: PAD["SELECT"],
}

CHAR_KEYMAP = {
    "z": PAD["A"],
    "x": PAD["B"],
}


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("host", help="ESP32 IP address, e.g. 192.168.8.215")
    ap.add_argument("-p", "--port", type=int, default=8888)
    args = ap.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    print(f"udp_pad -> {args.host}:{args.port}")
    print("Arrows = D-pad   Z=A   X=B   Enter=START   Backspace=SELECT")
    print("Esc to quit\n")

    held: set[int] = set()  # buttons currently down, to skip OS auto-repeat

    def resolve(key) -> int | None:
        if key in KEYMAP:
            return KEYMAP[key]
        if hasattr(key, "char") and key.char:
            return CHAR_KEYMAP.get(key.char.lower())
        return None

    def send(btn: int, pressed: bool) -> None:
        sock.sendto(bytes([btn, 1 if pressed else 0]), (args.host, args.port))
        print(f"{'DOWN' if pressed else ' UP '} btn={btn}")

    def on_press(key) -> bool | None:
        if key == keyboard.Key.esc:
            print("\nbye")
            return False  # stop listener
        btn = resolve(key)
        if btn and btn not in held:
            held.add(btn)
            send(btn, True)
        return None

    def on_release(key) -> None:
        btn = resolve(key)
        if btn and btn in held:
            held.discard(btn)
            send(btn, False)

    with keyboard.Listener(on_press=on_press, on_release=on_release) as lst:
        lst.join()


if __name__ == "__main__":
    main()
