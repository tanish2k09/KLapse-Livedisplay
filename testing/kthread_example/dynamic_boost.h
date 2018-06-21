#ifndef __DYNAMIC_BOOST_H__
#define __DYNAMIC_BOOST_H__

#define CONFIG_MTK_DYNAMIC_BOOST
#define CONFIG_TOUCH_BOOST

enum mode {
	PRIO_TWO_LITTLES,				//0
	PRIO_TWO_LITTLES_MAX_FREQ,		//1
	PRIO_ONE_BIG,					//2
	PRIO_ONE_BIG_MAX_FREQ,			//3
	PRIO_ONE_BIG_ONE_LITTLE,		//4
	PRIO_ONE_BIG_ONE_LITTLE_MAX_FREQ,//5
	PRIO_TWO_BIGS,					//6
	PRIO_TWO_BIGS_MAX_FREQ,			//7
	PRIO_FOUR_LITTLES,				//8
	PRIO_FOUR_LITTLES_MAX_FREQ,		//9
	PRIO_TWO_BIGS_TWO_LITTLES,		//10
	PRIO_TWO_BIGS_TWO_LITTLES_MAX_FREQ,//11
	PRIO_FOUR_BIGS,					//12
	PRIO_FOUR_BIGS_MAX_FREQ,		//13
	PRIO_MAX_CORES,					//14
	PRIO_MAX_CORES_MAX_FREQ,		//15
	PRIO_RESET,						//16
	/* Define the max priority for priority limit */
	PRIO_DEFAULT					//17
};

enum control {
	OFF = -2,
	ON = -1
};

int set_dynamic_boost(int duration, int prio_mode);
extern void interactive_boost_cpu(int boost);

#endif	/* __DYNAMIC_BOOST_H__ */

