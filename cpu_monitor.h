#ifndef _CPU_MONITOR_H_
#define _CPU_MONITOR_H_

#define PATH_MAX	4096	/* # chars in a path name including nul */
#define CPU_PATH	"/sys/devices/system/cpu"
#define THERMAL_PATH	"/sys/devices/virtual/thermal"
#define ADJ_SIZE(l,r,s) (l-strlen(r)-strlen(#s))
#define LINE_BUF_SIZE	1024

typedef struct systeminfo {
	unsigned int	*cpufreq;		//cpu current freq info
	unsigned int	nr_cpus;		//total cpus num
	unsigned int	cpu_temp;
	unsigned int	gpu_temp;
}Systeminfo_t;

#endif
