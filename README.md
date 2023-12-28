---
# XPA125B - display Power/Output with ESP32 & websocket-server in a webbrowser
<p align="center" width="100%">
  <img width="33%" src="https://github.com/dl1bz/ESP32-XPA125B-Webcontrol/blob/main/XPA_in_browser.png">
  <img width="33%" src="https://github.com/dl1bz/ESP32-XPA125B-Webcontrol/blob/main/ESP32_T-DISPLAY_XPA.jpg">
</p>

## <A HREF="https://xiegu.eu/product/xpa125-100w-solid-state-linear-amplifier/">XPA125b from XIEGU</A>
The XPA125 is a solid-state power RF MOSFET amplifier for shortwave, build for using in hamradio:
- input 1..5W, perfect for QRP-TRX
- output up to 120W
- frequency range 160m up to 6m
- internal wide-range ATU if needed, can be switched off/bypass
- the PA hasn't any HF-VOX or automatic switching of filters, so we need a PTT-line (low-active) and analogue voltage input for auto-filter-select, otherwise you need to do filter-select by hand 
- Vcc 12V, max. current up to 16A, normally ~10A @ 100W output
- SWR and temperature protection functions
- used finals: 2x Mitsubishi RD100HHFC1
- last FW: 2.0b13

## Some pre-work and ideas...
First I found this project around the XPA125B, which was the kick-off for me, something to do with my XPA125B - not the same, but similar:
- <A HREF="https://github.com/madpsy/xpa125b_controller">https://github.com/madpsy/xpa125b_controller</A><p></p>

Here I found two great ideas, which I use as a base for development with my ESP32:
- <A HREF="https://randomnerdtutorials.com/esp32-websocket-server-arduino/">ESP32 WebSocket Server: Control Outputs</A>
- <A HREF="https://randomnerdtutorials.com/esp32-websocket-server-sensor/">ESP32 WebSocket Server: Display Sensor Readings</A>
<p></p>
Great work, great guidelines for do something like that I want.

## The Goal
If you work remote with your PA XIEGU XPA125B (in my case with my Hermes-Lite 2 in front of the XPA125B) you need to control/show, if there is output power of PA and how much.
In this project I modified my XPA125B and split the two signals for forward and reflected power, normally they put into the internal ADC of the STM-processor.
<p><img src="https://github.com/dl1bz/ESP32-XPA125B-Webcontrol/blob/main/Split_SWR_ADC_Output_XPA125B.jpg"></p>
I route the two signals (after splitting) to outside of the PA (via a 3.5mm TRRS socket I added backside of XPA125B) socket as input for two ADC from a ESP32 modul.
<p></p>
<p align="center" width="100%">
  <img width="33%" src="https://github.com/dl1bz/ESP32-XPA125B-Webcontrol/blob/main/XPA125B_Mod_SWR_voltages.JPG">
  <img width="33%" src="https://github.com/dl1bz/ESP32-XPA125B-Webcontrol/blob/main/XPA125B_Mod_SWR_voltages_details.JPG">
  <img width="33%" src="https://github.com/dl1bz/ESP32-XPA125B-Webcontrol/blob/main/TRRS_socket.JPG">
</p>
The ESP32 is connected to WiFi and has a running webserver with websocket technology. So we measure the signals from the XPA125B with two ADC inputs of ESP32, calculate the power values & SWR and push them in realtime to a website shown in a browser.

**If you modify your XPA125B, you do all at your own risk. You could also lost warranty.**

## Show status only or full control of XPA125B ?
I think about for a long time, largely inspired from project <A HREF="https://github.com/madpsy/xpa125b_controller">https://github.com/madpsy/xpa125b_controller</A> and do some additional investigations with my XPA125B. My base for working was the <A HREF="https://github.com/dl1bz/ESP32-XPA125B-Webcontrol/blob/main/XPA125B-MB-Sch-V1_03.pdf">schematic of XPA125B</A> and the <A HREF="https://github.com/dl1bz/ESP32-XPA125B-Webcontrol/blob/main/XPA125B-MB-Assembly.pdf">assembly of XPA125B</A>.
**The sad news is, I don't found any usable signal for status ON/BYPASS. Only the XPA-LCD-Display show this information.** Otherwise I don't need the internal ATU, because I use a CG3000 as external tuner outside.
So I decided for only show the forward and reflected output power in the webinterface. Thats enough information for me to control the PA if I work remote.

