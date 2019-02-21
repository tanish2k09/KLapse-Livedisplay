#include <linux/module.h>
#include <linux/init.h>
#include "klapse.h"
#include "sde_hw_color_proc_v4.h"
#include <uapi/linux/time.h>
#include <uapi/linux/rtc.h>
#include <linux/rtc.h>


/* DEFAULT_ENABLE values :
 * 0 = off
 * 1 = time-based scaling
 * 2 = brightness-based scaling
 */
#define DEFAULT_ENABLE  0

// MAX_SCALE : Maximum value of RGB possible
#define MAX_SCALE       256

// SCALE_VAL_MIN : Minimum value of RGB recommended
#define SCALE_VAL_MIN   20

// MAX_BRIGHTNESS : Maximum value of the display brightness/backlight
#define MAX_BRIGHTNESS  1023

// MIN_BRIGHTNESS : Minimum value of the display brightness/backlight
#define MIN_BRIGHTNESS  2

/* UPPER_BL_LVL : Initial upper limit for brightness-dependent mode. 
 * Value <= MAX_BRIGHTNESS && > LOWER_BL_LVL (MUST)
 */
#define UPPER_BL_LVL  200

/* LOWER_BL_LVL : Initial lower limit for brightness-dependent mode. 
 * Value < UPPER_BL_LVL (MUST)
 */
#define LOWER_BL_LVL 2

#define LIC "GPLv2"
#define AUT "tanish2k09"
#define VER "2.1"

MODULE_LICENSE(LIC);
MODULE_AUTHOR(AUT);
MODULE_DESCRIPTION("A simple rgb dynamic lapsing module similar to livedisplay");
MODULE_VERSION(VER);


//Tunables :
unsigned int daytime_r, daytime_g, daytime_b, target_r, target_g, target_b;
unsigned int klapse_start_hour, klapse_stop_hour, enable_klapse;
unsigned int brightness_factor_auto_start_hour, brightness_factor_auto_stop_hour;
unsigned int klapse_scaling_rate, brightness_factor;
unsigned int backlight_lower, backlight_upper;
bool brightness_factor_auto_enable;

/*
 *Internal calculation variables :
 *WARNING : DO NOT MAKE THEM TUNABLE
 */
static bool target_achieved;
static unsigned int b_cache;
static int current_r, current_g, current_b;
static unsigned int active_minutes, last_bl;
static unsigned long local_time;
struct rtc_time tm;
static struct timeval time;


//klapse related functions
void calc_active_minutes(void)
{
    if(klapse_start_hour > klapse_stop_hour)
        active_minutes = (24 + klapse_stop_hour - klapse_start_hour)*60;
    else
        active_minutes = (klapse_stop_hour - klapse_start_hour)*60;
}

static int get_minutes_since_start(void)
{
    int hour, min;
    hour = tm.tm_hour - klapse_start_hour;

    if (hour < 0)
        hour += 24;

    min = ((hour*60) + tm.tm_min);
    return min;
}

static int get_minutes_before_stop(void)
{
    return (active_minutes - get_minutes_since_start());
}

static void set_rgb(int r, int g, int b)
{
    kcal_red = r;
    kcal_green = g;
    kcal_blue = b;
    
    current_r = r;
    current_g = g;
    current_b = b;
}

static void set_rgb_brightness(int r,int g,int b)
{

    r = ((r*brightness_factor)/10);
    g = ((g*brightness_factor)/10);
    b = ((b*brightness_factor)/10);

    if (r < 0)
        r = SCALE_VAL_MIN;
    else if (r > MAX_SCALE)
        r = MAX_SCALE;
    if (g < 0)
        g = SCALE_VAL_MIN;
    else if (g > MAX_SCALE)
        g = MAX_SCALE;
    if (b < 0)
        b = SCALE_VAL_MIN;
    else if (b > MAX_SCALE)
        b = MAX_SCALE;

    set_rgb(r,g,b);
}

static bool hour_within_range(int start, int stop, int check)
{
    // The 24-hour system is tricky because 0 comes after 23.
    // Handle it here properly

    // Check whether the start hour comes before stop hour.
    // if = 1, this would mean they are numerically in order
    // if both extremes are same, no time is possible inside so return false.
    // else, start hour is actually greater than stop, something like "From 5pm to 7am"
    // which translates to "From 17 to 7". It is clear why this could be a problem if not handled.
    if (start < stop) {
      if ((check >= start) && (check < stop))
        return 1;
      else
        return 0;
    }
    else if (start == stop)
      return 0;
    else {
      if ((check < stop) || (check >= start))
        return 1;
      else
        return 0;
    }
}
//klapse calc functions end here.

