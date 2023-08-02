*I found the notes from Ulli Rissel in Rabbitcreek's updates hard to read as one giant paragraph, so I've broken them down a bit here.*

## Upgrades from original:

  1. I added a barometric sensor (BMP180) to calculate the air density. The calculation of VO2 should also be correct in the mountains thanks to the barometric sensor. The BMP180 sensor is connected to the I2C bus like the other sensors. I glued it between the two sensors on the Venturi tube. In Arduino, the library "Adafruit_BMP085" must be added.

  2. The VO2max and VEmin calculation is carried out with moving averages and measurement of each individual breath. This allows current measured values every 5 seconds. 

  3. The displays of the values can be switched during the measurement.

  4. If the sensor reaches its limit (at approx. 4.5L/s corresponding to a VEmin of 136L/min!) this is displayed. However, the error should only be minimal. If the limit is displayed too often, the Venturi nozzle would have to be modified. 

  5. A warning appears if the O2 sensor measures less than 20% VO2% at startup. Then the sensor (or room) needs to be ventilated before moving on. If "Continue" is pressed anyway, the value is set to 20.9%.

  6. Battery indicator: The voltage is displayed with background color to the state of charge. White: USB or charge, Green: Battery full, Yellow: Battery approx. 50%, Red: Battery approx. 20%. The display takes place at the top left. A 1100mAh Lipo lasts about 10h!

  7. Kcal are now calculated correctly with their own integral timer.

  8. DEMO mode when the top button is pressed when powered on. It simulates about 28L of respiratory minute volume with respiratory rate of 12 at 4% CO2. This allows you to test the data transfer.

  9. Reset by pressing both buttons. This works after the 3L check.

  10. The volume measurement was measured with a 3L calibration pump and a calibration factor was programmed. The volume measurement can be checked in the first 10 seconds with a real-time display. I tested 2 sensors and for practical reasons set the average calibration factor 0.92 for volume measurement. 

You are welcome to customize this in the program itself (is easy to find).

  11. The data is sent every 5 seconds as csv via USB cable and Bluetooth serial. Data transmission with cable and BT starts immediately after "Press to start". With Excel Datastream, a display can be made in real time.BT on Excel works really well, even with instant graphical representation. 

You can also send the data to the APP Bluetooth Serial Monitor on an Android mobile phone and process or send it from there.

### Some thoughts on the calculation: 

The common spirometers measure the inspiration volume Vi in the supply line, not Ve. 

However, the concentration of O2 and CO2 is measured **expiratory**. 

Therefore, Ve must be calculated in order to correctly record the O2 consumption. 

Three factors come into play: 
  1) The volume shrinks minimally if less CO2 is produced than O2 is absorbed (i.e. at quotient <1). This can be corrected via the Haldane formula. However, the deviations are minimal. 
  2) Much more important is that BTPS conditions prevail in the expiration air (35°C, 95% humidity, actual ambient pressure). 
  3) The measurement has to be converted to STPD conditions (0 °C, 1013.25 hPa, 0% humidity). 

VO2 is defined under STPD conditions. STPD stands for standard temperature (0°C or 273 K) and pressure (760 mm Hg) and dry (no water vapor). 

According to my research, the conversion is nothing more than the ratio of the density of BTPS (actually measured pressure, 37°C, 95% humidity) to STPD. 

*The two symbols BTPS and STPD represent two conventional conditions used when discussing respiratory gases. BTPS stands for body temperature (37°C), ambient pressure and gas saturated with water vapor, whereas STPD stands for standard temperature (0°C or 273 K) and pressure (760 mm Hg) and dry (no water vapor). Pulmonary ventilation (1/min) is usually measured at BTPS, whereas gas volumes in blood are usually expressed at STPD. To convert a gas volume at BTPS to one at STPD, one needs to multiply the former volume by (273/310) (PB − 47) / 760. * (National Library of Medicine reference)[https://www.ncbi.nlm.nih.gov/books/NBK54114/]*

The density of STPD is constant = 1.2922 kg/m³. The density of BTPS can be easily calculated, whereby instead of the gas constant 287.058, the constant for moist air is used for BTPS (about 292.8). 

The constant fluctuates in our temperature range depending on pressure only by a few percent, the density only in the per mille range, if 292.8 is not adjusted. The density indicates how many molecules are in the air.  And now it becomes relatively simple: We measure Ve directly, so no further conversion is required. 

This results in: VO2 = Ve * O2diff% * Density BTPS / 1.2922. Done! By the way, according to the spirometry instructions, VO2 is only calculated from the O2 difference. 

CO2 is only used to calculate the quotient and for the Haldane formula, which we do not need.
