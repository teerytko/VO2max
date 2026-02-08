#!/usr/bin/env python3
"""Live visualize device output from a serial port.

This script connects to a serial port (e.g. COM6), reads JSON-lines output emitted
by the device, aggregates values per "expiration" event and updates two matplotlib
subplots in real time:
 - Volume / VE / freq
 - VO2 / VCO2

Usage:
  python tools/live_visualize_serial.py --port COM6 --baud 115200
  python tools/live_visualize_serial.py --port COM6 --baud 115200 --output data.json

Controls:
 - Close the plot window to exit.
 - Click the plot to pause/resume.

Notes:
 - Requires `pyserial`, `pandas`, and `matplotlib` (already in tools/requirements.txt)
"""
from __future__ import annotations

import argparse
import json
import queue
import threading
import time
from collections import deque
from datetime import datetime
from typing import Any, Dict

try:
    import serial
except Exception:  # helpful error message later
    serial = None

import matplotlib.pyplot as plt
import matplotlib.animation as animation


def is_debug_line(line: str) -> bool:
    """Check if line is an ESP-IDF debug/log line.
    
    Debug lines typically start with [timestamp][level][module] format.
    Example: [  4650][D][BLEDevice.cpp:579] getAdvertising(): get advertising
    """
    s = line.strip()
    return s.startswith('[') and '][' in s and ']:' in s


class SerialEventReader(threading.Thread):
    """Background thread that reads lines from serial and emits completed events.

    It pushes aggregated event dicts into a provided queue when an "event" record
    appears in the stream (signalling end of an expiration cycle).
    """

    def __init__(self, port: str, baud: int, out_q: queue.Queue, encoding: str = "utf-8"):
        super().__init__(daemon=True)
        self.port = port
        self.baud = baud
        self.encoding = encoding
        self.out_q = out_q
        self._stop = threading.Event()
        self._ser = None

    def stop(self) -> None:
        self._stop.set()
        try:
            if self._ser and self._ser.is_open:
                self._ser.close()
        except Exception:
            pass

    def run(self) -> None:
        if serial is None:
            self.out_q.put({"__error": "pyserial not installed"})
            return

        try:
            self._ser = serial.Serial(self.port, baudrate=self.baud, timeout=1)
        except Exception as exc:
            self.out_q.put({"__error": f"Failed opening serial port: {exc}"})
            return

        current: Dict[str, Any] = {}
        event_index = 0

        while not self._stop.is_set():
            try:
                raw = self._ser.readline()
                if not raw:
                    continue
                try:
                    line = raw.decode(self.encoding, errors="ignore").strip()
                except Exception:
                    line = raw.decode("utf-8", errors="ignore").strip()

                if not line:
                    continue

                # Skip debug/log lines
                if is_debug_line(line):
                    continue

                try:
                    rec = json.loads(line)
                except Exception:
                    # not JSON; skip
                    continue

                # If the record contains an "event" key we treat it as boundary
                if "event" in rec:
                    # attach time if present
                    if 'time' in rec:
                        current['time'] = rec.get('time')
                    # if there's any accumulated data, emit it
                    if current:
                        current['event_idx'] = event_index
                        self.out_q.put(current.copy())
                        event_index += 1
                        current.clear()
                    # keep the event label if other data follows
                    # optionally store event name
                    if 'event' in rec:
                        current['event_name'] = rec.get('event')
                else:
                    # merge inner dicts (volume, vo2, vco2) into flat keys
                    for k, v in rec.items():
                        if isinstance(v, dict):
                            for fk, fv in v.items():
                                current[f"{k}.{fk}"] = fv
                        else:
                            current[k] = v

            except Exception:
                # small sleep on unexpected errors to avoid tight loop
                time.sleep(0.1)

        # On exit, if a partial current exists push it
        if current:
            current['event_idx'] = event_index
            self.out_q.put(current)


def flatten_event(event: Dict[str, Any]) -> Dict[str, float]:
    """Convert event dict to numeric values for plotting. Non-numeric values are skipped."""
    out: Dict[str, float] = {}
    for k, v in event.items():
        if k.startswith('__'):
            continue
        if isinstance(v, (int, float)):
            out[k] = float(v)
        else:
            # try convert
            try:
                out[k] = float(v)
            except Exception:
                # ignore non-numeric (e.g., event_name, time)
                continue
    return out


