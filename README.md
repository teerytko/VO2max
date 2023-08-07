# VO2 Max Mask 
Super cool project to build your own VO2 Max mask.  Your "VO2 Max" is a measure of how much oxygen your body and absorb and use during exercise, read more about it [here](https://www.healthline.com/health/vo2-max#benefits).

Original idea and detailed build instructions can be found on [Instructable](https://www.instructables.com/Accurate-VO2-Max-for-Zwift-and-Strava/). 

I found the original build instructions comprehensive but fairly dense and disjointed, especially regarding the upgrades by urissel and ivor.  They were rad, but I was constantly scrolling and refreshing pages to find the next step.

I also thought there's a chance I can put my shoulder to the wheel here and rally a few experts to really improve some aspects of this; keep track of that over in the Wiki (up top for those you not used to Github).

This fork of [Ivor's excellent Github work](https://github.com/ihewitt/VO2max) incorporates the CO2 enhancements from Ulrich Rissel.  The intent with this repo is to make it as easy as possible to build a world class piece of kit that'll add useful metrics to your training.

Currently (July 23th 2023) a work in progress.  Refer to the Wiki for exactly where we are now.

## Order Sensors, board, and assorted fasteners.
Check the [BOM](https://github.com/meteoscientific/VO2max/blob/main/BOM.md) for all the various parts to order, they can take a week or two to come in.

## Print 3D Parts
Print out the 3D parts using PLA.  This is still a work in progress, but feel free to explore the 3D Test Print directory and print out what you'd like.  Nothing works well yet (Aug 4th, 2023).

<br>Test fit all parts and make sure you know where everything goes; that will make the next steps much easier.

## Program Board
When you first get the board, hold down the boot button while plugging it in, and keep that down.  You need to erase everything on the board before you load the rest.  From there, you should be alb to use the Platformio folder to load everything you'll need.  I did this on a Windows computer (a little Beelink with a monitor & display) because I couldn't get it to work on my Mac.

## Wire It Up

## Assembly
I painted my button housings black for contrast.  Use superglue to secure the button housing to the case, make sure the button is inside first.
<figure>
    <img src="/images/button_housing.jpg" width="640" height="327"
         alt="Button Housing">
</figure><br><br>
 
## Arduino (from Ivor)
Source code for Arduino under "VO2Max" - Arduino board settings to use for TTGO T-Display:

    Board: ESP32 Dev Module
    Upload Speed: 921600
    CPU Frequency: 240Mhz (WiFi/BT)
    Flash Frequency: 80Mhz
    Flash Mode: QIO
    Flash Size: 4MB (32Mb)
    Partition Scheme: Default 4MB with spiffs (1.2MB APP/1.5 SPIFFS)
    Core Debug Level: None`

## Useful Images
<figure>
    <img src="/images/sensor_measurements.JPG" width="480" height="640"
         alt="Sensor Measurements">
    <figcaption>Measurements for the sensors.</figcaption>
</figure><br><br>

<figure>
    <img src="/images/parts.jpg" width="640" height="480"
         alt="Build parts">
    <figcaption>Source parts, top to bottom. 3M mask with front plate removed, 3D printed case, Oxygen sensor, TTGo T-Display, Flow sensor.</figcaption>
</figure><br><br>
<figure>
    <img src="/images/built.jpg" width="640" height="480"
         alt="First build">
    <figcaption>First finished build.</figcaption>
</figure><br><br>
<figure>
    <img src="/images/upgrading.jpg" width="640" height="480"
         alt="Upgrading">
    <figcaption>Rebuilding to use CO2 sensor. SCD30 pictured right.</figcaption>
</figure><br><br>
<figure>
    <img src="/images/casefilling.jpg" width="640" height="480"
         alt="Upgraded build">
    <figcaption>Assembled into case tightly, BM280 barometer addition mounted onto front of tube, wiring for CO2 monitor fed behind and out to top.</figcaption>
</figure><br><br>
<figure>
    <img src="/images/built2.jpg" width="640" height="480"
         alt="Upgraded build">
    <figcaption>Pictured with the CO2 sensor upgrade attached</figcaption>
</figure><br><br>

3D printing files are within the `design` folder, Ulrich Rissel's design files to use a larger venturi diameter with CO2 sensor holder in `design/CO2_upgrade`

## Usage - Zwift & Strava
* Enable bluetooth on your phone
* Go to "Record" function at bottom of screen on unit, and press it
* Look for the Heart icon at the bottom, and push that to bring up the bluetooth pairing screen to find the unit

## Usage - App
* Turn on device
* Add your weight (kg or lbs?)
* Push the Go button
* Turn on the Sensirion App, which will automatically pair and start recording data

Programing is done through the USB-C connector. 
Charging the battery is accomplished by turning on the unit and then plugging it in.

The App is designed for collecting data from a CO2 sensor so you have to spoof it by sending the Volume Minute of O2 to the CO2 level screen, the VO2 max to the Temp screen and the O2 level to the Humidity screen. 


## Additional changes in this version:
- Menu system enhanced with adjustable calibration and setup options.
- Additional GoldenCheetah integration (with VO2 master output)
- CO2 sensor support (Ulrich's mods)

## Running the unit with Zwift or Strava
Use FinalZwiftConnect with files
* DFRobot_OxygenSensor.cpp
* DFRobot_OxygenSensor.h

## Running the unit on the [Sensirion MyAmbience app](https://apps.apple.com/us/app/sensirion-myambience/id1529131572) (iOS)
* FinalSensirionScreen
* DFRobot_OxygenSensor.cpp
* DFRobot_OxygenSensor.h
* Sensirion_GadgetBle_Lib.cpp
* Sensirion_GadgetBle_Lib.h

