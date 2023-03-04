# Operating Systems Project
This program reads a file and splits its contents into lines. Then, the program creates multiple child processes to execute transactions on a shared memory region. The transactions are described in the file read earlier.

The program initializes four semaphores for synchronizing access to the shared memory region. The shared memory region consists of a structure with two char arrays, one for storing the request id and one for storing the response.

The main function of the program takes three arguments, the filename, the number of child processes, and the number of transactions each child process will execute.

The code first checks if the user supplied the correct number of parameters. It then converts the string arguments to integers.

After that, it initializes the shared memory region and the semaphores. It then reads the entire file into a buffer, splits it into lines, and stores each line in an array of strings.

Finally, the program creates k_child number of child processes, and each child executes n_trans transactions. The transactions are selected randomly from the array of strings read earlier. Each transaction is implemented in a child process, which uses the shared memory region to store the request id and the response. The semaphores ensure that each child process and parent process access the shared memory region in a synchronized manner.
