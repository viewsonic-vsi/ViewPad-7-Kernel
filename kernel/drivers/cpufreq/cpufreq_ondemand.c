/*
 *  drivers/cpufreq/cpufreq_ondemand.c
 *
 *  Copyright (C)  2001 Russell King
 *            (C)  2003 Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>.
 *                      Jun Nakajima <jun.nakajima@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/input.h>
#include <linux/workqueue.h>

/* FIH, SimonSSChang, 2009/12/15 { */
/* [F0X_CR], add cpu loading event log */
#ifdef CONFIG_FIH_FXX
#include <linux/eventlog.h>
#include <linux/rtc.h>
#include <linux/mm.h>  //Added for test
#include <linux/time.h>
#include <asm/cputime.h>
#include <linux/kernel_stat.h>

#define MAX_PROCESS_NUM (3000)
#define LOADING_CHECK_TIME_SEC (2)
#endif
/* } FIH, SimonSSChang, 2009/12/15 */

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */

#define DEF_FREQUENCY_DOWN_DIFFERENTIAL		(10)
#define DEF_FREQUENCY_UP_THRESHOLD		(80)
#define MICRO_FREQUENCY_DOWN_DIFFERENTIAL	(3)
#define MICRO_FREQUENCY_UP_THRESHOLD		(95)
#define MICRO_FREQUENCY_MIN_SAMPLE_RATE		(10000)
#define MIN_FREQUENCY_UP_THRESHOLD		(11)
#define MAX_FREQUENCY_UP_THRESHOLD		(100)

/*
 * The polling frequency of this governor depends on the capability of
 * the processor. Default polling frequency is 1000 times the transition
 * latency of the processor. The governor will work on any processor with
 * transition latency <= 10mS, using appropriate sampling
 * rate.
 * For CPUs with transition latency > 10mS (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work.
 * All times here are in uS.
 */
#define MIN_SAMPLING_RATE_RATIO			(2)

static unsigned int min_sampling_rate;

#define LATENCY_MULTIPLIER			(1000)
#define MIN_LATENCY_MULTIPLIER			(100)
#define TRANSITION_LATENCY_LIMIT		(10 * 1000 * 1000)

static void do_dbs_timer(struct work_struct *work);
static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND
static
#endif
struct cpufreq_governor cpufreq_gov_ondemand = {
       .name                   = "ondemand",
       .governor               = cpufreq_governor_dbs,
       .max_transition_latency = TRANSITION_LATENCY_LIMIT,
       .owner                  = THIS_MODULE,
};

/* Sampling types */
enum {DBS_NORMAL_SAMPLE, DBS_SUB_SAMPLE};

/* FIH, SimonSSChang, 2009/12/15 { */
/* [F0X_CR], add cpu loading event log */
#ifdef CONFIG_FIH_FXX
enum lttng_thread_type {
	LTTNG_USER_THREAD = 0,
	LTTNG_KERNEL_THREAD = 1,
};

enum lttng_execution_mode {
	LTTNG_USER_MODE = 0,
	LTTNG_SYSCALL = 1,
	LTTNG_TRAP = 2,
	LTTNG_IRQ = 3,
	LTTNG_SOFTIRQ = 4,
	LTTNG_MODE_UNKNOWN = 5,
};

enum lttng_execution_submode {
	LTTNG_NONE = 0,
	LTTNG_UNKNOWN = 1,
};

enum lttng_process_status {
	LTTNG_UNNAMED = 0,
	LTTNG_WAIT_FORK = 1,
	LTTNG_WAIT_CPU = 2,
	LTTNG_EXIT = 3,
	LTTNG_ZOMBIE = 4,
	LTTNG_WAIT = 5,
	LTTNG_RUN = 6,
	LTTNG_DEAD = 7,
};

enum process_log_type {
    FIRST_LOG = 0,
    SECOND_LOG = 1,
};
struct cpu_stats {
	cputime64_t user;
	cputime64_t nice;
	cputime64_t system;
	cputime64_t idle;
	cputime64_t iowait;
	cputime64_t irq;
	cputime64_t softirq;
	cputime64_t steal;
};

struct process_stats {
    pid_t pid;
    cputime_t utime;
    cputime_t stime;
};
#endif
/* } FIH, SimonSSChang, 2009/12/15 */

struct cpu_dbs_info_s {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
	struct cpufreq_frequency_table *freq_table;
	unsigned int freq_lo;
	unsigned int freq_lo_jiffies;
	unsigned int freq_hi_jiffies;
	int cpu;
	unsigned int sample_type:1;
	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
};
static DEFINE_PER_CPU(struct cpu_dbs_info_s, od_cpu_dbs_info);

static unsigned int dbs_enable;	/* number of CPUs using this policy */

