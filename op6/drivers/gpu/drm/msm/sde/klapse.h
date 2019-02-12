#ifndef _LINUX_KLAPSE_H
#define _LINUX_KLAPSE_H

#include <uapi/linux/time.h>
#include <uapi/linux/rtc.h>
#include <linux/rtc.h>

#define MAX_SCALE 256
#define SCALE_VAL_MIN 20
//#define DEFAULT_BRIGHTNESS_THRESHOLD 40

/* Snapdragon support for KCAL can be made available by creating these two functions linked to KCAL rgb control
 * This needs some careful working around some KCAL methods to avoid conflicts and not make both of them available
 * to be toggled on at the same time from filesystem
 */

// KLapse variables
// For MTK these functions are created in mtk_disp_mgr in misc/mediatek/video/mt6752 folder 
extern void force_livedisplay_set_rgb(int force_r, int force_g, int force_b);
//extern void update_disp_mgr(bool force_l);

#endif	/* _LINUX_KLAPSE_H */
