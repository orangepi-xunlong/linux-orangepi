/*
 * arch/arm/kernel/topology.c
 *
 * Copyright (C) 2011 Linaro Limited.
 * Written by: Vincent Guittot
 *
 * based on arch/sh/kernel/topology.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/percpu.h>
#include <linux/node.h>
#include <linux/nodemask.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <asm/cputype.h>
#include <asm/topology.h>


struct cputopo_arm cpu_topology[NR_CPUS];
EXPORT_SYMBOL_GPL(cpu_topology);

const struct cpumask *cpu_coregroup_mask(int cpu)
{
	return &cpu_topology[cpu].core_sibling;
}

/*
 * store_cpu_topology is called at boot when only one cpu is running
 * and with the mutex cpu_hotplug.lock locked, when several cpus have booted,
 * which prevents simultaneous write access to cpu_topology array
 */
void store_cpu_topology(unsigned int cpuid)
{
	struct cputopo_arm *cpuid_topo = &cpu_topology[cpuid];
	unsigned int mpidr;
	unsigned int cpu;

	/* If the cpu topology has been already set, just return */
	if (cpuid_topo->core_id != -1)
		return;

	mpidr = read_cpuid_mpidr();

	/* create cpu topology mapping */
	if ((mpidr & MPIDR_SMP_BITMASK) == MPIDR_SMP_VALUE) {
		/*
		 * This is a multiprocessor system
		 * multiprocessor format & multiprocessor mode field are set
		 */

		if (mpidr & MPIDR_MT_BITMASK) {
			/* core performance interdependency */
			cpuid_topo->thread_id = MPIDR_AFFINITY_LEVEL(mpidr, 0);
			cpuid_topo->core_id = MPIDR_AFFINITY_LEVEL(mpidr, 1);
			cpuid_topo->socket_id = MPIDR_AFFINITY_LEVEL(mpidr, 2);
		} else {
			/* largely independent cores */
			cpuid_topo->thread_id = -1;
			cpuid_topo->core_id = MPIDR_AFFINITY_LEVEL(mpidr, 0);
			cpuid_topo->socket_id = MPIDR_AFFINITY_LEVEL(mpidr, 1);
		}
	} else {
		/*
		 * This is an uniprocessor system
		 * we are in multiprocessor format but uniprocessor system
		 * or in the old uniprocessor format
		 */
		cpuid_topo->thread_id = -1;
		cpuid_topo->core_id = 0;
		cpuid_topo->socket_id = -1;
	}

	/* update core and thread sibling masks */
	for_each_possible_cpu(cpu) {
		struct cputopo_arm *cpu_topo = &cpu_topology[cpu];

		if (cpuid_topo->socket_id == cpu_topo->socket_id) {
			cpumask_set_cpu(cpuid, &cpu_topo->core_sibling);
			if (cpu != cpuid)
				cpumask_set_cpu(cpu,
					&cpuid_topo->core_sibling);

			if (cpuid_topo->core_id == cpu_topo->core_id) {
				cpumask_set_cpu(cpuid,
					&cpu_topo->thread_sibling);
				if (cpu != cpuid)
					cpumask_set_cpu(cpu,
						&cpuid_topo->thread_sibling);
			}
		}
	}
	smp_wmb();

	printk(KERN_INFO "CPU%u: thread %d, cpu %d, socket %d, mpidr %x\n",
		cpuid, cpu_topology[cpuid].thread_id,
		cpu_topology[cpuid].core_id,
		cpu_topology[cpuid].socket_id, mpidr);
}

/*
 * cluster_to_logical_mask - return cpu logical mask of CPUs in a cluster
 * @socket_id:		cluster HW identifier
 * @cluster_mask:	the cpumask location to be initialized, modified by the
 *			function only if return value == 0
 *
 * Return:
 *
 * 0 on success
 * -EINVAL if cluster_mask is NULL or there is no record matching socket_id
 */
int cluster_to_logical_mask(unsigned int socket_id, cpumask_t *cluster_mask)
{
	int cpu;

	if (!cluster_mask)
		return -EINVAL;

	for_each_online_cpu(cpu)
		if (socket_id == topology_physical_package_id(cpu)) {
			cpumask_copy(cluster_mask, topology_core_cpumask(cpu));
			return 0;
		}

	return -EINVAL;
}

#ifdef CONFIG_SCHED_HMP

static const char * const little_cores[] = {
       "arm,cortex-a7",
       NULL,
};

#ifdef CONFIG_OF
static bool is_little_cpu(struct device_node *cn)
{
       const char * const *lc;
       for (lc = little_cores; *lc; lc++)
               if (of_device_is_compatible(cn, *lc))
                       return true;
       return false;
}
#endif

