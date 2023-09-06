Robert found this project and had the following notes, all relevant to the v1 form factor (not the v4 "Snork" designed by Stefan).

## Oxygen sensor:
1. As we know there is (or was) the issue of condensing air. In the new design it shouldn't be an issue anymore, if I'm correct? 
Anyway, I don't know which sensors the expensive spirometers use (do you?), but I'm guessing it's an oxygen sensor from the medical field.

Here's a [list of City Techs/Honeywell oxygen sensors](https://euro-gasman.com/app/uploads/2020/11/CITY-TECH-OXYGEN-SENSORS-LIST.pdf). Take a closer look at the "MediceL" ones: they are typically used for medical applications. 

The "best" as in "fastest response time" is the [MOX20](https://euro-gasman.com/app/uploads/2020/11/O2-MOX-20-MEDICEL-V2.pdf). It has a response time of <0.75s. That's quite fast, isn't it? If I remember correctly the data shouldn't be polled more often than once per second. Of course, being the fastest sensor comes at a high price (>$200).

What I noticed is that none of these sensors should be operated in condesing air. Even the MOX20 also states "Operating Humidity Range: 0-99% RH non-condensing". I haven't found a suitable sensor (T90 <5s) which works in a condensing environment. This leads me to believe that this can only be solved by design meaning we need to have a design which reduces condensate to a minimum.

Expensive spirometers will most likely use a sensor which won't work well in condensing air.

2. Response time: What is the response time of sensors used in expensive spirometers? I'm honestly guessing they are slower than 5s. Probably in the range of 5-15s. Which really is not that much of an issue for most of the time and as we are building a similar device we need to think about the importance of a fast response time. A faster than usual response time may even produce higher VO2max values than what the athlete might expect (I'll share more about that in the section "Calculations").

3. What I think we need is a cheap, fast enough, very reliable and if possible as long lasting oxygen sensor as possible. These sensors have pretty much an expiration date from the moment you first get them. Using a sensor with a high stability should be preferred.

4. I don't have a recommendation yet as I haven't really tested my VO2max mask enough. I read that some people complain about wrong readings, but I can't confirm that. If it works well the sensor should be suitable.

## CO2 sensor:
1. The SCD30's measurement range is not suited for accurately measuring our exhaled CO2 concentration. The range is 0-40,000 ppm which equals 0-4 vol%. As far as I know the exhaled CO2 concentration is always around the 4% mark (and my own tests confirm that). This makes the sensor inaccurate. 

2. Response time: The SCD30 has a pretty high response time of 20s to τ63%. Contrary to what I first thought this might not be that much of an issue, because we really only need CO2 for aerobic work measurements meaning at a VO2 below the maximum (depends on the athlete and the range should be from 60% VO2max to up to 90%). The reason is that the RQ should be at or higher than 1.0 as soon as the fat oxidation stops. This typically happens at your MaxLaSS (maximum lactate steady state, which is not the same but similiar to the FTP in biking). Below that threshold it doesn't really matter if it takes a minute to output the correct value, except for in-depth training analysis of high intensity workouts.

3. Keeping that in mind I'd say that having a low response time (<10s) is just an optional feature. Of course it shouldn't take much longer than the current SCD30 is able to accurately determine the CO2 concentration. The plus side is that CO2 sensors don't really come with an expiry date or it's as high as >10 years, so that's of no concern for us.

4. Recommendation? Well, again that's hard. I've searched but haven't really found the most suitable sensor for our device. There are some automotive sensors but I honestly forgot why I thought they aren't a good fit. Another sensor from Sensirion that uses a completely different technology than NDIR (SCD30) is the STC31, which uses a thermal conductivity measurement principle. Let me tell you the advantages first: it can measure a CO2 concentration of 0-25 vol% (or 0-250,000 ppm).

