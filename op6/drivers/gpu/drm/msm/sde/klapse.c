#include <linux/module.h> 
#include <linux/init.h> 
#include "klapse.h"
#include "sde_hw_color_proc_v4.h"

MODULE_LICENSE("GPLv3");
MODULE_AUTHOR("tanish2k09");
MODULE_DESCRIPTION("A simple rgb dynamic lapsing module similar to livedisplay");
MODULE_VERSION("1.0"); 


//Tunables :
int daytime_r, daytime_g, daytime_b, target_r, target_g, target_b;
int livedisplay_start_hour, livedisplay_stop_hour, force_livedisplay;
int brightness_lvl_auto_hour_start, brightness_lvl_auto_hour_end;
unsigned int livedisplay_aggression, brightness_lvl, klapse_brightness_threshold;
/* threshold var is linked to leds.c for dynamic brightness transition */
bool brightness_lvl_auto_enable;

/*
 *Internal calculation variables :
 *WARNING : DO NOT MAKE THEM TUNABLE
 */
static int current_r, current_g, current_b;
unsigned int b_cache;
static unsigned int active_minutes;
bool target_achieved;
static struct timeval time;
static unsigned long local_time;
struct rtc_time tm;


//Livedisplay related functions
void calc_active_minutes(void)
{
    if(livedisplay_start_hour > livedisplay_stop_hour)
        active_minutes = (24 + livedisplay_stop_hour - livedisplay_start_hour)*60;
    else
        active_minutes = (livedisplay_stop_hour - livedisplay_start_hour)*60;    
}

static int get_minutes_since_start(void)
{
    int hour, min;
    hour = tm.tm_hour - livedisplay_start_hour;
    
    if (hour < 0)
        hour += 24;
        
    min = ((hour*60) + tm.tm_min);
    return min;
}

static int get_minutes_before_stop(void)
{
    return (active_minutes - get_minutes_since_start());
}

void force_livedisplay_set_rgb(int r, int g, int b)
{
    kcal_red = r;
    kcal_green = g;
    kcal_blue = b;
}

void force_livedisplay_set_rgb_brightness(int r,int g,int b)
{
    
    r = ((r*brightness_lvl)/10);
    g = ((g*brightness_lvl)/10);
    b = ((b*brightness_lvl)/10);
    
    if (r < 0)
        r = SCALE_VAL_MIN;
    else if (r > daytime_r)
        r = daytime_r;
    if (g < 0)
        g = SCALE_VAL_MIN;
    else if (g > daytime_g)
        g = daytime_g;
    if (b < 0)
        b = SCALE_VAL_MIN;
    else if (b > daytime_b)
        b = daytime_b;
    
    force_livedisplay_set_rgb(r,g,b);
}
//Livedisplay calc functions end here.

// LiveDisplay lapse rgb update function
void klapse_pulse(void)
{
    if (force_livedisplay == 1)
    {
        // Get time
      	do_gettimeofday(&time);
        local_time = (u32)(time.tv_sec - (sys_tz.tz_minuteswest * 60));
        rtc_time_to_tm(local_time, &tm);
        
        //First we check for brightness scale automation...
        if (brightness_lvl_auto_enable == 1)
        {
            if(!((tm.tm_hour >= brightness_lvl_auto_hour_start) || (tm.tm_hour < brightness_lvl_auto_hour_end))) //Means not in dimmer-time
                brightness_lvl = 10;
            else
                brightness_lvl = b_cache;
        }
        else
            brightness_lvl = b_cache;	            
            
        
        if(!((tm.tm_hour >= livedisplay_start_hour) || (tm.tm_hour < livedisplay_stop_hour))) //Means not in livedisplay time period.
        {
            force_livedisplay_set_rgb_brightness(daytime_r,daytime_g,daytime_b); 
            target_achieved = 0;
        }        
        else if (target_achieved == 0)
        {
            current_r = daytime_r - (((daytime_r - target_r)*(get_minutes_since_start())*livedisplay_aggression)/(active_minutes*10));
            current_g = daytime_g - (((daytime_g - target_g)*(get_minutes_since_start())*livedisplay_aggression)/(active_minutes*10));
            current_b = daytime_b - (((daytime_b - target_b)*(get_minutes_since_start())*livedisplay_aggression)/(active_minutes*10));
            
            if ((current_r <= target_r) && (current_g <= target_g) && (current_b <= target_b))
            {
                target_achieved = 1;
                current_r = target_r;
                current_g = target_g;
                current_b = target_b;
            }    
            
            force_livedisplay_set_rgb_brightness(current_r, current_g, current_b);
       
        }
        else if (target_achieved == 1)
        {
            int backtime = get_minutes_before_stop();
            if(backtime <= 30)
            {                    
                current_r = target_r + (((daytime_r - target_r)*(30 - backtime))/30);
                current_g = target_g + (((daytime_g - target_g)*(30 - backtime))/30);
                current_b = target_b + (((daytime_b - target_b)*(30 - backtime))/30);
                
                force_livedisplay_set_rgb_brightness(current_r, current_g, current_b);
            }                                        
        }                
    }
}