// klapse rgb update function
void klapse_pulse(void)
{
    // Get time
    do_gettimeofday(&time);
    local_time = (u32)(time.tv_sec - (sys_tz.tz_minuteswest * 60));
    rtc_time_to_tm(local_time, &tm);

    // Check brightness level automation
    if (brightness_factor_auto_enable == 1)
    {
        if (hour_within_range(brightness_factor_auto_start_hour, brightness_factor_auto_stop_hour, tm.tm_hour) == 0) //Means not in dimmer-time
            brightness_factor = 10;
        else
            brightness_factor = b_cache;
    }
    else
        brightness_factor = b_cache;

    // Check klapse automation
    if (enable_klapse == 1)
    {
        if(hour_within_range(klapse_start_hour, klapse_stop_hour, tm.tm_hour) == 0) //Means not in klapse time period.
        {
            set_rgb_brightness(daytime_r,daytime_g,daytime_b);
            target_achieved = 0;
        }
        else if (target_achieved == 0)
        {
            current_r = daytime_r - (((daytime_r - target_r)*(get_minutes_since_start())*klapse_scaling_rate)/(active_minutes*10));
            current_g = daytime_g - (((daytime_g - target_g)*(get_minutes_since_start())*klapse_scaling_rate)/(active_minutes*10));
            current_b = daytime_b - (((daytime_b - target_b)*(get_minutes_since_start())*klapse_scaling_rate)/(active_minutes*10));

            if ((current_r <= target_r) && (current_g <= target_g) && (current_b <= target_b))
            {
                target_achieved = 1;
                current_r = target_r;
                current_g = target_g;
                current_b = target_b;
            }

            set_rgb_brightness(current_r, current_g, current_b);
        }
        else if (target_achieved == 1)
        {
            int backtime = get_minutes_before_stop();
            if(backtime <= 30)
            {
                current_r = target_r + (((daytime_r - target_r)*(30 - backtime))/30);
                current_g = target_g + (((daytime_g - target_g)*(30 - backtime))/30);
                current_b = target_b + (((daytime_b - target_b)*(30 - backtime))/30);

                set_rgb_brightness(current_r, current_g, current_b);
            }
        }
    }
}

// Brightness-based mode
void set_rgb_slider(u32 bl_lvl)
{
  if (bl_lvl >= MIN_BRIGHTNESS)
  {
    if ((enable_klapse == 2) && (bl_lvl <= MAX_BRIGHTNESS))
    {
      if (bl_lvl > backlight_upper)
        set_rgb_brightness(daytime_r, daytime_g, daytime_b);
      else if (bl_lvl < backlight_lower)
        set_rgb_brightness(target_r, target_g, target_b);
      else {
        current_r = daytime_r - ((daytime_r - target_r)*(backlight_upper - bl_lvl)/(backlight_upper - backlight_lower));
        current_g = daytime_g - ((daytime_g - target_g)*(backlight_upper - bl_lvl)/(backlight_upper - backlight_lower));
        current_b = daytime_b - ((daytime_b - target_b)*(backlight_upper - bl_lvl)/(backlight_upper - backlight_lower));
        set_rgb_brightness(current_r, current_g, current_b);
      }
    }
  
    last_bl = bl_lvl;
  }
}

void set_enable_klapse(int val)
{
    if ((val <= 2) && (val >= 0))
    {
        enable_klapse = val;
        if (enable_klapse != 1)
        {
            set_rgb_brightness(daytime_r, daytime_g, daytime_b);
            target_achieved = 0;
            current_r = daytime_r;
            current_g = daytime_g;
            current_b = daytime_b;
        }
    }
}

//SYSFS node for details :
static ssize_t info_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "Author : %s\nVersion : %s\nLicense : %s\n", AUT, VER, LIC);

  return count;
}

static ssize_t info_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    return count;
}

//SYSFS tunables :
static ssize_t enable_klapse_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", enable_klapse);

  return count;
}

static ssize_t enable_klapse_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    set_enable_klapse(tmpval);

    return count;
}

static ssize_t daytime_r_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", daytime_r);

  return count;
}

static ssize_t daytime_r_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval > (SCALE_VAL_MIN)) && (tmpval <= MAX_SCALE))
    {
        daytime_r = tmpval;
        if ((enable_klapse == 0) || ((enable_klapse == 1) && (tm.tm_hour < klapse_start_hour) && (tm.tm_hour >=klapse_stop_hour)))
           set_rgb_brightness(daytime_r, daytime_g, daytime_b);
    }

    return count;
}

static ssize_t daytime_g_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", daytime_g);

  return count;
}

