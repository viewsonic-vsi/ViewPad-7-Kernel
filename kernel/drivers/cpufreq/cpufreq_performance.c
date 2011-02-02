/*
 *  linux/drivers/cpufreq/cpufreq_performance.c
 *
 *  Copyright (C) 2002 - 2003 Dominik Brodowski <linux@brodo.de>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/init.h>

/* FIH, SimonSSChang, 2009/12/15 { */
/* [F0X_CR], add cpu loading event log */
#ifdef CONFIG_FIH_FXX
#include <linux/eventlog.h>
#include <linux/rtc.h>
#include <linux/mm.h>  //Added for test
#include <linux/time.h>
#include <asm/cputime.h>
#include <linux/kernel_stat.h>
#include <linux/jiffies.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#endif
/* } FIH, SimonSSChang, 2009/12/15 */

#define dprintk(msg...) \
	cpufreq_debug_printk(CPUFREQ_DEBUG_GOVERNOR, "performance", msg)

/* FIH, SimonSSChang, 2009/12/15 { */
/* [F0X_CR], add cpu loading event log */
#ifdef CONFIG_FIH_FXX
#define MAX_PROCESS_NUM (3000)
#define SAMPLING_TIME (50000)
#define LOADING_CHECK_TIME_SEC (2)

static void do_dbs_timer(struct work_struct *work);

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
//static struct cpu_info old_cpu, new_cpu;

struct cpu_dbs_info_s {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
	struct cpufreq_policy *cur_policy;
 	struct delayed_work work;
	int cpu;
    unsigned int enable:1;
};

static DEFINE_PER_CPU(struct cpu_dbs_info_s, cpu_dbs_info);

static DEFINE_MUTEX(dbs_mutex);

static struct workqueue_struct *kperformance_wq;

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
		*wall = cur_wall_time;

	return idle_time;
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, wall);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);

	return idle_time;
}

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