def run_live_plot(port: str, baud: int, max_points: int = 200, output_file: str | None = None):
    q: queue.Queue = queue.Queue()
    reader = SerialEventReader(port, baud, q)
    reader.start()

    # open output file if specified
    out_fh = None
    if output_file:
        try:
            out_fh = open(output_file, 'w', encoding='utf-8')
        except Exception as exc:
            print(f'Warning: could not open output file {output_file}: {exc}')

    # buffers
    idxs = deque(maxlen=max_points)
    time_seconds = deque(maxlen=max_points)
    vol_volumeExp = deque(maxlen=max_points)
    vol_VE = deque(maxlen=max_points)
    vol_freqVE = deque(maxlen=max_points)

    vo2_vo2Total = deque(maxlen=max_points)
    vco2_vco2Total = deque(maxlen=max_points)
    vco2_respq = deque(maxlen=max_points)
    
    vo2_vo2Rel = deque(maxlen=max_points)
    vco2_vco2Rel = deque(maxlen=max_points)

    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 10))
    
    # Subplot 1: Volume / VE / freq
    line_v_vol, = ax1.plot([], [], 'o-', linewidth=2, markersize=4, label='volume.volumeExp')
    line_v_VE, = ax1.plot([], [], 's-', linewidth=2, markersize=4, label='volume.VE')
    line_v_freq, = ax1.plot([], [], 'd-', linewidth=2, markersize=4, label='volume.freqVE')
    ax1.set_ylabel('Volume / VE / freq')
    ax1.legend(loc='upper left')
    ax1.grid(True)

    # Subplot 2: VO2 / VCO2 (absolute)
    line_vo2_total, = ax2.plot([], [], 'o-', linewidth=2, markersize=4, label='vo2.vo2Total')
    line_vco2_total, = ax2.plot([], [], 'd-', linewidth=2, markersize=4, label='vco2.vco2Total')
    ax2.set_ylabel('VO2 / VCO2')
    ax2.legend(loc='upper left')
    ax2.grid(True)
    
    # Subplot 3: VO2 / VCO2 (relative) + respq
    line_vo2_rel, = ax3.plot([], [], 'o-', linewidth=2, markersize=4, label='vo2.vo2Rel')
    line_vco2_rel, = ax3.plot([], [], 's-', linewidth=2, markersize=4, label='vco2.vco2Rel')
    line_vco2_respq, = ax3.plot([], [], '^-', linewidth=2, markersize=4, label='vco2.respq')
    ax3.set_ylabel('VO2Rel / VCO2Rel / RespQ')
    ax3.set_xlabel('Time (s)')
    ax3.legend(loc='upper left')
    ax3.grid(True)

    paused = {'val': False}

    def on_click(event):
        # toggle pause when figure clicked
        paused['val'] = not paused['val']

    fig.canvas.mpl_connect('button_press_event', on_click)

    def update(frame):
        # pull all available events from queue
        new = False
        while True:
            try:
                ev = q.get_nowait()
            except queue.Empty:
                break
            # check for error messages
            if isinstance(ev, dict) and ev.get('__error'):
                print('Reader error:', ev.get('__error'))
                reader.stop()
                raise SystemExit(1)

            # write raw event to output file if specified
            if out_fh:
                try:
                    out_fh.write(json.dumps(ev, ensure_ascii=False) + '\n')
                    out_fh.flush()
                except Exception as exc:
                    print(f'Warning: failed to write to output file: {exc}')

            flat = flatten_event(ev)
            
            # Skip events that don't have volume/vo2 data (e.g., INSPIRATION events)
            if 'volume.volumeExp' not in flat and 'vo2.vo2Total' not in flat:
                continue
            
            idx = ev.get('event_idx', (idxs[-1] + 1) if idxs else 0)
            idxs.append(idx)
            
            # Convert time string (MM:SS or HH:MM:SS) to seconds
            time_str = ev.get('time', '0:0')
            try:
                parts = time_str.split(':')
                if len(parts) == 3:
                    h, m, s = map(int, parts)
                    t_seconds = h * 3600 + m * 60 + s
                elif len(parts) == 2:
                    m, s = map(int, parts)
                    t_seconds = m * 60 + s
                else:
                    t_seconds = float(parts[0])
            except Exception:
                t_seconds = 0
            time_seconds.append(t_seconds)
            
            vol_volumeExp.append(flat.get('volume.volumeExp', float('nan')))
            vol_VE.append(flat.get('volume.VE', float('nan')))
            vol_freqVE.append(flat.get('volume.freqVE', float('nan')))

            vo2_vo2Total.append(flat.get('vo2.vo2Total', float('nan')))
            vco2_vco2Total.append(flat.get('vco2.vco2Total', float('nan')))
            vco2_respq.append(flat.get('vco2.respq', float('nan')))
            
            vo2_vo2Rel.append(flat.get('vo2.vo2Rel', float('nan')))
            vco2_vco2Rel.append(flat.get('vco2.vco2Rel', float('nan')))

            new = True

        if paused['val']:
            return []

        if not new and not idxs:
            return []

        # update data using time as x-axis
        xs = list(time_seconds)
        
        # Helper to get min/max of valid (non-nan) values
        def get_limits(data):
            valid = [v for v in data if isinstance(v, (int, float)) and v == v and v != float('inf') and v != float('-inf')]
            if not valid:
                return -0.1, 0.1
            mn, mx = min(valid), max(valid)
            if mn == mx:
                return mn - 0.5, mn + 0.5
            margin = abs(mx - mn) * 0.1
            return mn - margin, mx + margin
        
        line_v_vol.set_data(xs, list(vol_volumeExp))
        line_v_VE.set_data(xs, list(vol_VE))
        line_v_freq.set_data(xs, list(vol_freqVE))
        y_min, y_max = get_limits(list(vol_volumeExp) + list(vol_VE) + list(vol_freqVE))
        ax1.set_ylim(y_min, y_max)
        if xs:
            ax1.set_xlim(xs[0] - 1, xs[-1] + 1)

        line_vo2_total.set_data(xs, list(vo2_vo2Total))
        line_vco2_total.set_data(xs, list(vco2_vco2Total))
        y_min, y_max = get_limits(list(vo2_vo2Total) + list(vco2_vco2Total))
        ax2.set_ylim(y_min, y_max)
        if xs:
            ax2.set_xlim(xs[0] - 1, xs[-1] + 1)
        
        line_vo2_rel.set_data(xs, list(vo2_vo2Rel))
        line_vco2_rel.set_data(xs, list(vco2_vco2Rel))
        line_vco2_respq.set_data(xs, list(vco2_respq))
        y_min, y_max = get_limits(list(vo2_vo2Rel) + list(vco2_vco2Rel) + list(vco2_respq))
        ax3.set_ylim(y_min, y_max)
        if xs:
            ax3.set_xlim(xs[0] - 1, xs[-1] + 1)

        return [line_v_vol, line_v_VE, line_v_freq, line_vo2_total, line_vco2_total, line_vo2_rel, line_vco2_rel, line_vco2_respq]

    ani = animation.FuncAnimation(fig, update, interval=500, blit=False)

    try:
        plt.tight_layout()
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        reader.stop()
        if out_fh:
            try:
                out_fh.close()
                print(f'Saved data to {output_file}')
            except Exception:
                pass


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description='Live-visualize serial JSON-lines from device')
    parser.add_argument('--port', '-p', required=True, help='Serial port (e.g. COM6)')
    parser.add_argument('--baud', '-b', type=int, default=115200, help='Baud rate')
    parser.add_argument('--max-points', type=int, default=200, help='Number of events to keep visible')
    parser.add_argument('--output', '-o', help='Output JSON-lines file to save data')
    args = parser.parse_args(argv)

    if serial is None:
        print('pyserial not available. Install with: pip install pyserial')
        return 2

    try:
        run_live_plot(args.port, args.baud, max_points=args.max_points, output_file=args.output)
    except Exception as exc:
        print('Error running live plot:', exc)
        return 1

    return 0


if __name__ == '__main__':
    raise SystemExit(main())