static ssize_t daytime_g_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
        return -EINVAL;

    if ((tmpval > (SCALE_VAL_MIN)) && (tmpval <= MAX_SCALE))
    {
        daytime_g = tmpval;
        if ((enable_klapse == 0) || ((enable_klapse == 1) && (tm.tm_hour < klapse_start_hour) && (tm.tm_hour >=klapse_stop_hour)))
          set_rgb_brightness(daytime_r, daytime_g, daytime_b);
    }

    return count;
}

static ssize_t daytime_b_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", daytime_b);

  return count;
}

static ssize_t daytime_b_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval > (SCALE_VAL_MIN)) && (tmpval <= MAX_SCALE))
    {
        daytime_b = tmpval;
        if ((enable_klapse == 0) || ((enable_klapse == 1) && (tm.tm_hour < klapse_start_hour) && (tm.tm_hour >=klapse_stop_hour)))
          set_rgb_brightness(daytime_r, daytime_g, daytime_b);
    }

    return count;
}

static ssize_t target_r_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", target_r);

  return count;
}

static ssize_t target_r_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval > (SCALE_VAL_MIN)) && (tmpval <= MAX_SCALE))
    {
        target_r = tmpval;
        target_achieved = 0;
    }

    return count;
}

static ssize_t target_g_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", target_g);

  return count;
}

static ssize_t target_g_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval > (SCALE_VAL_MIN)) && (tmpval <= MAX_SCALE))
    {
        target_g = tmpval;
        target_achieved = 0;
    }

    return count;
}

static ssize_t target_b_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", target_b);

  return count;
}

static ssize_t target_b_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval > (SCALE_VAL_MIN)) && (tmpval <= MAX_SCALE))
    {
        target_b = tmpval;
        target_achieved = 0;
    }

    return count;
}

static ssize_t klapse_start_hour_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", klapse_start_hour);

  return count;
}

static ssize_t klapse_start_hour_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval >= 0) && (tmpval < 24) && (tmpval != klapse_stop_hour))
    {
        klapse_start_hour = tmpval;
        target_achieved = 0;
        calc_active_minutes();
    }

    return count;
}

static ssize_t klapse_stop_hour_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", klapse_stop_hour);

  return count;
}

static ssize_t klapse_stop_hour_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval >= 0) && (tmpval < 24) && (tmpval != klapse_start_hour))
    {
        klapse_stop_hour = tmpval;
        target_achieved = 0;
        calc_active_minutes();
    }

    return count;
}

static ssize_t klapse_scaling_rate_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", klapse_scaling_rate);

  return count;
}

static ssize_t klapse_scaling_rate_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval >= 0) && (tmpval < (MAX_SCALE*10)))
    {
        klapse_scaling_rate = tmpval;
        target_achieved = 0;
    }

    return count;
}

static ssize_t brightness_factor_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", brightness_factor);

  return count;
}

static ssize_t brightness_factor_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval >= 2) && (tmpval <= 10))
    {
        b_cache = tmpval;
        target_achieved = 0;
        if (brightness_factor_auto_enable == 0)
          set_rgb_brightness(current_r, current_g, current_b);
    }

    return count;
}


static ssize_t brightness_factor_auto_enable_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", brightness_factor_auto_enable);

  return count;
}

static ssize_t brightness_factor_auto_enable_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval == 0) || (tmpval == 1))
    {
        brightness_factor_auto_enable = tmpval;
        target_achieved = 0;
    }

    return count;
}

static ssize_t brightness_factor_auto_start_hour_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", brightness_factor_auto_start_hour);

  return count;
}

static ssize_t brightness_factor_auto_start_hour_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval >= 0) && (tmpval < 24) && (tmpval != brightness_factor_auto_stop_hour))
    {
        brightness_factor_auto_start_hour = tmpval;
        target_achieved = 0;
    }

    return count;
}

static ssize_t brightness_factor_auto_stop_hour_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", brightness_factor_auto_stop_hour);

  return count;
}

static ssize_t brightness_factor_auto_stop_hour_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval >= 0) && (tmpval < 24) && (tmpval != brightness_factor_auto_start_hour))
    {
        brightness_factor_auto_stop_hour = tmpval;
        target_achieved = 0;
    }

    return count;
}

static ssize_t backlight_lower_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", backlight_lower);

  return count;
}

static ssize_t backlight_lower_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval > 0) && (tmpval < backlight_upper))
    {
        backlight_lower = tmpval;
        set_rgb_slider(last_bl);
    }

    return count;
}

static ssize_t backlight_upper_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  size_t count = 0;

  count += sprintf(buf, "%d\n", backlight_upper);

  return count;
}

static ssize_t backlight_upper_dump(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;

    if (!sscanf(buf, "%d", &tmpval))
      return -EINVAL;

    if ((tmpval > 0) && (tmpval > backlight_lower))
    {
        backlight_upper = tmpval;
        set_rgb_slider(last_bl);
    }

    return count;
}


