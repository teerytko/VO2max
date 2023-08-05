# VO2 Max Mask 
Super cool project to build your own VO2 Max mask.  Your "VO2 Max" is a measure of how much oxygen your body and absorb and use during exercise, read more about it [here](https://www.healthline.com/health/vo2-max#benefits).

Original idea and detailed build instructions can be found on [Instructable](https://www.instructables.com/Accurate-VO2-Max-for-Zwift-and-Strava/). 

I found the original build instructions comprehensive but fairly dense and disjointed, especially regarding the upgrades by urissel and ivor.  They were rad, but I was constantly scrolling and refreshing pages to find the next step.

I also thought there's a chance I can put my shoulder to the wheel here and rally a few experts to really improve some aspects of this; keep track of that over in the Wiki (up top for those you not used to Github).

This fork of [Ivor's excellent Github work](https://github.com/ihewitt/VO2max) incorporates the CO2 enhancements from Ulrich Rissel.  The intent with this repo is to make it as easy as possible to build a world class piece of kit that'll add useful metrics to your training.

 Currently (July 23th 2023) a work in progress.  Refer to the Wiki for exactly where we are now.

## Parts List
| Item | Description/Details | Source | Est Cost |
| --- | --- | --- | --- |
| board (the brain) | ESP32 T-Display, 16MB CH9102F | [Amazon](https://www.amazon.com/gp/product/B099MPFJ9M/) | $19 |
| battery| Lipo Battery, 1000 Mah, E503450 3.7v |  [Amazon](https://www.amazon.com/gp/product/B07BTV3W87/ref=ox_sc_act_title_1?smid=A132D7PL1YID8X&psc=1) | $12 |
| pressure sensor | BMP280, calculate air density |  [Amazon](https://www.amazon.com/gp/product/B01ICN5QPU/ref=ox_sc_act_title_1?smid=A30QSGOJR8LMXA&psc=1) | $6 |
| screws | 2 x M3 x 8mm | [Amazon](https://www.amazon.com/Hilitchi-510-Pcs-Button-Socket-Assortment/dp/B073SWNV5N/) | $15 for kit |
| threaded inserts | M3 x 8 x 5mm | [Amazon](https://www.amazon.com/gp/product/B096MD67WB/) | $18 for kit |
| aquarium tubing | standard 3/16", 4mm inside diameter (ID) |  [Amazon](https://www.amazon.com/AQUANEAT-Standard-Airline-Aquarium-PREMIUM/dp/B01N8SNNOI/) | $6 |
| o-ring | for oxygen sensor, 22x19x1.5mm | [Amazon](https://www.amazon.com/uxcell-Silicone-Rings-Diameter-Gasket/dp/B082STZ4XL?th=1) | $7 for 30 |
| oxygen sensor | Gravity: I2C Oxygen Sensor |  [DF Robot](https://www.dfrobot.com/product-2052.html) | $75 |
| differential pressure sensor | Omron--D6F-PH0025AD2 |  [Digikey](https://www.digikey.com/en/products/detail/omron-electronics-inc-emc-div/d6f-ph0025amd2/15197363) | $28 |
| cable for diff press sensor | DF-CABLE3 |  [Digikey](https://www.digikey.com/en/products/detail/omron-electronics-inc-emc-div/D6F-CABLE3/9471785) | $12 |
| mask | half mask, particulate matter, 6200/07025 | [Digikey](https://www.digikey.com/en/products/detail/3m/6200-07025/7693860) | $22 |
| CO2 sensor | SCD30 |  [Digikey](https://www.digikey.com/en/products/detail/sensirion-ag/SCD30/8445334) | $62 |
| on/off switch | mini push button |  [Adafruit](https://www.adafruit.com/product/1683)| $6 |

## Print 3D Parts
Print out the 3D parts using PLA.  I used ASA because that's what I had and I was eager to get started. I've printed both the 20mm and the 16mm Venturi tubes.  I found the 20mm had a problem with the threads being super thin and breaking off.  For now (July 9th, 2023) I recommend against the 20mm.

*Do not* use supports on anything except the "computer housing".

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

