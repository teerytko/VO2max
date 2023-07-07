# VO2 Max Mask 
Super cool project to build your own VO2 Max mask.  Original build instructions can be found on [Instructable](https://www.instructables.com/Accurate-VO2-Max-for-Zwift-and-Strava/) I found the original build instructions comprehensive but fairly dense and disjointed.  This fork of Ivor's excellent Github work incorporates the CO2 enhancements from Ulrich Rissel.  The intent with this repo is to make it even cleaner and easier for someone to follow clear instructions that walk them through everything from purchasing parts to building the device.  Good luck, and enjoy!

## Parts List
| Item | Description/Details | Source | Est Cost |
| --- | --- | --- | --- |
| board (the brain) | ESP8266, alternative to the listed CP2104 | [Amazon](https://www.amazon.com/HiLetgo-Display-Bluetooth-Internet-Development/dp/B07X1W16QS/) | $19 |
| battery| Lipo Battery, 1000 Mah, E503450 3.7v |  [Amazon](https://www.amazon.com/gp/product/B07BTV3W87/ref=ox_sc_act_title_1?smid=A132D7PL1YID8X&psc=1) | $12 |
| pressure sensor | BMP280, calculate air density |  [Amazon](https://www.amazon.com/gp/product/B01ICN5QPU/ref=ox_sc_act_title_1?smid=A30QSGOJR8LMXA&psc=1) | $6 |
| oxygen sensor | Gravity: I2C Oxygen Sensor |  [DF Robot](https://www.dfrobot.com/product-2052.html) | $75 |
| differential pressure sensor | Omron--D6F-PH0025AD2 |  [Digikey](https://www.digikey.com/en/products/detail/omron-electronics-inc-emc-div/d6f-ph0025amd2/15197363) | $28 |
| cable for diff press sensor | DF-CABLE3 |  [Digikey](https://www.digikey.com/en/products/detail/omron-electronics-inc-emc-div/D6F-CABLE3/9471785) | $12 |
| mask | half mask, particulate matter, 6200/07025 | [Digikey](https://www.digikey.com/en/products/detail/3m/6200-07025/7693860) | $22 |
| CO2 sensor | SCD30 |  [Digikey](https://www.digikey.com/en/products/detail/sensirion-ag/SCD30/8445334) | $62 |
| on/off switch | mini push button |  [eBay](https://www.ebay.com/itm/225323324337?hash=item34765113b1:g:EDcAAOSwj5Vjrh1r)| $7 |
| tubing | 1/4" inside diameter (ID) |  Not found yet | Est Cost |

Source code for Arduino under "VO2Max" - Arduino board settings to use for TTGO T-Display:

    Board: ESP32 Dev Module
    Upload Speed: 921600
    CPU Frequency: 240Mhz (WiFi/BT)
    Flash Frequency: 80Mhz
    Flash Mode: QIO
    Flash Size: 4MB (32Mb)
    Partition Scheme: Default 4MB with spiffs (1.2MB APP/1.5 SPIFFS)
    Core Debug Level: None`

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

Additional changes in this version:
- Menu system enhanced with adjustable calibration and setup options.
- Additional GoldenCheetah integration (with VO2 master output)
- CO2 sensor support (Ulrich's mods)
