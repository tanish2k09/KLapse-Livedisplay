#ifndef _LINUX_KLAPSE_H
#define _LINUX_KLAPSE_H

//Required variables for external access
extern void klapse_pulse(void);
extern unsigned int enable_klapse;
extern bool brightness_factor_auto_enable;
extern void set_rgb_slider(u32 bl_lvl);

#endif  /* _LINUX_KLAPSE_H */
