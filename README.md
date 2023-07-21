# Operating System Programming Project - Nachos
1. Topic: Scheduling
2. Implementation:
    (1) Implement preemptive shortest job first
     - If current thread has the same burst time, the one with greater id should be executed first
     - Approximate burst time formula shown below as consideration:
     - t_i = 0.5 * T + 0.5 *t_i-1 , i> 0, t_0 = 0
    (2) Add a debugging flag j in your code and use the DEBUG('j', expression) macro to print folloeing messages.
