Serial/File Parser

This repository includes a small Python utility that reads lines from either a text file or a serial port and prints parsed output.

Files:
- [scripts/serial_file_parser.py](scripts/serial_file_parser.py)
- [requirements.txt](requirements.txt)

Install dependencies:

```bash
python -m pip install -r requirements.txt
```

Examples:

Read a file:

```bash
python scripts/serial_file_parser.py --file data.txt
```

Read from a serial port (Windows example):

```bash
python scripts/serial_file_parser.py --port COM6 --baud 115200
```

Options:
- `--raw`: print raw lines (or JSON records if lines are JSON)
- `--limit N`: stop after N lines

Behavior:
- Each input line is first attempted to be parsed as JSON; if parsing fails the line is returned as raw text.

Visualization:

- A visualization utility is available at `scripts/visualize_output.py` that reads a JSON-lines file (like `output.json`), aggregates measurements per "expiration" event and creates interactive and static plots.

Examples:

```bash
python scripts/visualize_output.py --input output.json --out-html output_plots.html --out-dir output_plots
```

This will create `output_plots.html` (interactive Plotly charts) and PNG files in the `output_plots` directory.

