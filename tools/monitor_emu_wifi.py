#!/usr/bin/env python
import socket
import string
import struct
import textwrap

def hexdump(data: bytes):
    return " ".join(["{:02X}".format(b) for b in data])

def pretty_macstr(mac: bytes):
    """Converts a 12-byte MAC into a pretty string representation"""
    return ':'.join((mac.decode("ASCII")[i:i+2] for i in range(0, 12, 2)))

def pretty_macbin(mac: bytes):
    """Returns the """
    return ':'.join((f"{b:02X}" for b in mac))

# The ESPNOW header, before the MAC string
ESPNOW_HEADER = b"ESP_NOW-"

# Message types from p2pConnection.h
MSG_CONNECT  = 0x00
MSG_START    = 0x01
MSG_ACK      = 0x02
MSG_DATA_ACK = 0x03
MSG_DATA     = 0x04

# The types mapped onto strings for convenience
MSG_TYPE_NAMES = {
    MSG_CONNECT:  "CONNECT",
    MSG_START:    "START",
    MSG_ACK:      "ACK",
    MSG_DATA_ACK: "ACK+D",
    MSG_DATA:     "DATA",
}

INDENT = " "*32

def dbg(*args, **kwargs):
    if debug:
        print(*args, **kwargs)

def handle_msg(addr, message):
    if message.startswith(ESPNOW_HEADER):
        (from_mac,) = struct.unpack_from("!12s", message, len(ESPNOW_HEADER))

        line = f"{pretty_macstr(from_mac)} > "

        rest = message[len(ESPNOW_HEADER) + struct.calcsize("!12s") + 1:]

        if rest[0] == ord('p'):
            # P2P message
            line += "P2P "

            mode_id, msg_type = struct.unpack_from("!BB", rest, 1)

            type_str = MSG_TYPE_NAMES.get(msg_type, f"{msg_type:02X}")

            # Print the mode as a char if it's printable, or as hex otherwise
            if chr(mode_id) in string.printable:
                line += f"{chr(mode_id):2} "
            else:
                line += f"{mode_id:02X} "

            line += f"{type_str:<5}"

            # These all have a seqnum and dest mac
            if msg_type in (MSG_START, MSG_ACK, MSG_DATA, MSG_DATA_ACK):
                seqnum, to_mac = struct.unpack_from("!B6s", rest, 3)
                line += f" #{seqnum:03d} > {pretty_macbin(to_mac)}"

                if msg_type in (MSG_DATA, MSG_DATA_ACK):
                    data = rest[3 + struct.calcsize("!B6s"):]

                    if len(data) > 4:
                        line += " [\n" + INDENT
                    else:
                        line += " [ "

                    line += hexdump(data) + " ]"

        else:
            # Raw message, not P2P
            if len(rest) > 4:
                line += f"[\n{INDENT}{hexdump(data)} ]"
            else:
                line += f" [ {hexdump(data)} ]"

        print(textwrap.fill(line, 80, subsequent_indent=INDENT, ))

def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    server_socket.bind(('', 32888))

    while True:
        message, address = server_socket.recvfrom(1024)
        handle_msg(address, message)

if __name__ == "__main__":
    main()
