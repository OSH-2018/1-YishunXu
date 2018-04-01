# OSH-Lab01

PB16051314 徐亦舜

## 实验环境

1.Ubuntu 16.04

2.GDB 7.11.1

3.GCC 5.4.0

4.qeum 2.12.0-rc1

5.busybox 1.28.1

## 实验准备

### 1.内核准备

官网下载最新版本内核

```
tar -xf linux-4.15.13.tar.xz
make menuconfig
```

（选择Kernel hacking->Compile-time->checks and compiler options->Compile #the kernel with debug info）

```
make x86_64_defconfig
make
```



### 2.qeum准备

```
下载quem最新版本：wget https://download.qemu.org/qemu-2.12.0-rc1.tar.xz
./configure
make
make install
### 
```

### 3.busybox准备

```
wget http://busybox.net/downloads/busybox-1.28.1.tar.bz2
tar -xf busybox-1.28.1.tar.bz2
make menuconfig
```

（勾选Settings->Build Options->Build static binary(no share libs)）

```
make
make install
```



### 4.busybox制作rootfs

```
cd _install
mkdir proc sys dev etc etc/init.d
touch etc/init.d/rcS
vim etc/init.d/rcS
```

*编辑rcS中的内容：
	

```
#!/bin/sh
	mount -t proc none /proc
	mount -t sysfs none /sys
	/sbin/mdev -s
```



```
cd ..
chmod +x _install/etc/init.d/rcS
cd _install
find . | cpio -o --format=newc > ../rootfs.img
```



## 实验调试

关键进程

1.start_kernel函数

