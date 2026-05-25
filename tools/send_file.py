#!/usr/bin/env python3
"""
MonkeyMetal WiFi File Sender

Sends files to the ESP32-S3 over WiFi TCP (port 9999).
The console must be connected to the same WiFi network.

Usage:
    python send_file.py <ip> <local_path> <remote_path>

Example:
    python send_file.py 192.168.1.100 render.lua /games/iron_heart/src/render.lua
    python send_file.py 192.168.1.100 player.lua /games/iron_heart/src/player.lua

To send an entire directory:
    python send_file.py 192.168.1.100 --dir sdcard_root/games/iron_heart/src /games/iron_heart/src
"""

import socket
import sys
import os
import time

PORT = 9999

def send_file(ip, local_path, remote_path):
    """Send a single file to the console."""
    if not os.path.exists(local_path):
        print(f"ERROR: {local_path} not found")
        return False

    with open(local_path, 'rb') as f:
        data = f.read()

    print(f"Sending {local_path} ({len(data)} bytes) → {remote_path}")

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(10)

    try:
        sock.connect((ip, PORT))

        # Protocol: <path>\n<content>
        sock.sendall(remote_path.encode() + b'\n')
        sock.sendall(data)

        # Shutdown write to signal EOF
        sock.shutdown(socket.SHUT_WR)

        # Read response
        resp = sock.recv(1024)
        resp_str = resp.decode().strip()
        print(f"  → {resp_str}")
        return resp_str == "OK"

    except socket.timeout:
        print(f"  → TIMEOUT")
        return False
    except ConnectionRefusedError:
        print(f"  → Connection refused (is the console on WiFi?)")
        return False
    except Exception as e:
        print(f"  → ERROR: {e}")
        return False
    finally:
        sock.close()


def send_directory(ip, local_dir, remote_dir):
    """Send all files in a directory tree."""
    success = 0
    fail = 0
    for root, dirs, files in os.walk(local_dir):
        for name in files:
            local_path = os.path.join(root, name)
            rel_path = os.path.relpath(local_path, local_dir).replace('\\', '/')
            remote_path = remote_dir.rstrip('/') + '/' + rel_path

            if send_file(ip, local_path, remote_path):
                success += 1
            else:
                fail += 1
            time.sleep(0.1)  # Small delay between files

    print(f"\nDone: {success} OK, {fail} failed")


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        sys.exit(1)

    ip = sys.argv[1]

    if sys.argv[2] == '--dir':
        local_dir = sys.argv[3]
        remote_dir = sys.argv[4] if len(sys.argv) > 4 else '/'
        send_directory(ip, local_dir, remote_dir)
    else:
        local_path = sys.argv[2]
        remote_path = sys.argv[3]
        send_file(ip, local_path, remote_path)


if __name__ == '__main__':
    main()