void __init arch_get_fast_and_slow_cpus(struct cpumask *fast,
                                       struct cpumask *slow)
{
#ifdef CONFIG_OF
       struct device_node *cn = NULL;
       int cpu;
#endif
#if defined(CONFIG_SCHED_HMP) && defined(CONFIG_ARCH_SUN9I)
       unsigned int cpu_part;
       struct cpumask pre_fast, pre_slow;
       int          revert_mask=0;
       unsigned int cpu_cur = smp_processor_id();
#endif
       cpumask_clear(fast);
       cpumask_clear(slow);

       /*
        * Use the config options if they are given. This helps testing
        * HMP scheduling on systems without a big.LITTLE architecture.
        */
       if (strlen(CONFIG_HMP_FAST_CPU_MASK) && strlen(CONFIG_HMP_SLOW_CPU_MASK)) {
               if (cpulist_parse(CONFIG_HMP_FAST_CPU_MASK, fast))
                       WARN(1, "Failed to parse HMP fast cpu mask!\n");
               if (cpulist_parse(CONFIG_HMP_SLOW_CPU_MASK, slow))
                       WARN(1, "Failed to parse HMP slow cpu mask!\n");
#if defined(CONFIG_SCHED_HMP) && defined(CONFIG_ARCH_SUN9I)
               cpu_part = (read_cpuid_id() >> 4) & 0xfff;
               if((cpu_part == 0xc0f) && (cpumask_test_cpu(cpu_cur,slow)))
                    revert_mask = 1;
               else if((cpu_part == 0xc07) && (cpumask_test_cpu(cpu_cur,fast)))
                    revert_mask = 1;

               if(revert_mask)
               {
                      cpumask_clear(&pre_fast);
                      cpumask_clear(&pre_slow);
                      cpumask_copy(&pre_fast,fast);
                      cpumask_copy(&pre_slow,slow);
                      cpumask_clear(fast);
                      cpumask_clear(slow);
                      cpumask_copy(fast,&pre_slow);
                      cpumask_copy(slow,&pre_fast);
               }
#endif
               return;
       }

       /*
        * Else, parse device tree for little cores.
        */
#ifdef CONFIG_OF
	while ((cn = of_find_node_by_type(cn, "cpu"))) {

               const u32 *mpidr;
               int len;

               mpidr = of_get_property(cn, "reg", &len);
               if (!mpidr || len != 4) {
                       pr_err("* %s missing reg property\n", cn->full_name);
                       continue;
               }

               cpu = get_logical_index(be32_to_cpup(mpidr));
               if (cpu == -EINVAL) {
                       pr_err("couldn't get logical index for mpidr %x\n",
                                                       be32_to_cpup(mpidr));
                       break;
                } 
               if (is_little_cpu(cn))
                       cpumask_set_cpu(cpu, slow);
               else
                       cpumask_set_cpu(cpu, fast);
       }
#endif
       if (!cpumask_empty(fast) && !cpumask_empty(slow))
               return;

       /*
        * We didn't find both big and little cores so let's call all cores
        * fast as this will keep the system running, with all cores being
        * treated equal.
        */
       cpumask_setall(fast);
       cpumask_clear(slow);
}

struct cpumask hmp_slow_cpu_mask;
struct cpumask hmp_fast_cpu_mask;
void __init arch_get_hmp_domains(struct list_head *hmp_domains_list)
{

       struct hmp_domain *domain;

       arch_get_fast_and_slow_cpus(&hmp_fast_cpu_mask, &hmp_slow_cpu_mask);

       /*
        * Initialize hmp_domains
        * Must be ordered with respect to compute capacity.
        * Fastest domain at head of list.
        */
       if(!cpumask_empty(&hmp_slow_cpu_mask)) {
               domain = (struct hmp_domain *)
                       kmalloc(sizeof(struct hmp_domain), GFP_KERNEL);
               cpumask_copy(&domain->possible_cpus, &hmp_slow_cpu_mask);
               cpumask_and(&domain->cpus, cpu_online_mask, &domain->possible_cpus);
               list_add(&domain->hmp_domains, hmp_domains_list);
       }
       domain = (struct hmp_domain *)
               kmalloc(sizeof(struct hmp_domain), GFP_KERNEL);
       cpumask_copy(&domain->possible_cpus, &hmp_fast_cpu_mask);
       cpumask_and(&domain->cpus, cpu_online_mask, &domain->possible_cpus);
       list_add(&domain->hmp_domains, hmp_domains_list);
}
#endif /* CONFIG_SCHED_HMP */


/*
 * init_cpu_topology is called at boot when only one cpu is running
 * which prevent simultaneous write access to cpu_topology array
 */
void init_cpu_topology(void)
{
	unsigned int cpu;

	/* init core mask */
	for_each_possible_cpu(cpu) {
		struct cputopo_arm *cpu_topo = &(cpu_topology[cpu]);

		cpu_topo->thread_id = -1;
		cpu_topo->core_id =  -1;
		cpu_topo->socket_id = -1;
		cpumask_clear(&cpu_topo->core_sibling);
		cpumask_clear(&cpu_topo->thread_sibling);
	}
	smp_wmb();
}