```
asmlinkage void __init start_kernel(void)
{
    char * command_line;
    extern const struct kernel_param __start___param[], __stop___param[];
    /*这两个变量为地址指针，指向内核启动参数处理相关结构体段在内存中的位置（虚拟地址）。 
    声明传入参数的外部参数对于ARM平台，位于 include\asm-generic\vmlinux.lds.h*/  
       /* 
        * Need to run as early as possible, to initialize the 
        * lockdep hash: 
          lockdep是一个内核调试模块，用来检查内核互斥机制（尤其是自旋锁）潜在的死锁问题。 
        */ 
    
    lockdep_init();//初始化内核依赖的关系表，初始化hash表  
    smp_setup_processor_id();//获取当前CPU,单处理器为空 
    debug_objects_early_init();//对调试对象进行早期的初始化,其实就是HASH锁和静态对象池进行初始化  

        /* 
              * Set up the the initial canary ASAP: 
               初始化栈canary值 
               canary值的是用于防止栈溢出攻击的堆栈的保护字 。 
            */ 
    boot_init_stack_canary();
     /*1.cgroup: 它的全称为control group.即一组进程的行为控制.  
           2.该函数主要是做数据结构和其中链表的初始化  
           3.参考资料： Linux cgroup机制分析之框架分析 
         */  

    cgroup_init_early();

    local_irq_disable();//关闭系统总中断（底层调用汇编指令）
    early_boot_irqs_disabled = true;

/*
 * Interrupts are still disabled. Do necessary setups, then
 * enable them
 */
     boot_cpu_init();//1.激活当前CPU（在内核全局变量中将当前CPU的状态设为激活状态）  
        page_address_init();//高端内存相关，未定义高端内存的话为空函数  
        pr_notice("%s", linux_banner); 
        /*1.内核构架相关初始化函数,可以说是非常重要的一个初始化步骤。 
        其中包含了处理器相关参数的初始化、内核启动参数（tagged list）的获取和前期处理、 
        内存子系统的早期的初始化（bootmem分配器）。 主要完成了4个方面的工作，一个就是取得MACHINE和PROCESSOR的信息然或将他们赋值 
        给kernel相应的全局变量，然后呢是对boot_command_line和tags接行解析，再然后呢就是 
        memory、cach的初始化，最后是为kernel的后续运行请求资源″**/  
        setup_arch(&command_line);  
        /*1.初始化代表内核本身内 
        存使用的管理结构体init_mm。  
        2.ps：每一个任务都有一个mm_struct结构以管理内存空间，init_mm是内核的mm_struct，其中：  
        3.设置成员变量* mmap指向自己，意味着内核只有一个内存管理结构;  
        4.设置* pgd=swapper_pg_dir，swapper_pg_dir是内核的页目录(在arm体系结构有16k， 所以init_mm定义了整个kernel的内存空间)。  
        5.这些内容涉及到内存管理子系统*/  
        mm_init_owner(&init_mm, &init_task);  
        mm_init_cpumask(&init_mm);//初始化CPU屏蔽字  
        /*1.对cmdline进行备份和保存：保存未改变的comand_line到字符数组static_command_line［］ 中。保存  boot_command_line到字符数组saved_command_line［］中 
    */  
        setup_command_line(command_line); 
        /*如果没有定义CONFIG_SMP宏，则这个函数为空函数。如果定义了CONFIG_SMP宏，则这个setup_per_cpu_areas()函数给每个CPU分配内存，并拷贝.data.percpu段的数据。为系统中的每个CPU的per_cpu变量申请空间。 
        */  
        /*下面三段1.针对SMP处理器的内存初始化函数，如果不是SMP系统则都为空函数。 (arm为空)  
        2.他们的目的是给每个CPU分配内存，并拷贝.data.percpu段的数据。为系统中的每个CPU的per_cpu变量申请空间并为boot CPU设置一些数据。  
        3.在SMP系统中，在引导过程中使用的CPU称为boot CPU*/ 
    
    setup_nr_cpu_ids();
    setup_per_cpu_areas();
    smp_prepare_boot_cpu();    /* arch-specific boot-cpu hooks */

    build_all_zonelists(NULL, NULL);//  建立系统内存页区(zone)链表 
    page_alloc_init();//内存页初始化 

    pr_notice("Kernel command line: %s\n", boot_command_line);
    parse_early_param();//  解析早期格式的内核参数  
        /*函数对Linux启动命令行参数进行在分析和处理, 
        当不能够识别前面的命令时，所调用的函数。*/  
    parse_args("Booting kernel", static_command_line, __start___param,
           __stop___param - __start___param,
           -1, -1, &unknown_bootoption);

    jump_label_init();

    /*
     * These use large bootmem allocations and must precede
     * kmem_cache_init()
     */
    setup_log_buf(0);
     /*初始化hash表，以便于从进程的PID获得对应的进程描述指针，按照开发办上的物理内存初始化pid hash表 
        */ 
    pidhash_init();
    vfs_caches_init_early();//建立节点哈希表和数据缓冲哈希表 
    sort_main_extable();//对异常处理函数进行排序
    trap_init();//初始化硬件中断 
    mm_init();//    Set up kernel memory allocators     建立了内核的内存分配器   

    /*
     * Set up the scheduler prior starting any interrupts (such as the
     * timer interrupt). Full topology setup happens at smp_init()
     * time - but meanwhile we still have a functioning scheduler.
     */
    sched_init();//核心进程调度器初始化
    /*
     * Disable preemption - early bootup scheduling is extremely
     * fragile until we cpu_idle() for the first time.
     */
    preempt_disable();//禁止调度
     //  先检查中断是否已经打开，若打开，输出信息后则关闭中断。
    if (WARN(!irqs_disabled(), "Interrupts were enabled *very* early, fixing it\n"))
        local_irq_disable();
    idr_init_cache();//创建idr缓冲区  
    rcu_init();//互斥访问机制 
    tick_nohz_init();
    context_tracking_init();
    radix_tree_init();
    /* init some links before init_ISA_irqs() */
    early_irq_init();
    init_IRQ();//使用alpha_mv结构和entry.S入口初始化系统IRQ
    tick_init();
    init_timers();//定时器初始化
    hrtimers_init();//高精度时钟初始化
    softirq_init();//软中断初始化
    timekeeping_init();//   初始化资源和普通计时器 
    time_init();//时间、定时器初始化（包括读取CMOS时钟、估测主频、初始化定时器中断等）
    sched_clock_postinit();
    perf_event_init();
    profile_init();//   对内核的一个性能测试工具profile进行初始化。
    call_function_init();
    WARN(!irqs_disabled(), "Interrupts were enabled early\n");
    early_boot_irqs_disabled = false;
    local_irq_enable();//使能中断 

    kmem_cache_init_late();//kmem_cache_init_late的目的就在于完善slab分配器的缓存机制.

    /*
     * HACK ALERT! This is early. We're enabling the console before
     * we've done PCI setups etc, and console_init() must be aware of
     * this. But we do want output early, in case something goes wrong.
     */
    console_init();//初始化控制台以显示printk的内容  
    if (panic_later)
        panic("Too many boot %s vars at `%s'", panic_later,
              panic_param);

    lockdep_info();//   如果定义了CONFIG_LOCKDEP宏，那么就打印锁依赖信息，否则什么也不做 

    /*
     * Need to run this when irqs are enabled, because it wants
     * to self-test [hard/soft]-irqs on/off lock inversion bugs
     * too:
     */
    locking_selftest();