void set_force_livedisplay(int val)
{
    // Val 2 REQUIRES BRIGHTNESS INTEGRATION
    // if( (val <= 2) && (val >= 0) )
    if( (val < 2) && (val >= 0) )
    {
        force_livedisplay = val;
        if (force_livedisplay != 1)
        {
            force_livedisplay_set_rgb_brightness(daytime_r, daytime_g, daytime_b);
            target_achieved = 0;
            current_r = daytime_r;
            current_g = daytime_g;
            current_b = daytime_b;                           
        }
        /// update_disp_mgr((val > 0 ? 1 : 0)); Maybe NOT required
    }
}

/* REQUIRES FUNCTIONAL BRIGHTNESS INTEGRATION

void klapse_set_rgb_daytime_target(bool target)
{
    if(!target)
        force_livedisplay_set_rgb_brightness(daytime_r,daytime_g,daytime_b);
    else
        force_livedisplay_set_rgb_brightness(target_r,target_g,target_b);
    
}
*/

void daytime_rgb_updated(int r,int g, int b)
{
    daytime_r = r;
    daytime_g = g;
    daytime_b = b;    
	if ((force_livedisplay == 0) || ((force_livedisplay == 1) && (tm.tm_hour < livedisplay_start_hour) && (tm.tm_hour >=livedisplay_stop_hour))) 
	    force_livedisplay_set_rgb_brightness(daytime_r, daytime_g, daytime_b);
}

//SYSFS tunables :
static ssize_t force_livedisplay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count = 0;

	count += sprintf(buf, "%d\n", force_livedisplay);

	return count;
}

static ssize_t force_livedisplay_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;    
    
	if (!sscanf(buf, "%d", &tmpval))
		return -EINVAL;

    set_force_livedisplay(tmpval);
    
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
		if ((force_livedisplay == 0) || ((force_livedisplay == 1) && (tm.tm_hour < livedisplay_start_hour) && (tm.tm_hour >=livedisplay_stop_hour))) 
		    force_livedisplay_set_rgb_brightness(daytime_r, daytime_g, daytime_b);
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
		if ((force_livedisplay == 0) || ((force_livedisplay == 1) && (tm.tm_hour < livedisplay_start_hour) && (tm.tm_hour >=livedisplay_stop_hour))) 
		    force_livedisplay_set_rgb_brightness(daytime_r, daytime_g, daytime_b);
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
		if ((force_livedisplay == 0) || ((force_livedisplay == 1) && (tm.tm_hour < livedisplay_start_hour) && (tm.tm_hour >=livedisplay_stop_hour))) 
		    force_livedisplay_set_rgb_brightness(daytime_r, daytime_g, daytime_b);
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

static ssize_t livedisplay_start_hour_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count = 0;

	count += sprintf(buf, "%d\n", livedisplay_start_hour);

	return count;
}

