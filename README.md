# KLapse - A kernel level livedisplay module

### What is it?
Kernel-based Lapse ("K-Lapse") is a linear RGB scaling module that 'shifts' RGB based on time (of the day/selected by user).
This concept is inspired from LineageOS (formerly known as 'CyanogenMod') ROM's feature "livedisplay" which also changes the display settings (RGB, hue, temperature, etc) based on time.

### Why did I decide to make this? Tell me a story.
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

### I want more! Tell me what can I customize!
```python
1. enable_klapse : A switch to enable or disable klapse. Values : 0 = off, 1 = on (in v2, 2 = brightness-dependent mode)
2. klapse_start_hour : The hour at which klapse should start scaling the RGB values from daytime to target (see next points). Values : 0-23
3. klapse_stop_hour : The hour by which klapse should scale back the RGB values from target to daytime (see next points). Values : 0-23
4. daytime_r, g, b : The RGB set that must be used for all the time outside of start and stop hour range.
5. target_r, g, b : The RGB set that must be scaled towards for all the time inside of start and stop hour range.
6. klapse_scaling_rate : Controls how soon the RGB reaches from daytime to target inside of start and stop hour range. Once target is reached, it remains constant till 30 minutes before stop hour, where target RGB scales back to daytime RGB.
7. brightness_factor : From the name itself, this value has the ability to bend perception and make your display appear as if it is at a lesser brightness level than it actually is at. It works by reducing the RGB values by the same factor. Values : 2-10, (10 means accurate brightness, 5 means 50% of current brightness, you get it)
8. brightness_factor_auto : A switch that allows you to automatically set the brightness factor in a set time range. Value : 0 = off, 1 = on
9. brightness_factor_auto_start_hour : The hour at which brightness_factor should be applied. Works only if #8 is 1. Values : 0-23
10. brightness_factor_auto_stop_hour : The hour at which brightness_factor should be reverted to 10. Works only if #8 is 1. Values : 0-23
```
