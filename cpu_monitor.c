#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <getopt.h>
#include <regex.h>
#include <errno.h>

#include "cpumask.h"
#include "cpu_monitor.h"

//#define DEBUG

static struct option opts[] = {
	{ "delay", 1, NULL, 'd' },
	{ "count", 1, NULL, 'c' },
	{ "help", no_argument, NULL, 'h' },
	{ NULL, 0, NULL, 0 }
};

static int interval = 1;	//default 1 seconds
static int count = 0;		//no limit
cpumask_t cpu_online_map;	//cpu status, online or offline

static struct systeminfo systeminfo;
static void usage(void)
{
	printf("cpu_monitor 11/16/2021. (c) 2021 huafenghuang/(c).\n\n"
		"cpu_monitor [-dSECONDS] [-cCOUNT]\n"
		"cpu_monitor -h\n"
		"-d|--delay                      Set the monitoring period\n"
		"-c|--count                      Set the monitoring time\n"
		"-h|--help                       Show usage information\n"
	);
}

static void parse_command_line(int argc, char **argv)
{
	int c;
	while ((c = getopt_long(argc, argv, "d:c:h", opts, NULL)) != -1) {
		switch(c) {
			case 'd':
				if (!optarg) {
					interval = 1;	// use default value
					break;
				}
				interval = atoi(optarg);
				if (interval < 0)
					interval = 1;	// use default value
				break;
			case 'c':
				if (!optarg) {
					count = 0;	// use default value
					break;
				}
				count = atoi(optarg);
				if (count < 0)
					count = 0;	// use default value
				break;
			case 'h':
			default:
				usage();
				exit(1);;
				break;
		}
	}
}

static void get_offline_status(char *line, void *data)
{
	int *status = (int *)data;

	*status = (line && line[0] == '0') ? 1 : 0;
}

static void get_cpufreq(char *line, void *data)
{
	unsigned int *cpufreq = (unsigned int *)data;
	*cpufreq = strtoul(line, NULL, 10);
}

int process_one_line(char *path, void (*cb)(char *line, void *data), void *data)
{
	FILE *file;
	char *line = NULL;
	size_t size = 0;
	int ret = -1;

	file = fopen(path, "r");
	if (!file)
		return ret;

	if (getline(&line, &size, file) > 0) {
		cb(line, data);
		ret = 0;
	}
	free(line);
	fclose(file);
	return ret;
}

static void parse_online_cpufreq_info(char *path)
{
	int offline_status = 0;
	int cpu_num = -1;
	char new_path[PATH_MAX];
	unsigned int cpufreq = 0;
	int ret;

	/* skip offline cpus */
	snprintf(new_path, ADJ_SIZE(PATH_MAX, path, "/online"), "%s/online", path);
	ret = process_one_line(new_path, get_offline_status, &offline_status);
	if (offline_status)
		return;

	cpu_num = strtoul(&path[27], NULL, 10);


	cpu_set(cpu_num, cpu_online_map);

	/* get online cpufreq */
	snprintf(new_path, ADJ_SIZE(PATH_MAX, path, "/cpufreq/cpuinfo_cur_freq"),
			"%s/cpufreq/cpuinfo_cur_freq", path);
	ret = process_one_line(new_path, get_cpufreq, &cpufreq);
	if (ret < 0) {
		cpufreq = 0;
		printf("Need to support cpufreq driver\n");
	}
	systeminfo.cpufreq[cpu_num] = cpufreq;
#ifdef DEBUG
	printf("cpu num:%d, cpufreq:%u\n", cpu_num, cpufreq);
#endif
}

static int parse_cpu_info(void)
{
	char new_path[PATH_MAX];
	int i;

	/* Must clear all mask for cpu hotplug */
	cpus_clear(cpu_online_map);

	for (i = 0; i < systeminfo.nr_cpus; i++) {
		snprintf(new_path, PATH_MAX, "/sys/devices/system/cpu/cpu%d", i);
#ifdef DEBUG
		printf("path:%s, i:%d\n", new_path, i);
#endif
		parse_online_cpufreq_info(new_path);
	}
}

static int init_systeminfo_struct(struct systeminfo *systeminfo)
{

	/* Get total cpu nums */
	systeminfo->nr_cpus = sysconf(_SC_NPROCESSORS_CONF);
	if (systeminfo->nr_cpus < 0) {
		DIR *dir;
		struct dirent *entry;

		dir = opendir(CPU_PATH);
		if (!dir)
			return -EINVAL;
		do {
			int num;
			char pad;
			entry = readdir(dir);
			/*
			 * We only want to count real cpus, not cpufreq and
			 * cpuidle
			 */
			if (entry &&
			    sscanf(entry->d_name, "cpu%d%c", &num, &pad) == 1 &&
			    !strchr(entry->d_name, ' ')) {
				systeminfo->nr_cpus++;
			}
		} while (entry);

		if (systeminfo->nr_cpus < 0) {
			printf("get cpu nums error\n");
			return -EINVAL;
		}
	}
/*
	systeminfo->status = (unsigned int *)malloc(systeminfo->nr_cpus * sizeof(unsigned int));
	if (!systeminfo->status) {
		printf("alloc mem for systeminfo status failed\n");
		return -ENOMEM;
	}
*/
	systeminfo->cpufreq = (unsigned int *)malloc(systeminfo->nr_cpus * sizeof(unsigned int));
	if (!systeminfo->cpufreq) {
		printf("alloc mem for systeminfo cpufreq failed\n");
		return -ENOMEM;
	}

	return 0;
}

static void display_header(void)
{
	printf("System info:\n");
	printf("\t\tCPU%%\t\tcpufreq(kHz)\t\ttemp\t\tgpufreq(kHz)\t\tgputemp\n");
}

static void display_system_info(void)
{
	static const char fmt[] = "cpu%d\t\t%4u\t\t%12u\t\t%4u\t\t%12u\t\t%4u\n";
	char line_buf[LINE_BUF_SIZE];
	int ret;
	unsigned int i;

	for(i = 0; i < systeminfo.nr_cpus; i++) {
		for_each_online_cpu(i) {
			ret = sprintf(line_buf, fmt, i, 0, systeminfo.cpufreq[i],
					0, 0, 0);
			fputs(line_buf, stdout);
		}
	}
}

int main(int argc, char *argv[])
{
	int ret = 0;

	parse_command_line(argc, argv);
#ifdef DEBUG
	printf("interval:%d, count=%d\n", interval, count);
#endif
	ret = init_systeminfo_struct(&systeminfo);
	if (ret < 0) {
		printf("cpu_monitor init error\n");
		return ret;
	}
#ifdef DEBUG
	printf("nr_cpus: %d\n", systeminfo.nr_cpus);
#endif
	display_header();
	/* main loop */
	for(;;) {
		parse_cpu_info();
		display_system_info();
		printf("\n");
		if (count > 0) {
			if (--count == 0)
				break;
		}
		sleep(interval);
	}
	return 0;
}