We'd only really need 0-5 vol%, but that's okay. It has an extremely fast measurement time of 66 ms, although I don't know how that value compares to the SCD30 as it says "measurement time" and not "τ63%". It works up to a humidity of 40°C dew point and does not seem to have issues with condensing air (the SCD30 does!). 40°C dew point is just a bit more than we need. It's extremely small. However, on the downside the accuracy of 0.5 vol% + 3% measured value might be too low and the small form factor makes it pretty much impossible to solder manually, but there are options available.
What do you think?

## Pressure sensor:
I haven't taken a look at this sensor. But I always wondered why we don't use something like the Sensirion SFM3400-AW (https://www.mouser.com/datasheet/2/682/Sensirion_Datasheet_SFM3400_AW_SFM3400_D-3002584.pdf)? Is it the price? This would eliminate the need to calibrate the sensor with a 3l calibration syringe.

## Ambient pressure sensor:
The current one seems to be okay, although the first one I got didn't work. Maybe my soldering skills were that bad and destroyed it but I think Nik (or someone else) had the same problem. Sounds to me that they aren't that reliable?

## Air flow:
The original design definitely had issues due to outside airflow interfering with the measurements. Think about this: When you are doing a VO2max test you will most likely run a fan or an ac. The fan will push air exactly in the direction the mask is facing. During my tests without a fan I could see changes in the measured expiratory volume and ventilation frequency just by moving around a little. Using a fan or an ac completely made the results unreliable. 

The new design will most likely be less affected by this, but even then the CO2 sensor might produce wrong results as the CO2 concentration will drop by a lot. This will probably not happen during VO2max tests (remember CO2 is not really of concern when measuring the VO2max) due to high ventilation, but it can happen when you want to know the correct CO2 concentration which is when you are working aerobically.
So what I did is inserting a 3M exhalation valve (exactly the same blue one which is included in the 3M mask) in the front of the original design. This is a single part I just glued on for testing purposes.

## Calculations:
1. I honestly haven't checked each calculation but I'm relying on them to be accurate.  What I noticed is that the VO2max might be a bit too high. When I did my last test with an expensive spirometer the guy said that sometimes a "single breath can produce a higher VO2" which is why he doesn't use this value to determine the VO2max and instead uses the highest average of 30s or something like that. This should be checked.

2. The CO2 sensor accepts ambient pressure readings. I have implemented that in my version of the code as we have the BMP280 to provide that data.

## Code:
1. I have replaced the existing SCD30 library with another one as the CO2 sensor needs to be calibrated from time to time (and due to the pressure sensor addition). We are not using the automatic self calibration which normally should be used with SCD30. This automatic calibration assumes that in a given time the sensor will reach the atmospheric CO2 concentration at least once. Automatic self calibration takes at least many hours up to days, which is not suitable for our device. Instead I implemented a function to self calibrate the CO2 sensor by running it for at least 2 minutes in outside air. Then it'll calibrate to 421 ppm which is the current atmospheric CO2 concentration of 2023. Remember, due to increasing CO2 in the air the value will increase each year and therefore using a value of 400 ppm would already create an error of 5% during calibration (not something we want in an accurate device). 2 minutes might not be enough but this step can be repeated or I can increase the timer in the code.

2. For my own tests I've changed some of the BLE code. In the future I would like to output all useful data to an app similar to a bike computer. The implementation can be done on the bluetooth level by sending each value as a specific service. This can then be combined with powermeter data, heart rate data and so on and even be saved in a FIT file or something like that.

There is an amazing website for analyzing of these files (www.intervals.icu) and the developer is a great guy who always listens to new ideas and often implements them (quite quick actually!).
The app itself is a work in progress and just something I would use but I can share it if it'll ever be ready.

5. The user interface could use an overhaul. My idea is to create multiple sections on the initial screen to make it more clear (Start, Settings, Status) and by using submenus to navigate further.

6. The code has quite a lot of comments (thanks for that) which make it easy to understand. Still it could be reorganized a little. But that's not really important now.


I've probably forgot some points but I'll share them if I remember.
Let me know your thoughts.
