CFS:

For the Completely Fair Scheduler (CFS), I added to struct proc a nice value for process priority, a corresponding weight derived from the nice value, and a vruntime field to record normalized CPU usage. The scheduler keeps runnable processes ordered by increasing vruntime and always chooses the process with the smallest vruntime to execute next. I applied time slice computation based on a desired latency of 48 ticks with a minimum slice of 3 ticks and incremented vruntime after each slice. There was also logging to print out PID and vruntime of all runnable processes prior to each scheduling choice, enabling verification of proper behavior. Compilation is managed through SCHEDULER=CFS. In general, FCFS offers basic scheduling based on arrival time, whereas CFS provides equal CPU allocation among processes based on priority and previous CPU usage.


[Scheduler Tick]
PID: 2 | vRuntime: 8
[Scheduler Tick]
Scheduling PID 2 lowest vRuntime 8 | slice=48
[Scheduler Tick]
PID: 2 | vRuntime: 8
Scheduling PID 2 lowest vRuntime 8 | slice=48


FCFS:


For First-Come, First-Served (FCFS) scheduler, I changed the scheduler() function in proc.c to choose the RUNNABLE process with the smallest creation time (ctime) rather than the default round-robin. The struct proc already contained the creation time field, and I utilized it to order processes. This guarantees that processes are run in the order that they arrive, and the running process is not preempted until it terminates or blocks. Compilation is managed through the SCHEDULER=FCFS macro such that the kernel may switch to FCFS scheduling when necessary.


[FCFS] Scheduling PID 2 | ctime=16 |
[FCFS] Scheduling PID 2 | ctime=16 |
$ readcount
[FCFS] Scheduling PID 2 | ctime=16 |
[FCFS] Scheduling PID 3 | ctime=434 |
[FCFS] Scheduling PID 3 | ctime=434 |




FCFS executes processes strictly in order of arrival, which can cause long waiting times for short processes if a long process arrives first, although it has minimal context-switching overhead. The CFS scheduler achieves a balance by always running the process with the lowest virtual runtime, ensuring fair CPU distribution according to process weight and priority. As a result, CFS generally reduces starvation and balances waiting times better than FCFS