void performacne_cpu_time_log(enum process_log_type log_type, 
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

void performance_process_log(enum process_log_type log_type, 
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
    performacne_cpu_time_log(log_type, old_cpu_stat);

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

void inline performance_init_data(bool *print_pid_flag, int64_t *total_run_time, 
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

static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
    static struct process_stats old_process_stats[MAX_PROCESS_NUM];
    static struct cpu_stats old_cpu_stat = {cputime64_zero, cputime64_zero, cputime64_zero, 
        cputime64_zero, cputime64_zero, cputime64_zero, cputime64_zero, cputime64_zero};
    static bool print_pid_flag=false;
    static int64_t total_run_time=0;
    static bool init_flag=true;
    
    int64_t s;
    uint32_t ns;
    int64_t t1;
    static int64_t t2;

	struct cpufreq_policy *policy;
	unsigned int j;

    if(!this_dbs_info->enable)
        return;

	policy = this_dbs_info->cur_policy;


	/* Get Absolute Load - in terms of freq */

	for_each_cpu(j, policy->cpus) {
		struct cpu_dbs_info_s *j_dbs_info;
		cputime64_t cur_wall_time, cur_idle_time;
		unsigned int idle_time, wall_time;
		unsigned int load;
		int freq_avg;

		j_dbs_info = &per_cpu(cpu_dbs_info, j);

		cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);

		wall_time = (unsigned int) cputime64_sub(cur_wall_time,
				j_dbs_info->prev_cpu_wall);
		j_dbs_info->prev_cpu_wall = cur_wall_time;

		idle_time = (unsigned int) cputime64_sub(cur_idle_time,
				j_dbs_info->prev_cpu_idle);
		j_dbs_info->prev_cpu_idle = cur_idle_time;

		if (unlikely(!wall_time || wall_time < idle_time))
			continue;

		load = 100 * (wall_time - idle_time) / wall_time;

		freq_avg = policy->cur;

        if(cpu_loading_debug_get() == true) {
            if(init_flag == false) {
                //re-init data
                performance_init_data(&print_pid_flag, &total_run_time, 
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
                performance_process_log(SECOND_LOG, &old_cpu_stat, old_process_stats);
                print_pid_flag = false;
            }

            s = total_run_time;
            ns = do_div(s, NSEC_PER_SEC);
            if(s >= cpu_loading_check_time_get()) {
                eventlog("T  %3lld.%09u\n", s, ns);
                eventlog("L  %3d %6d\n", load, freq_avg);
                if(freq_avg != 122880) {
                    if((load*freq_avg/100) >= cpu_loading_threshold_get()) {
                        performance_process_log(FIRST_LOG, &old_cpu_stat, old_process_stats);
                        print_pid_flag = 1;
                    }
                }
                total_run_time = 0;        
            }
        } else {
            //re-init data
            init_flag = false;
        }
	}
}
static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;

	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(SAMPLING_TIME);

	delay -= jiffies % delay;

	if (lock_policy_rwsem_write(cpu) < 0)
		return;

    if(!dbs_info->enable) {
        unlock_policy_rwsem_write(cpu);
        return;
    }

    dbs_check_cpu(dbs_info);
    
	queue_delayed_work_on(cpu, kperformance_wq, &dbs_info->work, delay);
	unlock_policy_rwsem_write(cpu);
    
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
    int delay = usecs_to_jiffies(SAMPLING_TIME);
    delay -= jiffies % delay;

    dbs_info->enable = 1;
    INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
    queue_delayed_work_on(dbs_info->cpu, kperformance_wq, &dbs_info->work,
                 delay);    
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
    dbs_info->enable = 0;
	cancel_delayed_work(&dbs_info->work);
}
#endif
/* } FIH, SimonSSChang, 2009/12/15 */

/* FIH, SimonSSChang, 2009/12/15 { */
/* [F0X_CR], add cpu loading event log */
#ifdef CONFIG_FIH_FXX
static int cpufreq_governor_performance(struct cpufreq_policy *policy,
					unsigned int event)
{
    unsigned int cpu = policy->cpu;
    struct cpu_dbs_info_s *this_dbs_info;
    unsigned int j;

    static int timer_init_flag = 0;
    this_dbs_info = &per_cpu(cpu_dbs_info, cpu);
    
	switch (event) {
	case CPUFREQ_GOV_START:
        if(this_dbs_info->enable) /* Already enabled */
            break;
        mutex_lock(&dbs_mutex);
        if(cpu_loading_debug_get() == true) {
        eventlog("G  performance\n");
        
        for_each_cpu(j, policy->cpus) {
			struct cpu_dbs_info_s *j_dbs_info;
			j_dbs_info = &per_cpu(cpu_dbs_info, j);
			j_dbs_info->cur_policy = policy;

			j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&j_dbs_info->prev_cpu_wall);
			/*if (dbs_tuners_ins.ignore_nice) {
				j_dbs_info->prev_cpu_nice =
						kstat_cpu(j).cpustat.nice;
			}*/
		}
		this_dbs_info->cpu = cpu;
        
        dbs_timer_init(this_dbs_info);
        
            timer_init_flag = 1;
        }

   		dprintk("setting to %u kHz because of event %u\n",
						policy->max, event);
		__cpufreq_driver_target(policy, policy->max,
						CPUFREQ_RELATION_H);

        mutex_unlock(&dbs_mutex);
        break;        
	case CPUFREQ_GOV_LIMITS:
        mutex_lock(&dbs_mutex);
		dprintk("setting to %u kHz because of event %u\n",
						policy->max, event);
		__cpufreq_driver_target(policy, policy->max,
						CPUFREQ_RELATION_H);
        mutex_unlock(&dbs_mutex);
		break;
    case CPUFREQ_GOV_STOP: 
        mutex_lock(&dbs_mutex);
        if(timer_init_flag == 1) {
        dbs_timer_exit(this_dbs_info);
            timer_init_flag = 0;
        }
        mutex_unlock(&dbs_mutex);
        break;
	default:
		break;
	}
	return 0;
}
#else
static int cpufreq_governor_performance(struct cpufreq_policy *policy,
					unsigned int event)
{
	switch (event) {
	case CPUFREQ_GOV_START:
	case CPUFREQ_GOV_LIMITS:
		dprintk("setting to %u kHz because of event %u\n",
						policy->max, event);
		__cpufreq_driver_target(policy, policy->max,
						CPUFREQ_RELATION_H);
		break;
	default:
		break;
	}
	return 0;
}
#endif
/* } FIH, SimonSSChang, 2009/12/15 */

#ifdef CONFIG_CPU_FREQ_GOV_PERFORMANCE_MODULE
static
#endif
struct cpufreq_governor cpufreq_gov_performance = {
	.name		= "performance",
	.governor	= cpufreq_governor_performance,
	.owner		= THIS_MODULE,
};


static int __init cpufreq_gov_performance_init(void)
{
/* FIH, SimonSSChang, 2009/12/15 { */
/* [F0X_CR], add cpu loading event log */
#ifdef CONFIG_FIH_FXX
    int err;
    kperformance_wq = create_workqueue("kperformance");
    if(!kperformance_wq) {
        printk(KERN_ERR "Creation of kperformance failed\n");
        return -EFAULT;
    }

    err = cpufreq_register_governor(&cpufreq_gov_performance);
    if(err)
        destroy_workqueue(kperformance_wq);
    
	return err;
#else
	return cpufreq_register_governor(&cpufreq_gov_performance);
#endif
/* } FIH, SimonSSChang, 2009/12/15 */    
}


static void __exit cpufreq_gov_performance_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_performance);

/* FIH, SimonSSChang, 2009/12/15 { */
/* [F0X_CR], add cpu loading event log */
#ifdef CONFIG_FIH_FXX
    destroy_workqueue(kperformance_wq);
#endif
/* } FIH, SimonSSChang, 2009/12/15 */    
}


MODULE_AUTHOR("Dominik Brodowski <linux@brodo.de>");
MODULE_DESCRIPTION("CPUfreq policy governor 'performance'");
MODULE_LICENSE("GPL");

fs_initcall(cpufreq_gov_performance_init);
module_exit(cpufreq_gov_performance_exit);