/*
 * dbs_mutex protects data in dbs_tuners_ins from concurrent changes on
 * different CPUs. It protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);

static struct workqueue_struct	*kondemand_wq;

static struct dbs_tuners {
	unsigned int sampling_rate;
	unsigned int up_threshold;
	unsigned int down_differential;
	unsigned int ignore_nice;
	unsigned int powersave_bias;
} dbs_tuners_ins = {
	.up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
	.down_differential = DEF_FREQUENCY_DOWN_DIFFERENTIAL,
	.ignore_nice = 0,
	.powersave_bias = 0,
};

static inline cputime64_t get_cpu_idle_time_jiffy(unsigned int cpu,
							cputime64_t *wall)
{
	cputime64_t idle_time;
	cputime64_t cur_wall_time;
	cputime64_t busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());
	busy_time = cputime64_add(kstat_cpu(cpu).cpustat.user,
			kstat_cpu(cpu).cpustat.system);

	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.irq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.softirq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.steal);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.nice);

	idle_time = cputime64_sub(cur_wall_time, busy_time);
	if (wall)
		*wall = (cputime64_t)jiffies_to_usecs(cur_wall_time);

	return (cputime64_t)jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, wall);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);

	return idle_time;
}

/*
 * Find right freq to be set now with powersave_bias on.
 * Returns the freq_hi to be used right now and will set freq_hi_jiffies,
 * freq_lo, and freq_lo_jiffies in percpu area for averaging freqs.
 */
static unsigned int powersave_bias_target(struct cpufreq_policy *policy,
					  unsigned int freq_next,
					  unsigned int relation)
{
	unsigned int freq_req, freq_reduc, freq_avg;
	unsigned int freq_hi, freq_lo;
	unsigned int index = 0;
	unsigned int jiffies_total, jiffies_hi, jiffies_lo;
	struct cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info,
						   policy->cpu);

	if (!dbs_info->freq_table) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_jiffies = 0;
		return freq_next;
	}

	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_next,
			relation, &index);
	freq_req = dbs_info->freq_table[index].frequency;
	freq_reduc = freq_req * dbs_tuners_ins.powersave_bias / 1000;
	freq_avg = freq_req - freq_reduc;

	/* Find freq bounds for freq_avg in freq_table */
	index = 0;
	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
			CPUFREQ_RELATION_H, &index);
	freq_lo = dbs_info->freq_table[index].frequency;
	index = 0;
	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
			CPUFREQ_RELATION_L, &index);
	freq_hi = dbs_info->freq_table[index].frequency;

	/* Find out how long we have to be in hi and lo freqs */
	if (freq_hi == freq_lo) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_jiffies = 0;
		return freq_lo;
	}
	jiffies_total = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);
	jiffies_hi = (freq_avg - freq_lo) * jiffies_total;
	jiffies_hi += ((freq_hi - freq_lo) / 2);
	jiffies_hi /= (freq_hi - freq_lo);
	jiffies_lo = jiffies_total - jiffies_hi;
	dbs_info->freq_lo = freq_lo;
	dbs_info->freq_lo_jiffies = jiffies_lo;
	dbs_info->freq_hi_jiffies = jiffies_hi;
	return freq_hi;
}

static void ondemand_powersave_bias_init_cpu(int cpu)
{
	struct cpu_dbs_info_s *dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
	dbs_info->freq_table = cpufreq_frequency_get_table(cpu);
	dbs_info->freq_lo = 0;
}

static void ondemand_powersave_bias_init(void)
{
	int i;
	for_each_online_cpu(i) {
		ondemand_powersave_bias_init_cpu(i);
	}
}

/************************** sysfs interface ************************/

static ssize_t show_sampling_rate_max(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	printk_once(KERN_INFO "CPUFREQ: ondemand sampling_rate_max "
	       "sysfs file is deprecated - used by: %s\n", current->comm);
	return sprintf(buf, "%u\n", -1U);
}

static ssize_t show_sampling_rate_min(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", min_sampling_rate);
}

#define define_one_ro(_name)		\
static struct global_attr _name =	\
__ATTR(_name, 0444, show_##_name, NULL)

define_one_ro(sampling_rate_max);
define_one_ro(sampling_rate_min);

/* cpufreq_ondemand Governor Tunables */
#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)              \
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}
show_one(sampling_rate, sampling_rate);
show_one(up_threshold, up_threshold);
show_one(ignore_nice_load, ignore_nice);
show_one(powersave_bias, powersave_bias);

/*** delete after deprecation time ***/

