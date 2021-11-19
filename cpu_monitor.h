#ifndef _CPU_MONITOR_H_
#define _CPU_MONITOR_H_

#define PATH_MAX	4096	/* # chars in a path name including nul */
#define CPU_PATH	"/sys/devices/system/cpu"
#define THERMAL_PATH	"/sys/devices/virtual/thermal"
#define STAT_PATH	"/proc/stat"
#define ADJ_SIZE(l,r,s) (l-strlen(r)-strlen(#s))
#define LINE_BUF_SIZE	1024

/* Parameters used to convert the timespec values: */
#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define FSEC_PER_SEC	1000000000000000LL


typedef struct jiffy_counts_t {
	unsigned long long usr, nic, sys, idle;
	unsigned long long iowait, irq, softirq, steal;
	unsigned long long total;
	unsigned long long busy;
}Jiffy_count_t;

typedef struct systeminfo {
	int		first_run_flag;
	unsigned int	*cpufreq;		//cpu current freq info
	unsigned int	nr_cpus;		//total cpus num
	unsigned int	cpu_temp;
	unsigned int	gpu_temp;
	Jiffy_count_t *cur_jiffy, *prev_jiffy;
	char	**cpu_rate;
}Systeminfo_t;

#endif