## Technical implementation
First I open my XPA125B and add an 3.5mm TRRS socket backside, there's enough space for doing this. I split the two signals (forward and reflected voltage), which are the ADC inputs for the onboard STM processor for measurement the status of output power coming from the SWR bridge. I also feed these signals into two separate ADC of the ESP32.
The measurement is an average calculation, we check 16 times the forward voltage and calculate an average value. Values with 0V were ignored and removed from the average calculation. The SWR is calculated too from the forward and reflected voltage SWR = (U_fwd + U_ref)/(U_fwd - U_ref). I setup the internal ADC of ESP32 to attenuation of 2.5db, so the measure voltage range will be min. 100mV ~ max. 1.25V. More than 1.0V I havn't seen as output voltage from the XPA125B. The ADC from the ESP32 are not the best, more information about this you can read here <A HREF="https://www.makerfabs.cc/article/cautions-in-using-esp32-adc.html">https://www.makerfabs.cc/article/cautions-in-using-esp32-adc.html</A>. However, the accuracy of the ESP32 ADC is sufficient for this project. I didn't calibrate the ADC of the ESP32 for this project, but it is possible.

**The output power shown at LCD display of XPA125B is not ever correct, so I try to do a kind of calibration.
Due to limits of the ADCs from ESP32 I need to say to, my measurement is not the exactest too.
I've done my best to compensate the difference. At the end I say, the accuracy is good enough for normal use related to this project.
I only need the approximate output, but not the 100% exactest. This is not intended to be a measuring device with calibrated values :-)**

At the ESP32 I run an small webserver with websocket support, so we have the option to push the calculated power values **in realtime** in the webbrowser we use as display. The ESP32 itself is connected via WiFi at local network in station mode ("STA"), so we can access it's webserver this way. We need a DHCP server, otherwise you need to change the WiFi implementaion for using a static IP. Look at the WiFi-API description of ESP32 devices for more information. 
Additional debug output via it's USB UART (115200,8,N,1) is also present. If you use an ESP32 T-DISPLAY like me, there is an option in source code to activate the onboard TFT display, which also show some information.
We need 3 wires between XPA and ESP32, 2x SWR voltage and GND. If you want, you can also use in addition the internal +5V coming from the XPA as power supply the ESP32.

With some code changes it's possible, running the ESP32 and it's WiFi as access point. Then the ESP32 generate an WiFi network by itself, which you can connect with your computer or smartphone. That's useful - maybe - if you use your rig as portable station. But that doesn't make any sense, because in this case you can look at the LCD display from the XPA125B. I made this all for running as remote station with my Hermes-Lite 2, where I can see the status of the PA if I'm out of home.

## If not using an ESP32 LILYGO TTGO T-DISPLAY
I made the code for my ESP32 LILYGO TTGO T-DISPLAY **(but not the newer T-DISPLAY S3, the used TFT-Lib is not compatible with it so far as I know)**. If you use any other, "display-less" ESP32, please change<br>
<code>#define TDISPLAY</code><br>
to<br>
<code>// #define TDISPLAY</code><br>
Look in code too, if you need to change the PINs for ADC input with your ESP32:<br>
<code>// define ADC input PINs for T-DISPLAY, don't forget to connect GND XPA<->ESP32 in addition !
const int PoutPin = 32; // ADC forward power from XPA125B
const int PrefPin = 33; // ADC reflected power from XPA125B</code>

## The code
This program is free software only for personal use; you can redistribute it and/or modify it how you want.
The codebase is "as is" without any kind of support of me. It's NOT for commercial use in any case.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

73 Heiko, DL1BZ