#define DEPRECATION_MSG(file_name)					\
	printk_once(KERN_INFO "CPUFREQ: Per core ondemand sysfs "	\
		    "interface is deprecated - " #file_name "\n");

#define show_one_old(file_name)						\
static ssize_t show_##file_name##_old					\
(struct cpufreq_policy *unused, char *buf)				\
{									\
	printk_once(KERN_INFO "CPUFREQ: Per core ondemand sysfs "	\
		    "interface is deprecated - " #file_name "\n");	\
	return show_##file_name(NULL, NULL, buf);			\
}
show_one_old(sampling_rate);
show_one_old(up_threshold);
show_one_old(ignore_nice_load);
show_one_old(powersave_bias);
show_one_old(sampling_rate_min);
show_one_old(sampling_rate_max);

#define define_one_ro_old(object, _name)       \
static struct freq_attr object =               \
__ATTR(_name, 0444, show_##_name##_old, NULL)

define_one_ro_old(sampling_rate_min_old, sampling_rate_min);
define_one_ro_old(sampling_rate_max_old, sampling_rate_max);

/*** delete after deprecation time ***/

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	mutex_lock(&dbs_mutex);
	dbs_tuners_ins.sampling_rate = max(input, min_sampling_rate);
	mutex_unlock(&dbs_mutex);

	return count;
}

static ssize_t store_up_threshold(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_FREQUENCY_UP_THRESHOLD ||
			input < MIN_FREQUENCY_UP_THRESHOLD) {
		return -EINVAL;
	}

	mutex_lock(&dbs_mutex);
	dbs_tuners_ins.up_threshold = input;
	mutex_unlock(&dbs_mutex);

	return count;
}

static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	mutex_lock(&dbs_mutex);
	if (input == dbs_tuners_ins.ignore_nice) { /* nothing to do */
		mutex_unlock(&dbs_mutex);
		return count;
	}
	dbs_tuners_ins.ignore_nice = input;

	/* we need to re-evaluate prev_cpu_idle */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(od_cpu_dbs_info, j);
		dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&dbs_info->prev_cpu_wall);
		if (dbs_tuners_ins.ignore_nice)
			dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;

	}
	mutex_unlock(&dbs_mutex);

	return count;
}

static ssize_t store_powersave_bias(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	if (input > 1000)
		input = 1000;

	mutex_lock(&dbs_mutex);
	dbs_tuners_ins.powersave_bias = input;
	ondemand_powersave_bias_init();
	mutex_unlock(&dbs_mutex);

	return count;
}

