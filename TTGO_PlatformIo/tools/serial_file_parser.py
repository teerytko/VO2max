#!/usr/bin/env python3
"""Read from a file or a serial port and yield parsed records.

Usage examples:
  python scripts/serial_file_parser.py --file data.txt
  python scripts/serial_file_parser.py --port COM6 --baud 115200

The parser will try to parse each line as JSON; if that fails it returns the raw string.
"""
from __future__ import annotations

import argparse
import json
import os
import sys
import time
from typing import Iterator, Union

try:
    import serial
except Exception:  # pragma: no cover - optional dependency
    serial = None


def is_debug_line(line: str) -> bool:
    """Check if line is an ESP-IDF debug/log line.
    Debug lines typically start with [timestamp][level][module] format.
    Example: [  4650][D][BLEDevice.cpp:579] getAdvertising(): get advertising
    """
    s = line.strip()
    return s.startswith('[') and '][' in s and ']:' in s


def parse_line(line: str) -> Union[dict, str, None]:
    """Parse a line, returning dict for JSON, string for non-JSON, None for debug lines."""
    s = line.strip()
    if not s:
        return None
    # Skip ESP-IDF debug/log lines
    if is_debug_line(s):
        return None
    try:
        return json.loads(s)
    except Exception:
        return s


def read_from_file(path: str, encoding: str = "utf-8") -> Iterator[Union[dict, str]]:
    with open(path, "r", encoding=encoding, errors="ignore") as fh:
        for line in fh:
            yield parse_line(line)


def read_from_serial(port: str, baud: int = 115200, encoding: str = "utf-8") -> Iterator[Union[dict, str]]:
    if serial is None:
        raise RuntimeError("pyserial is required to read from serial ports. Install with: pip install pyserial")
    with serial.Serial(port, baudrate=baud, timeout=1) as ser:
        # flush input a little
        time.sleep(0.1)
        try:
            ser.reset_input_buffer()
        except Exception:
            pass
        while True:
            raw = ser.readline()
            if not raw:
                continue
            try:
                line = raw.decode(encoding, errors="ignore")
            except Exception:
                line = raw.decode("utf-8", errors="ignore")
            yield parse_line(line)


def print_record(record: Union[dict, str, None], out_fh = None) -> None:
    if record is None:
        return
    if isinstance(record, dict):
        print(json.dumps(record, ensure_ascii=False))
        if out_fh:
            out_fh.write(json.dumps(record, ensure_ascii=False) + "\n")
    else:
        print(json.dumps({"raw": record}, ensure_ascii=False))
        if out_fh:
            out_fh.write(json.dumps({"raw": record}, ensure_ascii=False) + "\n")

def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Read lines from a file or serial port and parse them")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--file", "-f", help="Path to a text file to read")
    group.add_argument("--port", "-p", help="Serial port to open (e.g. COM6)")
    parser.add_argument("--out-file", "-o", help="Path to an output text file to write parsed records to (in addition to stdout)")
    parser.add_argument("--baud", "-b", type=int, default=115200, help="Baud rate for serial")
    parser.add_argument("--encoding", "-e", default="utf-8", help="Text encoding to use")
    parser.add_argument("--raw", action="store_true", help="Print raw lines instead of parsed JSON/object output")
    parser.add_argument("--limit", "-n", type=int, default=0, help="Limit number of lines to read (0 = unlimited)")

    args = parser.parse_args(argv)

    try:
        out_fh = None
        if args.file:
            if not os.path.exists(args.file):
                print(f"File not found: {args.file}", file=sys.stderr)
                return 2
            source = read_from_file(args.file, encoding=args.encoding)
        else:
            source = read_from_serial(args.port, baud=args.baud, encoding=args.encoding)

        if args.out_file:
            out_fh = open(args.out_file, "w", encoding=args.encoding, errors="ignore")

        count = 0
        for record in source:
            # Skip None records (debug lines, empty lines)
            if record is None:
                continue
            count += 1
            print_record(record, out_fh=out_fh)
            if args.limit and count >= args.limit:
                break

    except KeyboardInterrupt:
        return 0
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
