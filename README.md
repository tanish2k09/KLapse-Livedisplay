# KLapse - A kernel level livedisplay module

### What is it?
Kernel-based Lapse ("K-Lapse") is a linear RGB scaling module that 'shifts' RGB based on time (of the day/selected by user), or (since v2.0) brightness.
This concept is inspired from LineageOS (formerly known as 'CyanogenMod') ROM's feature "livedisplay" which also changes the display settings (RGB, hue, temperature, etc) based on time.

### Why did you decide to make this? (Tell me a story).
I (personally) am a big fan of the livedisplay feature found on LineageOS ROM. I used it every single day, since Android Lollipop.
Starting from Android Nougat, a native night mode solution was added to AOSP and it felt like livedisplay was still way superior,
thanks to its various options (you could say it spoiled me, sure). I also maintained a kernel (Venom kernel) for the device I was using at that time.
It was all good until the OEM dropped support for the device at Android M, and XDA being XDA, was already working on N ROMs.
The issue was, these ROMs weren't LineageOS or based on it, so livedisplay was... gone. I decided I'll try to bring that feature to every other ROM.
How would I do that? Of course! The kernel! It worked on every single 
ROM, it was the key! I started to work on it ASAP and here it is, up on 
GitHub, licensed under GPL (check klapse.c), open to everyone :)

### How does it work?
Think of it like a fancy night mode, but not really. Klapse is dependent on an RGB interface (like Gamma on MTK and KCAL on SD chipsets).
It fetches time from the kernel, converts it to local time, and selects and RGB set based on the time. The result is really smooth shifting of RGB over time.

### How does it really work (dev)?
Klapse mode 1 (time-based scaling) uses a method `void klapse_pulse(unsigned long data)` that should ideally be called every minute.
This is done using a kernel timer, that is asynchronous so it should be handled with care, which I did.
The pulse function fetches the current time and makes calculations based on the current hour and the values of the tunables listed down below.

Klapse mode 2 (brightness-based scaling) uses a method `void set_rgb_slider(<type> bl_lvl)` where <type> is the data type of the brightness level used in your kernel source.
(OnePlus 6 uses u32 data type for bl_lvl)
set_rgb_slider needs to be called/injected inside a function that sets brightness for your device.
(OnePlus 6 uses dsi_panel.c for that, check out the diff for that file in /op6)

### What all stuff can it do?
1. Emulate night mode with the proper RGB settings
2. Smoothly scale from one set of RGB to another set of RGB in integral intervals over time.
3. Reduce perceived brightness using brightness_factor by reducing the amount of color on screen. Allows lower apparent brightness than system permits.
4. Scale RGB based on brightness of display (low brightness usually implies a dark environment, where yellowness is probably useful).
5. Automate the perceived brightness independent of whether klapse is enabled, using its own set of start and stop hours.
6. Be more efficient,faster by residing inside the kernel instead of having to use the HWC HAL like android's night mode.
7. (On older devices) Reduce stuttering or frame lags caused by native night mode.
8. An easier solution against overlay-based apps that run as service in userspace/Android and sometimes block apps asking for permissions.
9. Give you a Livedisplay alternative if it doesn't work in your ROM.
10. Impress your crush so you can get a date (Hey, don't forget to credit me if it works).

### Alright, so this is a replacement for night mode?
NO! Not at all. One can say this is merely an alternative for LineageOS' Livedisplay, but inside a kernel. Night mode is a sub-function of both Livedisplay and KLapse.
Most comparisons here were made with night mode because that's what an average user uses, and will relate to the most.
There is absolutely no reason for your Android kernel to not have KLapse. Go ahead and add it or ask your kernel maintainer to. It's super-easy!

### What can it NOT do (yet)?
1. Calculate scaling to the level of minutes, like "Start from 5:37pm till 7:19am". --TODO
2. Make coffee for you.
3. Fly you to the moon. Without a heavy suit.
4. Get you a monthly subscription of free food, cereal included.

### I want more! Tell me what can I customize!
All these following tunables are found in their respective files in /sys/klapse/
```python
1. enable_klapse : A switch to enable or disable klapse. Values : 0 = off, 1 = on (since v2.0, 2 = brightness-dependent mode)
2. klapse_start_hour : The hour at which klapse should start scaling the RGB values from daytime to target (see next points). Values : 0-23
3. klapse_stop_hour : The hour by which klapse should scale back the RGB values from target to daytime (see next points). Values : 0-23
4. daytime_rgb : The RGB set that must be used for all the time outside of start and stop hour range.
5. target_rgb : The RGB set that must be scaled towards for all the time inside of start and stop hour range.
6. klapse_scaling_rate : Controls how soon the RGB reaches from daytime to target inside of start and stop hour range. Once target is reached, it remains constant till 30 minutes before stop hour, where target RGB scales back to daytime RGB.
7. brightness_factor : From the name itself, this value has the ability to bend perception and make your display appear as if it is at a lesser brightness level than it actually is at. It works by reducing the RGB values by the same factor. Values : 2-10, (10 means accurate brightness, 5 means 50% of current brightness, you get it)
8. brightness_factor_auto : A switch that allows you to automatically set the brightness factor in a set time range. Value : 0 = off, 1 = on
9. brightness_factor_auto_start_hour : The hour at which brightness_factor should be applied. Works only if #8 is 1. Values : 0-23
10. brightness_factor_auto_stop_hour : The hour at which brightness_factor should be reverted to 10. Works only if #8 is 1. Values : 0-23
11. backlight_range : The brightness range within which klapse should scale from daytime to target_rgb. Works only if #1 is 2. Values : MIN_BRIGHTNESS-MAX_BRIGHTNESS
12. pulse_freq : The amount of milliseconds after which klapse_pulse is called. A more developer-targeted tunable. Only works when one or both of #1 and #8 are 1. Values : 1000-600000 (Represents 1sec to 10 minutes
13. fadeback_minutes : The number of minutes before klapse_stop_hour when RGB should start going back to daytime_rgb. Only works when #1 is 1. Values : 2-minutes between #2 and #3
```