#define define_one_rw(_name) \
static struct global_attr _name = \
__ATTR(_name, 0644, show_##_name, store_##_name)

define_one_rw(sampling_rate);
define_one_rw(up_threshold);
define_one_rw(ignore_nice_load);
define_one_rw(powersave_bias);

static struct attribute *dbs_attributes[] = {
	&sampling_rate_max.attr,
	&sampling_rate_min.attr,
	&sampling_rate.attr,
	&up_threshold.attr,
	&ignore_nice_load.attr,
	&powersave_bias.attr,
	NULL
};

static struct attribute_group dbs_attr_group = {
	.attrs = dbs_attributes,
	.name = "ondemand",
};

/*** delete after deprecation time ***/

#define write_one_old(file_name)					\
static ssize_t store_##file_name##_old					\
(struct cpufreq_policy *unused, const char *buf, size_t count)		\
{									\
       printk_once(KERN_INFO "CPUFREQ: Per core ondemand sysfs "	\
		   "interface is deprecated - " #file_name "\n");	\
       return store_##file_name(NULL, NULL, buf, count);		\
}
write_one_old(sampling_rate);
write_one_old(up_threshold);
write_one_old(ignore_nice_load);
write_one_old(powersave_bias);

#define define_one_rw_old(object, _name)       \
static struct freq_attr object =               \
__ATTR(_name, 0644, show_##_name##_old, store_##_name##_old)

define_one_rw_old(sampling_rate_old, sampling_rate);
define_one_rw_old(up_threshold_old, up_threshold);
define_one_rw_old(ignore_nice_load_old, ignore_nice_load);
define_one_rw_old(powersave_bias_old, powersave_bias);

static struct attribute *dbs_attributes_old[] = {
       &sampling_rate_max_old.attr,
       &sampling_rate_min_old.attr,
       &sampling_rate_old.attr,
       &up_threshold_old.attr,
       &ignore_nice_load_old.attr,
       &powersave_bias_old.attr,
       NULL
};

static struct attribute_group dbs_attr_group_old = {
       .attrs = dbs_attributes_old,
       .name = "ondemand",
};

/*** delete after deprecation time ***/

/************************** sysfs end ************************/

/* FIH, SimonSSChang, 2009/12/15 { */
/* [F0X_CR], add cpu loading event log */
#ifdef CONFIG_FIH_FXX
static int proc_pid_cmdline(struct task_struct *task, char * buffer)
{
	int res = 0;
	unsigned int len;
	struct mm_struct *mm = get_task_mm(task);
	if (!mm)
		goto out;
	if (!mm->arg_end)
		goto out_mm;	/* Shh! No looking before we're done */

 	len = mm->arg_end - mm->arg_start;
 
	if (len > PAGE_SIZE)
		len = PAGE_SIZE;
 
	res = access_process_vm(task, mm->arg_start, buffer, len, 0);

	// If the nul at the end of args has been overwritten, then
	// assume application is using setproctitle(3).
	if (res > 0 && buffer[res-1] != '\0' && len < PAGE_SIZE) {
		len = strnlen(buffer, res);
		if (len < res) {
		    res = len;
		} else {
			len = mm->env_end - mm->env_start;
			if (len > PAGE_SIZE - res)
				len = PAGE_SIZE - res;
			res += access_process_vm(task, mm->env_start, buffer+res, len, 0);
			res = strnlen(buffer, res);
		}
	}
out_mm:
	mmput(mm);
out:
	return res;
}

void ondemand_cpu_time_log(enum process_log_type log_type, 
    struct cpu_stats *old_cpu_stat)
{
	int i;	
	long unsigned total_time;
    cputime64_t user, nice, system, idle, iowait, irq, softirq, steal;	
    user = nice = system = idle = iowait = irq = softirq = steal = cputime64_zero;	

    if(log_type == FIRST_LOG) {
    	for_each_online_cpu(i) {
            user = kstat_cpu(i).cpustat.user;
    	    nice = kstat_cpu(i).cpustat.nice;
    		system = kstat_cpu(i).cpustat.system;
    		idle = kstat_cpu(i).cpustat.idle;
    		iowait = kstat_cpu(i).cpustat.iowait;
	    	irq = kstat_cpu(i).cpustat.irq;
    		softirq = kstat_cpu(i).cpustat.softirq;
    		steal = kstat_cpu(i).cpustat.steal;

            //memorize first time cpu stats
            old_cpu_stat->user = user;
            old_cpu_stat->nice = nice;
            old_cpu_stat->system = system;
            old_cpu_stat->idle = idle;
            old_cpu_stat->iowait = iowait;
            old_cpu_stat->irq = irq;
            old_cpu_stat->softirq = softirq;
            old_cpu_stat->steal = steal;
        }
    } else {
    	for_each_online_cpu(i) {
            user = kstat_cpu(i).cpustat.user;
    	    nice = kstat_cpu(i).cpustat.nice;
    		system = kstat_cpu(i).cpustat.system;
    		idle = kstat_cpu(i).cpustat.idle;
    		iowait = kstat_cpu(i).cpustat.iowait;
	    	irq = kstat_cpu(i).cpustat.irq;
    		softirq = kstat_cpu(i).cpustat.softirq;
    		steal = kstat_cpu(i).cpustat.steal;

	    	eventlog("C  %3llu %3llu %3llu %3llu %3llu %3llu %3llu\n",
    		(unsigned long long)cputime64_to_clock_t(user - old_cpu_stat->user),
    		(unsigned long long)cputime64_to_clock_t(nice - old_cpu_stat->nice),
    		(unsigned long long)cputime64_to_clock_t(system - old_cpu_stat->system),
	    	(unsigned long long)cputime64_to_clock_t(idle - old_cpu_stat->idle),
    		(unsigned long long)cputime64_to_clock_t(iowait - old_cpu_stat->iowait),
	    	(unsigned long long)cputime64_to_clock_t(irq - old_cpu_stat->irq),
    		(unsigned long long)cputime64_to_clock_t(softirq - old_cpu_stat->softirq)
        		);
        }    
        
    	total_time = cputime64_to_clock_t(user - old_cpu_stat->user) + 
            cputime64_to_clock_t(nice - old_cpu_stat->nice) +
    	    cputime64_to_clock_t(system - old_cpu_stat->system) +
    	    cputime64_to_clock_t(idle - old_cpu_stat->idle) +
        	cputime64_to_clock_t(iowait - old_cpu_stat->iowait) + 
        	cputime64_to_clock_t(irq - old_cpu_stat->irq) +
        	cputime64_to_clock_t(softirq - old_cpu_stat->softirq);

        //clear first time cpu stats
        old_cpu_stat->user = cputime64_zero;
        old_cpu_stat->nice = cputime64_zero;        
        old_cpu_stat->system = cputime64_zero;
        old_cpu_stat->idle = cputime64_zero;        
        old_cpu_stat->iowait = cputime64_zero;
        old_cpu_stat->irq = cputime64_zero;        
        old_cpu_stat->softirq = cputime64_zero;
        old_cpu_stat->steal = cputime64_zero;
    }
}

void ondemad_process_log(enum process_log_type log_type, 
                struct cpu_stats *old_cpu_stat, struct process_stats *old_process_stats)
{
	struct task_struct *t = &init_task;
	struct task_struct *p = t;
	enum lttng_process_status status;
	enum lttng_thread_type type;
	enum lttng_execution_mode mode;
	enum lttng_execution_submode submode;
	char cmdline[256];
	char proc_name[256];

    cputime_t total_utime;
    cputime_t total_stime;

    //check cpu time
    ondemand_cpu_time_log(log_type, old_cpu_stat);

    do {
        mode = LTTNG_MODE_UNKNOWN;
        submode = LTTNG_UNKNOWN;
                
        read_lock(&tasklist_lock);
        if (t != &init_task) {
            atomic_dec(&t->usage);
            t = next_thread(t);
        }
        if (t == p) {
            p = next_task(t);
            t = p;
        }
        atomic_inc(&t->usage);
        read_unlock(&tasklist_lock);
                
        /* FIH, Charles Huang, 2009/10/08 { */
        /* [FXX_CR], performance test */
        #ifdef CONFIG_FIH_FXX
        proc_pid_cmdline(t,proc_name);
        sprintf(cmdline,"%s(%s)",proc_name,t->comm);
        #endif		
        /* } FIH, Charles Huang, 2009/10/08 */
                
        task_lock(t);
                
        if (t->exit_state == EXIT_ZOMBIE)
            status = LTTNG_ZOMBIE;
        else if (t->exit_state == EXIT_DEAD)
            status = LTTNG_DEAD;
        else if (t->state == TASK_RUNNING) {
        /* Is this a forked child that has not run yet? */
        if (list_empty(&t->rt.run_list))
            status = LTTNG_WAIT_FORK;
        else
            /*
            * All tasks are considered as wait_cpu;
            * the viewer will sort out if the task was
            * really running at this time.
            */
            status = LTTNG_WAIT_CPU;
        } else if (t->state &
                (TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)) {
            /* Task is waiting for something to complete */
            status = LTTNG_WAIT;
        } else
            status = LTTNG_UNNAMED;
        submode = LTTNG_NONE;
                
            /*
            * Verification of t->mm is to filter out kernel threads;
            * Viewer will further filter out if a user-space thread was
            * in syscall mode or not.
            */
            if (t->mm)
                type = LTTNG_USER_THREAD;
            else
                type = LTTNG_KERNEL_THREAD;
            /* FIH, Charles Huang, 2009/10/08 { */
            /* [FXX_CR], performance test */
#ifdef CONFIG_FIH_FXX
            if(log_type == FIRST_LOG) {
                if(t->pid < MAX_PROCESS_NUM) {
                    //memorize first time process stats
                    old_process_stats[t->pid].pid = t->pid;
                    old_process_stats[t->pid].utime= t->utime;
                    old_process_stats[t->pid].stime= t->stime;
                }
            } else {
                if(t->pid < MAX_PROCESS_NUM) {
                    if(old_process_stats[t->pid].pid != 0) {
                        total_utime = t->utime - old_process_stats[t->pid].utime;
                        total_stime = t->stime - old_process_stats[t->pid].stime;
                    
                        if((total_utime + total_stime) != 0) {
                            eventlog("I  %3u %3u %s\n", 
                                total_utime, total_stime, cmdline);                
                        }  
                    }           
                }
            }
#else
            __trace_mark(generic, name, call_private, format, args...)(0, task_state, process_state, call_data,
                    "pid %d parent_pid %d name %s type %d mode %d "
                    "submode %d status %d tgid %d",
                    t->pid, t->parent->pid, t->comm,
                    type, mode, submode, status, t->tgid);
#endif
            /* } FIH, Charles Huang, 2009/10/08 */
            task_unlock(t);
    } while (t != &init_task);

    //clear first process stats
    if(log_type == SECOND_LOG) {
        memset(old_process_stats, 0, sizeof(old_process_stats));        
    }
}

void inline ondemand_init_data(bool *print_pid_flag, int64_t *total_run_time, 
    struct cpu_stats *old_cpu_stat, struct process_stats *old_process_stats)
{
    *print_pid_flag = 0;
    *total_run_time = 0;

    //clear first time cpu stats
    old_cpu_stat->user = cputime64_zero;
    old_cpu_stat->nice = cputime64_zero;        
    old_cpu_stat->system = cputime64_zero;
    old_cpu_stat->idle = cputime64_zero;        
    old_cpu_stat->iowait = cputime64_zero;
    old_cpu_stat->irq = cputime64_zero;        
    old_cpu_stat->softirq = cputime64_zero;
    old_cpu_stat->steal = cputime64_zero;    

    //clear first process stats
    memset(old_process_stats, 0, sizeof(old_process_stats));
}
#endif
/* } FIH, SimonSSChang, 2009/12/15 */

static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
    /* FIH, SimonSSChang, 2009/12/15 { */
/* [F0X_CR], add cpu loading event log */
#ifdef CONFIG_FIH_FXX
    static struct process_stats old_process_stats[MAX_PROCESS_NUM];    
    static struct cpu_stats old_cpu_stat = {cputime64_zero, cputime64_zero, cputime64_zero, 
        cputime64_zero, cputime64_zero, cputime64_zero, cputime64_zero, cputime64_zero};
    static int64_t total_run_time=0;
    static bool print_pid_flag=false;
    static bool init_flag=true;
    
    int64_t s;
    uint32_t ns;
    int64_t t1;
    static int64_t t2;
#endif
/* } FIH, SimonSSChang, 2009/12/15 */

	unsigned int max_load_freq;

	struct cpufreq_policy *policy;
	unsigned int j;

	this_dbs_info->freq_lo = 0;
	policy = this_dbs_info->cur_policy;

	/*
	 * Every sampling_rate, we check, if current idle time is less
	 * than 20% (default), then we try to increase frequency
	 * Every sampling_rate, we look for a the lowest
	 * frequency which can sustain the load while keeping idle time over
	 * 30%. If such a frequency exist, we try to decrease to this frequency.
	 *
	 * Any frequency increase takes it to the maximum frequency.
	 * Frequency reduction happens at minimum steps of
	 * 5% (default) of current frequency
	 */

	/* Get Absolute Load - in terms of freq */
	max_load_freq = 0;

	for_each_cpu(j, policy->cpus) {
		struct cpu_dbs_info_s *j_dbs_info;
		cputime64_t cur_wall_time, cur_idle_time;
		unsigned int idle_time, wall_time;
		unsigned int load, load_freq;
		int freq_avg;

		j_dbs_info = &per_cpu(od_cpu_dbs_info, j);

		cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);

		wall_time = (unsigned int) cputime64_sub(cur_wall_time,
				j_dbs_info->prev_cpu_wall);
		j_dbs_info->prev_cpu_wall = cur_wall_time;

		idle_time = (unsigned int) cputime64_sub(cur_idle_time,
				j_dbs_info->prev_cpu_idle);
		j_dbs_info->prev_cpu_idle = cur_idle_time;

		if (dbs_tuners_ins.ignore_nice) {
			cputime64_t cur_nice;
			unsigned long cur_nice_jiffies;

			cur_nice = cputime64_sub(kstat_cpu(j).cpustat.nice,
					 j_dbs_info->prev_cpu_nice);
			/*
			 * Assumption: nice time between sampling periods will
			 * be less than 2^32 jiffies for 32 bit sys
			 */
			cur_nice_jiffies = (unsigned long)
					cputime64_to_jiffies64(cur_nice);

			j_dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;
			idle_time += jiffies_to_usecs(cur_nice_jiffies);
		}

		if (unlikely(!wall_time || wall_time < idle_time))
			continue;

		load = 100 * (wall_time - idle_time) / wall_time;

		freq_avg = __cpufreq_driver_getavg(policy, j);
		if (freq_avg <= 0)
			freq_avg = policy->cur;

		load_freq = load * freq_avg;
		if (load_freq > max_load_freq)
			max_load_freq = load_freq;

        /* FIH, SimonSSChang, 2009/12/15 { */
        /* [F0X_CR], add cpu loading event log */
        #ifdef CONFIG_FIH_FXX
        if(cpu_loading_debug_get() == true) {
            if(init_flag == false) {
                //re-init data
                ondemand_init_data(&print_pid_flag, &total_run_time, 
                    &old_cpu_stat, old_process_stats);

                //re-sync time
                t2 = ktime_to_ns(ktime_get());
                init_flag = true;                
            } else {            
                t1 = ktime_to_ns(ktime_get());
                s = t1 - t2;
                total_run_time += s;
                //ns = do_div(s, NSEC_PER_SEC);
                //eventlog("L  %3d C%6d  R %lld.%09u\n", load, freq_avg, s, ns);
                t2 = ktime_to_ns(ktime_get());
            }

            if(print_pid_flag == true) {   
                ondemad_process_log(SECOND_LOG, &old_cpu_stat, old_process_stats);
                print_pid_flag = false;
            }
        
            s = total_run_time;
            ns = do_div(s, NSEC_PER_SEC);
            if(s >= cpu_loading_check_time_get()) {
                eventlog("T  %3lld.%09u\n", s, ns);
                eventlog("L  %3d %6d\n", load, freq_avg);
                if(freq_avg != 122880) {
                    if((load*freq_avg/100) >= cpu_loading_threshold_get()) {
                        ondemad_process_log(FIRST_LOG, &old_cpu_stat, old_process_stats);
                        print_pid_flag = true;
                    }                    
                }                
                total_run_time = 0;        
            }            
        } else {
            //re-init data
            init_flag = false;
        }
        #endif
        /* } FIH, SimonSSChang, 2009/12/15 */        
	}

	/* Check for frequency increase */
	if (max_load_freq > dbs_tuners_ins.up_threshold * policy->cur) {
		/* if we are already at full speed then break out early */
		if (!dbs_tuners_ins.powersave_bias) {
			if (policy->cur == policy->max)
				return;

			__cpufreq_driver_target(policy, policy->max,
				CPUFREQ_RELATION_H);
		} else {
			int freq = powersave_bias_target(policy, policy->max,
					CPUFREQ_RELATION_H);
			__cpufreq_driver_target(policy, freq,
				CPUFREQ_RELATION_L);
		}
		return;
	}

	/* Check for frequency decrease */
	/* if we cannot reduce the frequency anymore, break out early */
	if (policy->cur == policy->min)
		return;

	/*
	 * The optimal frequency is the frequency that is the lowest that
	 * can support the current CPU usage without triggering the up
	 * policy. To be safe, we focus 10 points under the threshold.
	 */
	if (max_load_freq <
	    (dbs_tuners_ins.up_threshold - dbs_tuners_ins.down_differential) *
	     policy->cur) {
		unsigned int freq_next;
		freq_next = max_load_freq /
				(dbs_tuners_ins.up_threshold -
				 dbs_tuners_ins.down_differential);

		if (!dbs_tuners_ins.powersave_bias) {
			__cpufreq_driver_target(policy, freq_next,
					CPUFREQ_RELATION_L);
		} else {
			int freq = powersave_bias_target(policy, freq_next,
					CPUFREQ_RELATION_L);
			__cpufreq_driver_target(policy, freq,
				CPUFREQ_RELATION_L);
		}
	}
}

static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;
	int sample_type = dbs_info->sample_type;

	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);

	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

	mutex_lock(&dbs_info->timer_mutex);

	/* Common NORMAL_SAMPLE setup */
	dbs_info->sample_type = DBS_NORMAL_SAMPLE;
	if (!dbs_tuners_ins.powersave_bias ||
	    sample_type == DBS_NORMAL_SAMPLE) {
		dbs_check_cpu(dbs_info);
		if (dbs_info->freq_lo) {
			/* Setup timer for SUB_SAMPLE */
			dbs_info->sample_type = DBS_SUB_SAMPLE;
			delay = dbs_info->freq_hi_jiffies;
		}
	} else {
		__cpufreq_driver_target(dbs_info->cur_policy,
			dbs_info->freq_lo, CPUFREQ_RELATION_H);
	}
	queue_delayed_work_on(cpu, kondemand_wq, &dbs_info->work, delay);
	mutex_unlock(&dbs_info->timer_mutex);
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);
	delay -= jiffies % delay;

	dbs_info->sample_type = DBS_NORMAL_SAMPLE;
	INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
	queue_delayed_work_on(dbs_info->cpu, kondemand_wq, &dbs_info->work,
		delay);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
	cancel_delayed_work_sync(&dbs_info->work);
}

