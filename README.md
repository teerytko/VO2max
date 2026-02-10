# VO2 Max Mask 
Super cool project to build your own VO2 Max mask.  Your "VO2 Max" is a measure of how much oxygen your body and absorb and use during exercise, read more about it [here](https://www.healthline.com/health/vo2-max#benefits).

Original idea and detailed build instructions can be found on [Instructable](https://www.instructables.com/Accurate-VO2-Max-for-Zwift-and-Strava/). Other designs can be found [here, a UC Davis Med Center project](https://faculty.engineering.ucdavis.edu/knoesen/wp-content/uploads/sites/119/2016/12/OOCOO_WirelessHealthSubmission_Final.pdf) and a [commercial version that costs just under US$6,000](https://vo2master.com/).

This fork of [excellent Github work](https://github.com/meteoscientific/VO2max) incorporates the CO2 enhancements. The intent with this repo is to make it as easy as possible to build a world class piece of kit that'll add useful metrics to your training.

## Design Constraints
Less than $200 for all parts and printing.
Printable by anyone with a 3D printer (easy print files).
Can handle up to 200 liters per minute of exhaled air.
Small and light enough to be worn for a full workout.
Able to measure resting as well maximum VO2.

## App Intent
Allows an athlete to monitor real time [RER](https://www.adinstruments.com/signal/rer#:~:text=Respiratory%20Exchange%20Ratio%20(RER)%20is,is%20operating%20aerobically%20or%20anaerobically.).

Allows an athlete to monitor gross mechanical efficiency

The main mini version now aims to do just the basic VO2, VCO2 measurements, which can be sent via BLE to external device (e.g. phone). Any other sensors should connected to the phone as a data collection device.

## Versions
- V1 - Original version that works, can be found via the Instructable link above.
- V2 - Upgraded version by Urissel & Ivor, includes the CO2 & ambient temp/pressure to adapt to different elevation & temperatures.
- V3 - Proposed by Mahmoud, this is the T version.  Currently abandoned due to issues with getting correct sensor readings.
- V4 - Proposed by Stefan, affectionately called "The Snork".  Latest version.
- V5 - My latest version is based of V1, but not yet finalized. 

## Recent Firmware Status (as of 2026-02-08)
Latest commits in this repo:
- `f149ecc` (2026-02-08): Update the JSON outputs and scripts to match.
- `c29ee34` (2026-02-07): Add support for scripts parsing serial outputs.
- `f1a9a76` (2026-02-07): Refactor VO2 BLE server and serial output.
- `85bd076` (2026-02-07): Split BLE data into separate characteristics.
- `75d0030` (2026-01-06): Refactor volume calculations.

The active PlatformIO firmware lives in `TTGO_PlatformIo` and includes:
- `src/main.cpp`: Full firmware variant.
- `src/main_mini.cpp`: Newer "mini" variant with JSON serial telemetry and BLE service split.
- `src/vo2_ble_service.h` and `src/vo2_ble_service.cpp`: BLE server wrapper with separate characteristics for VO2, VCO2, and RQ.
- `tools/`: Python utilities for parsing and plotting serial JSON output.

### Steps to Build & Use
Check the build notes in `TTGO_PlatformIo/README`.

## Order Sensors, board, and assorted fasteners.
Check the [BOM](https://github.com/meteoscientific/VO2max/blob/main/BOM.md) for all the various parts to order, they can take a week or two to come in.

## Print 3D Parts
Print out the 3D parts using PLA.  This is still a work in progress, but feel free to explore the 3D Test Print directory and print out what you'd like.  Nothing works well yet (Aug 4th, 2023).

<br>Test fit all parts and make sure you know where everything goes; that will make the next steps much easier.

## Program Board
Originally this project was built with the Arduino IDE.  A few of us nerds decided that Platformio might be easier and more rad, so we're using that.

When you first get the board, hold down the boot button while plugging it in, and keep that down.  You need to erase everything on the board before you load the rest.  From there, you should be able to use the Platformio folder to load everything you'll need.  I did this on a Windows computer (a little Beelink with a monitor & display) because I couldn't get it to work on my Mac.

Current PlatformIO commands (from `TTGO_PlatformIo`):

```bash
platformio.exe run -e lilygo-vo2mini
platformio.exe run --target upload -e lilygo-vo2mini
platformio.exe test -e lilygo-vo2mini
platformio.exe device monitor -e lilygo-vo2mini
```

## Serial Output Tooling (JSON)
The `TTGO_PlatformIo/tools` directory now contains utilities for processing JSON serial output from the firmware:
- `serial_file_parser.py`: Parse JSON-lines from serial (`--port`) or from a captured file (`--file`).
- `live_visualize_serial.py`: Live plot JSON events from a serial port.
- `visualize_output.py`: Aggregate captured JSON-lines and generate HTML/PNG plots.

Install requirements and run examples:

```bash
python -m pip install -r TTGO_PlatformIo/tools/requirements.txt
python TTGO_PlatformIo/tools/serial_file_parser.py --port COM6 --baud 115200 --out-file output.json
python TTGO_PlatformIo/tools/visualize_output.py --input output.json --out-html output_plots.html --out-dir output_plots
```

## Wire It Up

## Assembly

