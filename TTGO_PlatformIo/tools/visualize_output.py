#!/usr/bin/env python3
"""Visualize JSON-lines output produced by the device.

Supports two formats:
  1. Raw sensor output (from serial) - will aggregate per event
  2. Already-aggregated output (from live_visualize_serial.py) - used directly

Creates an interactive HTML plot (`output_plots.html`) and saves PNGs for key charts.

Usage:
  python tools/visualize_output.py --input output.json
  python tools/visualize_output.py --input test_linterval.json

"""
from __future__ import annotations

import argparse
import json
import math
import os
from datetime import datetime

import pandas as pd

try:
    import plotly.express as px
    import plotly.graph_objects as go
except Exception:
    px = None
    go = None

import matplotlib.pyplot as plt


def is_debug_line(line: str) -> bool:
    """Check if line is an ESP-IDF debug/log line.
    
    Debug lines typically start with [timestamp][level][module] format.
    Example: [  4650][D][BLEDevice.cpp:579] getAdvertising(): get advertising
    """
    s = line.strip()
    return s.startswith('[') and '][' in s and ']:' in s


def load_json_lines(path: str) -> list[dict]:
    records = []
    with open(path, "r", encoding="utf-8", errors="ignore") as fh:
        for line in fh:
            s = line.strip()
            if not s:
                continue
            # Skip debug/log lines
            if is_debug_line(s):
                continue
            try:
                records.append(json.loads(s))
            except Exception:
                # skip unparsable lines
                continue
    return records


def is_data_already_aggregated(records: list[dict]) -> bool:
    """Check if records have flattened keys like 'volume.VE' or 'vo2.vo2Total'.
    
    If records have this structure, they're already aggregated.
    Looks at first few records to find one with aggregated keys.
    """
    if not records:
        return False
    # Check the first few records (in case first one is a header)
    aggregated_keys = {'volume.VE', 'volume.volumeExp', 'vo2.vo2Total', 'vco2.vco2Total'}
    for rec in records[:5]:
        found_keys = set(rec.keys()) & aggregated_keys
        if len(found_keys) > 0:
            return True
    return False


def load_aggregated_data(records: list[dict]) -> pd.DataFrame:
    """Load already-aggregated JSON records directly into DataFrame.
    
    These records have flattened keys like 'volume.VE', 'vo2.vo2Total', etc.
    """
    df = pd.json_normalize(records)
    return df


def aggregate_per_event(records: list[dict]) -> pd.DataFrame:
    """Aggregate raw sensor records per "event" boundary.
    
    Events are marked by {"event": "..."} records. Data between events is grouped together.
    Nested dicts (volume, vo2, vco2) are flattened to keys like "volume.VE".
    """
    rows = []
    current = {}
    event_idx = 0

    for rec in records:
        if not isinstance(rec, dict):
            continue
            
        # each rec is like {"event": ...} or {"volume": {...}} etc.
        if "event" in rec:
            # finalize previous if had data
            if current:
                current.setdefault("event_idx", event_idx)
                rows.append(current)
                current = {}
                event_idx += 1
            # store event info
            current_event = rec.get("event")
            if rec.get("time"):
                current["time"] = rec.get("time")
            if rec.get("duration"):
                current["duration"] = rec.get("duration")
            if current_event:
                current["event"] = current_event
        else:
            # merge dicts inside (volume, vo2, vco2)
            for k, v in rec.items():
                if isinstance(v, dict):
                    # flatten
                    for fk, fv in v.items():
                        current[f"{k}.{fk}"] = fv
                else:
                    current[k] = v

    # append last
    if current:
        current.setdefault("event_idx", event_idx)
        rows.append(current)

    if not rows:
        # Return empty dataframe with expected columns if no data
        return pd.DataFrame()

    # normalize times to datetime if present
    df = pd.json_normalize(rows)
    if "time" in df.columns:
        # assume format HH:MM:SS or MM:SS
        def to_dt(t):
            try:
                return datetime.strptime(t, "%H:%M:%S")
            except Exception:
                try:
                    return datetime.strptime(t, "%M:%S")
                except Exception:
                    return pd.NaT

        df["time_dt"] = df["time"].apply(lambda x: to_dt(x) if pd.notna(x) else pd.NaT)
    else:
        df["time_dt"] = pd.NaT

    return df