static void dbs_refresh_callback(struct work_struct *unused)
{
	struct cpufreq_policy *policy;
	struct cpu_dbs_info_s *this_dbs_info;

	if (lock_policy_rwsem_write(0) < 0)
		return;

	this_dbs_info = &per_cpu(od_cpu_dbs_info, 0);
	policy = this_dbs_info->cur_policy;

	if (policy->cur < policy->max) {
		policy->cur = policy->max;

		__cpufreq_driver_target(policy, policy->max,
					CPUFREQ_RELATION_L);
		this_dbs_info->prev_cpu_idle = get_cpu_idle_time(0,
				&this_dbs_info->prev_cpu_wall);
	}
	unlock_policy_rwsem_write(0);
}

static DECLARE_WORK(dbs_refresh_work, dbs_refresh_callback);

static void dbs_input_event(struct input_handle *handle, unsigned int type,
		unsigned int code, int value)
{
	schedule_work(&dbs_refresh_work);
}

static int dbs_input_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "cpufreq";

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void dbs_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id dbs_ids[] = {
	{ .driver_info = 1 },
	{ },
};

static struct input_handler dbs_input_handler = {
	.event		= dbs_input_event,
	.connect	= dbs_input_connect,
	.disconnect	= dbs_input_disconnect,
	.name		= "cpufreq_ond",
	.id_table	= dbs_ids,
};

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int j;
	int rc;

	this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
        /* FIH, SimonSSChang, 2009/12/15 { */
        /* [F0X_CR], add cpu loading event log */
        #ifdef CONFIG_FIH_FXX
        if(cpu_loading_debug_get() == true) {
            eventlog("G  ondemand\n");
        }
        #endif
        /* } FIH, SimonSSChang, 2009/12/15 */        
      
		if ((!cpu_online(cpu)) || (!policy->cur))
			return -EINVAL;

		mutex_lock(&dbs_mutex);

		rc = sysfs_create_group(&policy->kobj, &dbs_attr_group_old);
		if (rc) {
			mutex_unlock(&dbs_mutex);
			return rc;
		}

		dbs_enable++;
		for_each_cpu(j, policy->cpus) {
			struct cpu_dbs_info_s *j_dbs_info;
			j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
			j_dbs_info->cur_policy = policy;

			j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&j_dbs_info->prev_cpu_wall);
			if (dbs_tuners_ins.ignore_nice) {
				j_dbs_info->prev_cpu_nice =
						kstat_cpu(j).cpustat.nice;
			}
		}
		this_dbs_info->cpu = cpu;
		ondemand_powersave_bias_init_cpu(cpu);
		/*
		 * Start the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 1) {
			unsigned int latency;

			rc = sysfs_create_group(cpufreq_global_kobject,
						&dbs_attr_group);
			if (rc) {
				mutex_unlock(&dbs_mutex);
				return rc;
			}

			/* policy latency is in nS. Convert it to uS first */
			latency = policy->cpuinfo.transition_latency / 1000;
			if (latency == 0)
				latency = 1;
			/* Bring kernel and HW constraints together */
			min_sampling_rate = max(min_sampling_rate,
					MIN_LATENCY_MULTIPLIER * latency);
			dbs_tuners_ins.sampling_rate =
				max(min_sampling_rate,
				    latency * LATENCY_MULTIPLIER);
		}
		rc = input_register_handler(&dbs_input_handler);
		mutex_unlock(&dbs_mutex);

		mutex_init(&this_dbs_info->timer_mutex);
		dbs_timer_init(this_dbs_info);
		break;

	case CPUFREQ_GOV_STOP:
		dbs_timer_exit(this_dbs_info);

		mutex_lock(&dbs_mutex);
		sysfs_remove_group(&policy->kobj, &dbs_attr_group_old);
		mutex_destroy(&this_dbs_info->timer_mutex);
		dbs_enable--;
		input_unregister_handler(&dbs_input_handler);
		mutex_unlock(&dbs_mutex);
		if (!dbs_enable)
			sysfs_remove_group(cpufreq_global_kobject,
					   &dbs_attr_group);

		break;

	case CPUFREQ_GOV_LIMITS:
		mutex_lock(&this_dbs_info->timer_mutex);
		if (policy->max < this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->max, CPUFREQ_RELATION_H);
		else if (policy->min > this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->min, CPUFREQ_RELATION_L);
		mutex_unlock(&this_dbs_info->timer_mutex);
		break;
	}
	return 0;
}

