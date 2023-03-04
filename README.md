# Operating Systems Project 
This program performs the following tasks: first, it reads a file and splits its contents into lines. Next, it creates multiple child processes to execute transactions on a shared memory region based on the information in the file.

The shared memory region consists of a structure with two char arrays, one for storing the request ID and one for storing the response. To ensure synchronized access to this shared memory region, four semaphores are initialized.

The main function of the program accepts three arguments: the filename, the number of child processes, and the number of transactions each child process should execute. The code validates the input parameters by checking if the correct number of parameters was supplied and converting the string arguments to integers.

Next, the program initializes the shared memory region and semaphores. It reads the entire file into a buffer, splits it into lines, and stores each line in an array of strings.

Finally, the program creates a specified number of child processes, and each child executes a specified number of transactions. The transactions are randomly selected from the array of strings read earlier. Each transaction is implemented in a child process that utilizes the shared memory region to store the request ID and response. The semaphores ensure that the parent process and each child process access the shared memory region in a synchronized manner.