#ifdef CONFIG_BLK_DEV_INITRD
    if (initrd_start && !initrd_below_start_ok &&
        page_to_pfn(virt_to_page((void *)initrd_start)) < min_low_pfn) {
        pr_crit("initrd overwritten (0x%08lx < 0x%08lx) - disabling it.\n",
            page_to_pfn(virt_to_page((void *)initrd_start)),
            min_low_pfn);
        initrd_start = 0;
    }
#endif
    page_cgroup_init();
    debug_objects_mem_init();
    kmemleak_init();
    setup_per_cpu_pageset();
    numa_policy_init();
    if (late_time_init)
        late_time_init();
    sched_clock_init();
    calibrate_delay();//延迟校准（获得时钟jiffies与CPU主频ticks的延迟）
    pidmap_init();//进程号位图初始化，一般用一个錺age来表示所有进程的錺id占用情况  
    anon_vma_init();//  匿名虚拟内存域（ anonymous VMA）初始化  
    acpi_early_init();
#ifdef CONFIG_X86
    if (efi_enabled(EFI_RUNTIME_SERVICES))
        efi_enter_virtual_mode();
#endif
#ifdef CONFIG_X86_ESPFIX64
    /* Should be run before the first non-init thread is created */
    init_espfix_bsp();
#endif
    thread_info_cache_init();//获取thread_info缓存空间，大部分构架为空函数（包括ARM  
    cred_init();//任务信用系统初始化。详见：Documentation/credentials.txt  
    fork_init(totalram_pages);//进程创建机制初始化。为内核"task_struct"分配空间，计算最大任务数。  
    proc_caches_init();//初始化进程创建机制所需的其他数据结构，为其申请空间。 
    buffer_init();//块设备读写缓冲区初始化（同时创建"buffer_head"cache用户加速访问）
    key_init();//内核密钥管理系统初始化 
    security_init();//内核安全框架初始化
    dbg_late_init();
    vfs_caches_init(totalram_pages);//虚拟文件系统（VFS）缓存初始化  
    signals_init();//信号管理系统初始化 
    /* rootfs populating might need page-writeback */
    page_writeback_init();//页写回机制初始化
#ifdef CONFIG_PROC_FS
    proc_root_init();//proc文件系统初始化
#endif
    cgroup_init();//control group正式初始化  
    cpuset_init();//CPUSET初始化。 参考资料：《多核心計算環境—NUMA與CPUSET簡介》
    taskstats_init_early();//任务状态早期初始化函数：为结构体获取高速缓存，并初始化互斥机制。
    delayacct_init();//任务延迟初始化 

    check_bugs();//检查CPU BUG的函数，通过软件规避BUG 

    sfi_init_late();//功能跟踪调试机制初始化，ftrace 是 function trace 的简称 

    if (efi_enabled(EFI_RUNTIME_SERVICES)) {
        efi_late_init();
        efi_free_boot_services();
    }

    ftrace_init();

    /* Do the rest non-__init'ed, we're now alive */
    rest_init();// 虽然从名字上来说是剩余的初始化。但是这个函数中的初始化包含了很多的内容  
}
```

在start_kernel()，我们看到，调用的第一个函数是lockdep_init()，这个函数会初始化内核死锁检测机制的哈希表。接下来，则会调用set_task_stack_end_magic(&init_task)函数

2.setup_arch函数

setup_arch()函数是体系结构相关的内核初始化过程

除此之外，还有很多函数非常重要。如 `set_task_stack_end_magic()`、`smp_setup_processor_id()`、`cgroup_init_early()`等等