static ssize_t livedisplay_start_hour_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;    
    
	if (!sscanf(buf, "%d", &tmpval))
		return -EINVAL;

    if ((tmpval >= 0) && (tmpval < 24))
    {
        livedisplay_start_hour = tmpval;
        target_achieved = 0;
        calc_active_minutes();
    }
    
	return count;
}

static ssize_t livedisplay_stop_hour_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count = 0;

	count += sprintf(buf, "%d\n", livedisplay_stop_hour);

	return count;
}

static ssize_t livedisplay_stop_hour_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;    
    
	if (!sscanf(buf, "%d", &tmpval))
		return -EINVAL;

    if ((tmpval >= 0) && (tmpval < 24) && (tmpval != livedisplay_start_hour))
    {
        livedisplay_stop_hour = tmpval;
        target_achieved = 0;
        calc_active_minutes();
    }
    
	return count;
}

static ssize_t livedisplay_aggression_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count = 0;

	count += sprintf(buf, "%d\n", livedisplay_aggression);

	return count;
}

static ssize_t livedisplay_aggression_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;    
    
	if (!sscanf(buf, "%d", &tmpval))
		return -EINVAL;

    if ((tmpval >= 0) && (tmpval < (MAX_SCALE*5)))
    {
        livedisplay_aggression = tmpval;
        target_achieved = 0;
    }
    
	return count;
}

static ssize_t brightness_lvl_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count = 0;

	count += sprintf(buf, "%d\n", brightness_lvl);

	return count;
}

static ssize_t brightness_lvl_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;    
    
	if (!sscanf(buf, "%d", &tmpval))
		return -EINVAL;

    if ((tmpval > 2) && (tmpval <= 10))
    {
        b_cache = tmpval;
        target_achieved = 0;
    }
    
	return count;
}


static ssize_t brightness_lvl_auto_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count = 0;

	count += sprintf(buf, "%d\n", brightness_lvl_auto_enable);

	return count;
}

static ssize_t brightness_lvl_auto_enable_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;    
    
	if (!sscanf(buf, "%d", &tmpval))
		return -EINVAL;

    if ((tmpval == 0) || (tmpval == 1))
    {
        brightness_lvl_auto_enable = tmpval;
        target_achieved = 0;
    }
    
	return count;
}

/* REQUIRES BRIGHTNESS INTEGRATION

static ssize_t klapse_brightness_threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count = 0;

	count += sprintf(buf, "%d\n", klapse_brightness_threshold);

	return count;
}

static ssize_t klapse_brightness_threshold_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
    int tmpval = 0;    
    
	if (!sscanf(buf, "%d", &tmpval))
		return -EINVAL;

    if ((tmpval > 0) || (tmpval <= 255))
    {
        klapse_brightness_threshold = tmpval;
        if(force_livedisplay == 2)
            force_livedisplay_set_rgb(daytime_r, daytime_g, daytime_b);
        target_achieved = 0;
    }
    
	return count;
}
*/