def plot_with_plotly(df: pd.DataFrame, out_html: str):
    if px is None:
        print("plotly not installed; skipping interactive plots")
        return

    figs = []

    # Plot volume / VE / freq
    cols = [c for c in df.columns if c.startswith("volume.") or c == "event_idx"]
    if cols:
        fig1 = go.Figure()
        if "volume.volumeExp" in df.columns:
            fig1.add_trace(go.Scatter(x=df.index, y=df["volume.volumeExp"], name="volumeExp"))
        if "volume.VE" in df.columns:
            fig1.add_trace(go.Scatter(x=df.index, y=df["volume.VE"], name="VE"))
        if "volume.freqVE" in df.columns:
            fig1.add_trace(go.Scatter(x=df.index, y=df["volume.freqVE"], name="freqVE"))
        fig1.update_layout(title="Volume / VE / Frequency per Event", xaxis_title="event index")
        figs.append(fig1)

    # Plot VO2 / VCO2
    fig2 = go.Figure()
    added = False
    for col in ["vo2.vo2Total", "vo2.consumed_o2", "vco2.vco2Total", "vco2.respq"]:
        if col in df.columns:
            fig2.add_trace(go.Scatter(x=df.index, y=df[col], name=col))
            added = True
    if added:
        fig2.update_layout(title="VO2 / VCO2 per Event", xaxis_title="event index")
        figs.append(fig2)

    # Combine into single HTML
    with open(out_html, "w", encoding="utf-8") as fh:
        fh.write("<html><head><meta charset=\"utf-8\"><title>Plots</title></head><body>\n")
        for i, fig in enumerate(figs):
            fh.write(f"<h2>Chart {i+1}</h2>\n")
            fh.write(fig.to_html(full_html=False, include_plotlyjs=(i == 0)))
            fh.write("<hr/>\n")
        fh.write("</body></html>")

    print(f"Interactive plots written to: {out_html}")


def plot_with_matplotlib(df: pd.DataFrame, out_dir: str):
    os.makedirs(out_dir, exist_ok=True)

    # Volume/VE/freq
    plt.figure(figsize=(8, 4))
    plotted = False
    if "volume.volumeExp" in df.columns:
        mask = df["volume.volumeExp"].notna()
        plt.plot(df.index[mask], df["volume.volumeExp"][mask], marker="o", linestyle='-', linewidth=1.5, label="volumeExp")
        plotted = True
    if "volume.VE" in df.columns:
        mask = df["volume.VE"].notna()
        plt.plot(df.index[mask], df["volume.VE"][mask], marker="o", linestyle='-', linewidth=1.5, label="VE")
        plotted = True
    if "volume.freqVE" in df.columns:
        mask = df["volume.freqVE"].notna()
        plt.plot(df.index[mask], df["volume.freqVE"][mask], marker="o", linestyle='-', linewidth=1.5, label="freqVE")
        plotted = True
    if plotted:
        plt.legend()
        plt.xlabel("event index")
        plt.title("Volume / VE / freq per event")
        plt.grid(True)
        plt.tight_layout()
        outpng = os.path.join(out_dir, "volume_ve_freq.png")
        plt.savefig(outpng)
        plt.close()
        print(f"Saved {outpng}")

    # VO2/VCO2
    plt.figure(figsize=(8, 4))
    plotted = False
    for col in ["vo2.vo2Total", "vo2.consumed_o2", "vco2.vco2Total", "vco2.respq"]:
        if col in df.columns:
            mask = df[col].notna()
            plt.plot(df.index[mask], df[col][mask], marker="o", linestyle='-', linewidth=1.5, label=col)
            plotted = True
    if plotted:
        plt.legend()
        plt.xlabel("event index")
        plt.title("VO2 / VCO2 per event")
        plt.grid(True)
        plt.tight_layout()
        outpng = os.path.join(out_dir, "vo2_vco2.png")
        plt.savefig(outpng)
        plt.close()
        print(f"Saved {outpng}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Visualize device output JSON-lines file")
    parser.add_argument("--input", "-i", default="output.json", help="Path to JSON-lines file")
    parser.add_argument("--out-html", default="output_plots.html", help="Interactive HTML output file")
    parser.add_argument("--out-dir", default="output_plots", help="Directory to save PNGs")
    args = parser.parse_args(argv)

    if not os.path.exists(args.input):
        print(f"Input file not found: {args.input}")
        return 2

    records = load_json_lines(args.input)
    if not records:
        print("No valid JSON records found in input file")
        return 0

    # Create output directory
    os.makedirs(args.out_dir, exist_ok=True)

    # Detect if data is already aggregated
    if is_data_already_aggregated(records):
        print("Data appears to be already aggregated; using directly")
        df = load_aggregated_data(records)
    else:
        print("Data appears to be raw sensor output; aggregating per event")
        df = aggregate_per_event(records)

    if df.empty:
        print("No data to visualize")
        return 0

    # Save a CSV for debugging
    df.to_csv(os.path.join(args.out_dir, "aggregated.csv"), index=False)

    plot_with_matplotlib(df, args.out_dir)
    plot_with_plotly(df, args.out_html)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
