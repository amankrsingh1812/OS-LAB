### Testing 

We used created 3 files for testing and examining our schedular under varied circumstances to validate its rebustness and take observations. The files are described below:

#### ts2.c

This file is used to illustrate the significance of the burst time of the process in the order of execution of the processes waiting to be run.

A function called `childProcess()` is called by a newly forked child which performs extensive CPU based calculation, more specifically it calculates the 300000000th term of the fibonacci sequence (module 1e9+7).

The `main` function forks two processes and for each of them calls `childProcess()`, which before the actual execution starts a timer using the `uptime()` system call, sets the burst time and preempts itself back to the ready queue (i.e. becomes runnable). Then it completes the above fibonacci computation and before exiting stops the timer (i.e. again calls `uptime()` and computes the 
difference with above `uptime()` value) and prints its status.

Initially both the child processes are forked and the driver process after creating them waits for them to finish (after calling `wait()`). These child processes then start timer and set burst time. Once both have obtained the _positive_ burst times and driver process is waiting, the child with the lower burst time is scheduled. Once it completes executing, the other child executes. This is clearly reflected in the output too.
(Note that as default burst time is 0, the driver/parent process gets scheduled when it is available)

Qualitatively, (see summary in output) the one with lower burst time is executed first.
Quantitatively, (see output before summary) the turnaround time (time it took to complete its execution after being ready for execution) for the second process is almost _double_ the turnaround time for the first process. This is due to the fact that both the processes are ready for execution at almost the same time and one process executes itself while the other one waits for its execution and then is executed.

< ADD OUTPUT IMAGE HERE >

#### test_scheduler.c

This file is used to study the behaviour of implemented SJF schedular when there is a mixture of CPU bound and IO bound processes.
The test file `test_scheduler.c` contains the following functions:

* `looper()`: This function simply runs the inner loop `loopfac` number of times. The inner loop runs with an empty body for 10^8 iteractions. Thus in total the number of iterations is `loopfac * 10^8`. It is a means to include an CPU-bound process

* `userIO()`: This function simply takes the reader input from STDIN and prints it back on STDOUT. It is a means to include an IO-bound process, which waits for user input while the other processes can run.

* `fileIO()`: This function simply reads `readBytes` bytes from the file `filename` from the xv6 file system. It is a means to include a file-IO bound process, which reads content while the other processes are RUNNABLE.

The driver code is mainly responsible for creating 5 child processes and calling the above functions to perform different tasks in different child processes. It passes the required parameters like the burst time to be set and `loopfac` in case of CPU bound loop based processes. The code then uses the PIDs to determine and print a summary of the order in which the processes completed their execution.

Five child processes are being forked from the parent process, and their PIDs are being saved for later use (for printing the final order of execution):
1. A loop which runs 10^8 loop 2 times, and burst time set to 8.
2. A process for user IO, with burst time set to 1.
3. A loop which runs 10^8 loop 4 times, and burst time set to 10.
4. A process for file IO, where we read 1500 bytes, with burst time set to 5.
5. A loop which runs 10^8 loop 1 time, and burst time set to 6.
6. A process for file IO, where we read 500 bytes, with burst time set to 3.

When `test_scheduler.c` is run, various important observations are made:
* The parent process runs whenever it is not in the SLEEP state (ie. it has not called `wait()`). This is because by default the burst time is initialised to 0 for all processes, so the parent process (and other system process) gets scheduled first as SJF here works on burst time.
* Each child process first sets its burst time, using a modified `set_burst_time` syscall, which sets its burst time and then calls `yield()` to preempt the child process. This is done because the burst time is being set inside the child process, and we want the child processes to actually start execution once all the child processes have been given burst times.
* Since there is a child which reads user input (the second process forked) and prints it, the order in which the child processes finish executing is _partly dependent_ on _when_ the user gives the input. It first performs some printing, then waits for the user to input something. This waiting time determines how long it would be SLEEPING (and hence, won't be RUNNABLE). Since it has the shortest burst time, as soon as the user input has been read, the next process that will be scheduled is this process. Hence a fast user input means this processes finishes quickly, otherwise it may even finish in the end. 
<!-- However, it is impossible for it to finish first, as there are pure CPU bound processes which once scheduled, will complete all the instructions without sleeping/waiting. -->