static int __init cpufreq_gov_dbs_init(void)
{
	int err;
	cputime64_t wall;
	u64 idle_time;
	int cpu = get_cpu();

	idle_time = get_cpu_idle_time_us(cpu, &wall);
	put_cpu();
	if (idle_time != -1ULL) {
		/* Idle micro accounting is supported. Use finer thresholds */
		dbs_tuners_ins.up_threshold = MICRO_FREQUENCY_UP_THRESHOLD;
		dbs_tuners_ins.down_differential =
					MICRO_FREQUENCY_DOWN_DIFFERENTIAL;
		/*
		 * In no_hz/micro accounting case we set the minimum frequency
		 * not depending on HZ, but fixed (very low). The deferred
		 * timer might skip some samples if idle/sleeping as needed.
		*/
		min_sampling_rate = MICRO_FREQUENCY_MIN_SAMPLE_RATE;
	} else {
		/* For correct statistics, we need 10 ticks for each measure */
		min_sampling_rate =
			MIN_SAMPLING_RATE_RATIO * jiffies_to_usecs(10);
	}

	kondemand_wq = create_workqueue("kondemand");
	if (!kondemand_wq) {
		printk(KERN_ERR "Creation of kondemand failed\n");
		return -EFAULT;
	}
	err = cpufreq_register_governor(&cpufreq_gov_ondemand);
	if (err)
		destroy_workqueue(kondemand_wq);

	return err;
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_ondemand);
	destroy_workqueue(kondemand_wq);
}


MODULE_AUTHOR("Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>");
MODULE_AUTHOR("Alexey Starikovskiy <alexey.y.starikovskiy@intel.com>");
MODULE_DESCRIPTION("'cpufreq_ondemand' - A dynamic cpufreq governor for "
	"Low Latency Frequency Transition capable processors");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