static DEVICE_ATTR(enable_klapse, 0644, enable_klapse_show, enable_klapse_dump);
static DEVICE_ATTR(daytime_r, 0644, daytime_r_show, daytime_r_dump);
static DEVICE_ATTR(daytime_g, 0644, daytime_g_show, daytime_g_dump);
static DEVICE_ATTR(daytime_b, 0644, daytime_b_show, daytime_b_dump);
static DEVICE_ATTR(target_r, 0644, target_r_show, target_r_dump);
static DEVICE_ATTR(target_g, 0644, target_g_show, target_g_dump);
static DEVICE_ATTR(target_b, 0644, target_b_show, target_b_dump);
static DEVICE_ATTR(klapse_start_hour, 0644, klapse_start_hour_show, klapse_start_hour_dump);
static DEVICE_ATTR(klapse_stop_hour, 0644, klapse_stop_hour_show, klapse_stop_hour_dump);
static DEVICE_ATTR(klapse_scaling_rate, 0644, klapse_scaling_rate_show, klapse_scaling_rate_dump);
static DEVICE_ATTR(brightness_factor, 0644, brightness_factor_show, brightness_factor_dump);
static DEVICE_ATTR(brightness_factor_auto, 0644, brightness_factor_auto_enable_show, brightness_factor_auto_enable_dump);
static DEVICE_ATTR(brightness_factor_auto_start_hour, 0644, brightness_factor_auto_start_hour_show, brightness_factor_auto_start_hour_dump);
static DEVICE_ATTR(brightness_factor_auto_stop_hour, 0644, brightness_factor_auto_stop_hour_show, brightness_factor_auto_stop_hour_dump);
static DEVICE_ATTR(backlight_lower, 0644, backlight_lower_show, backlight_lower_dump);
static DEVICE_ATTR(backlight_upper, 0644, backlight_upper_show, backlight_upper_dump);
static DEVICE_ATTR(info, 0444, info_show, info_dump);

//INIT
static void values_setup(void)
{
    daytime_r = MAX_SCALE;
    daytime_g = MAX_SCALE;
    daytime_b = MAX_SCALE;
    current_r = MAX_SCALE;
    current_g = MAX_SCALE;
    current_b = MAX_SCALE;
    target_r = MAX_SCALE;
    target_g = (MAX_SCALE*79)/100;
    target_b = (MAX_SCALE*59)/100;
    brightness_factor = 10;
    b_cache = brightness_factor;
    klapse_scaling_rate = 30;
    klapse_start_hour = 17;
    klapse_stop_hour = 7;
    brightness_factor_auto_start_hour = 23;
    brightness_factor_auto_stop_hour = 6;
    enable_klapse = DEFAULT_ENABLE;
    target_achieved = 0;
    brightness_factor_auto_enable = 0;
    calc_active_minutes();
    backlight_lower = LOWER_BL_LVL;
    backlight_upper = UPPER_BL_LVL;
    last_bl = 1023;

    do_gettimeofday(&time);
    local_time = (u32)(time.tv_sec - (sys_tz.tz_minuteswest * 60));
    rtc_time_to_tm(local_time, &tm);
}

struct kobject *klapse_kobj;
EXPORT_SYMBOL_GPL(klapse_kobj);

static int __init klapse_init(void)
{
    int rc;
    values_setup();

    klapse_kobj = kobject_create_and_add("klapse", NULL) ;
    if (klapse_kobj == NULL) {
      pr_warn("%s: klapse_kobj create_and_add failed\n", __func__);
    }

    rc = sysfs_create_file(klapse_kobj, &dev_attr_info.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for info\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_enable_klapse.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for enable_klapse\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_daytime_r.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for daytime_r\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_daytime_g.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for daytime_g\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_daytime_b.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for daytime_b\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_target_r.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for target_r\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_target_g.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for target_g\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_target_b.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for target_b\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_klapse_start_hour.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for klapse_start_hour\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_klapse_stop_hour.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for klapse_stop_hour\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_klapse_scaling_rate.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for klapse_scaling_rate\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_brightness_factor.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for brightness_factor\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_brightness_factor_auto.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for brightness_factor_auto\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_brightness_factor_auto_start_hour.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for brightness_factor_auto_start_hour\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_brightness_factor_auto_stop_hour.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for brightness_factor_auto_stop_hour\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_backlight_lower.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for backlight_lower\n", __func__);
    }
    rc = sysfs_create_file(klapse_kobj, &dev_attr_backlight_upper.attr);
    if (rc) {
      pr_warn("%s: sysfs_create_file failed for backlight_upper\n", __func__);
    }

    return 0;
}

static void __exit klapse_exit(void){
    kobject_del(klapse_kobj);
}

module_init(klapse_init);
module_exit(klapse_exit);