static DEVICE_ATTR(enable_klapse_livedisplay, 0644, force_livedisplay_show, force_livedisplay_dump);
static DEVICE_ATTR(daytime_r, 0644, daytime_r_show, daytime_r_dump);
static DEVICE_ATTR(daytime_g, 0644, daytime_g_show, daytime_g_dump);
static DEVICE_ATTR(daytime_b, 0644, daytime_b_show, daytime_b_dump);
static DEVICE_ATTR(target_r, 0644, target_r_show, target_r_dump);
static DEVICE_ATTR(target_g, 0644, target_g_show, target_g_dump);
static DEVICE_ATTR(target_b, 0644, target_b_show, target_b_dump);
static DEVICE_ATTR(klapse_start_hour, 0644, livedisplay_start_hour_show, livedisplay_start_hour_dump);
static DEVICE_ATTR(klapse_stop_hour, 0644, livedisplay_stop_hour_show, livedisplay_stop_hour_dump);
static DEVICE_ATTR(klapse_scaling_rate, 0644, livedisplay_aggression_show, livedisplay_aggression_dump);
static DEVICE_ATTR(brightness_multiplier, 0644, brightness_lvl_show, brightness_lvl_dump);
static DEVICE_ATTR(brightness_multiplier_auto, 0644, brightness_lvl_auto_enable_show, brightness_lvl_auto_enable_dump);
//static DEVICE_ATTR(klapse_brightness_threshold, 0666, klapse_brightness_threshold_show, klapse_brightness_threshold_dump);



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
    target_g = (MAX_SCALE*10)/13;
    target_b = (MAX_SCALE*10)/21;
    brightness_lvl = 10;
    b_cache = 10;
    active_minutes = 600;
    livedisplay_aggression = 40;
    livedisplay_start_hour = 17;
    livedisplay_stop_hour = 7;
    brightness_lvl_auto_hour_start = 23;
    brightness_lvl_auto_hour_end = 6;
    force_livedisplay = 0;
    target_achieved = 0;
    brightness_lvl_auto_enable = 0;
    //klapse_brightness_threshold = DEFAULT_BRIGHTNESS_THRESHOLD;
    
    do_gettimeofday(&time);
    local_time = (u32)(time.tv_sec - (sys_tz.tz_minuteswest * 60));
    rtc_time_to_tm(local_time, &tm);
}

struct kobject *klapse_livedisplay_kobj;
EXPORT_SYMBOL_GPL(klapse_livedisplay_kobj);

static int __init klapse_init(void){

int rc;

    values_setup();        
    
    klapse_livedisplay_kobj = kobject_create_and_add("klapse_livedisplay", NULL) ;
    if (klapse_livedisplay_kobj == NULL) {
	    pr_warn("%s: klapse_livedisplay_kobj create_and_add failed\n", __func__);
    }
    
    rc = sysfs_create_file(klapse_livedisplay_kobj, &dev_attr_enable_klapse_livedisplay.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for enable_klapse_livedisplay\n", __func__);
	}
	rc = sysfs_create_file(klapse_livedisplay_kobj, &dev_attr_daytime_r.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for daytime_r\n", __func__);
	}
	rc = sysfs_create_file(klapse_livedisplay_kobj, &dev_attr_daytime_g.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for daytime_g\n", __func__);
	}
	rc = sysfs_create_file(klapse_livedisplay_kobj, &dev_attr_daytime_b.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for daytime_b\n", __func__);
	}
	rc = sysfs_create_file(klapse_livedisplay_kobj, &dev_attr_target_r.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for target_r\n", __func__);
	}
	rc = sysfs_create_file(klapse_livedisplay_kobj, &dev_attr_target_g.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for target_g\n", __func__);
	}
	rc = sysfs_create_file(klapse_livedisplay_kobj, &dev_attr_target_b.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for target_b\n", __func__);
	}
	rc = sysfs_create_file(klapse_livedisplay_kobj, &dev_attr_klapse_start_hour.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for klapse_start_hour\n", __func__);
	}
	rc = sysfs_create_file(klapse_livedisplay_kobj, &dev_attr_klapse_stop_hour.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for klapse_stop_hour\n", __func__);
	}
	rc = sysfs_create_file(klapse_livedisplay_kobj, &dev_attr_klapse_scaling_rate.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for klapse_scaling_rate\n", __func__);
	}
	rc = sysfs_create_file(klapse_livedisplay_kobj, &dev_attr_brightness_multiplier.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for brightness_multiplier\n", __func__);
	}
	rc = sysfs_create_file(klapse_livedisplay_kobj, &dev_attr_brightness_multiplier_auto.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for brightness_multiplier_auto\n", __func__);
	}  
	/* REQUIRES BRIGHTNESS INTEGRATION
	rc = sysfs_create_file(klapse_livedisplay_kobj, &dev_attr_klapse_brightness_threshold.attr);
	if (rc) {
		pr_warn("%s: sysfs_create_file failed for klapse_brightness_threshold\n", __func__);
	}
	*/

return 0;
}

static void __exit klapse_exit(void){
    kobject_del(klapse_livedisplay_kobj);
}

module_init(klapse_init);
module_exit(klapse_exit);
