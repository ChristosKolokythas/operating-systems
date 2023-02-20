#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>

#define SHMSIZE 2048

typedef struct {
	char request_id[100];
	char response[100];
} memory;

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

void sem_init(int sem_id){
	union semun arg0;
	union semun arg1;

	arg0.val = 0;
	arg1.val = 1;

	if (semctl(sem_id, 0, SETVAL, arg1) == -1){
		perror("semctl0: ");
		exit(EXIT_FAILURE);
	}

	if (semctl(sem_id, 1, SETVAL, arg0) == -1){
		perror("semctl1: ");
		exit(EXIT_FAILURE);
	}

	if (semctl(sem_id, 2, SETVAL, arg1) == -1){
		perror("semctl2: ");
		exit(EXIT_FAILURE);
	}

	if (semctl(sem_id, 3, SETVAL, arg0) == -1){
		perror("semctl3: ");
		exit(EXIT_FAILURE);
	}
}

void sem_down(int sem_id, int sem_number){
	struct sembuf sem_oper;

	sem_oper.sem_num = sem_number;
	sem_oper.sem_op = -1;
	sem_oper.sem_flg = 0;

	if (semop(sem_id, &sem_oper, 1) != 0){
		perror("semop");
	}
}

void sem_up(int sem_id, int sem_number) {
	struct sembuf sem_oper;

	sem_oper.sem_num = sem_number;
	sem_oper.sem_op = 1;
	sem_oper.sem_flg = 0;

	if (semop(sem_id, &sem_oper, 1) != 0){
		perror("semop");
	}
}

char *trimwhitespace(char *str){
	char *end;

	while (isspace((unsigned char)*str))
		str++;

	if (*str == 0)
		return str;

	end = str + strlen(str) - 1;

	while (end > str && isspace((unsigned char) *end))
		end--;

	return str;
}

int main(int argc, char const *argv[]){
	
	int input_fd, lines = 0;
	int k_child, n_trans;
	char *file_buffer, *token, **file;
	char *request, *response;
	off_t file_size;
	pid_t pid = 0;
	int shmid, semid;
	memory *shmaddr;

	// Check if the user supplied the corrent number of parameters
	if (argc != 4){
		// Let the user know the correct programma usage
		printf("Usage is %s <filename> <K children> <N transactions>\n", argv[0]);

		// Exit the program. 
		exit(EXIT_FAILURE);
	}

	// Convert the arguments from string to int
	k_child= atoi(argv[2]);
	n_trans= atoi(argv[3]);

	// Initialize the shared memory segment. Check for possible errors
	if ((shmid = shmget(IPC_PRIVATE, SHMSIZE, IPC_CREAT | 0666)) == -1){
		perror("shmget: ");
		exit(EXIT_FAILURE);
	}

	// Attach the memory to the procces. Check for possible errors
	shmaddr = shmat(shmid, 0, 0);
	if (shmaddr == (memory *) -1){
		perror("shmat: ");
		exit(EXIT_FAILURE);
	}

	// Initialize the 4 semaphores needed for synchronization. Check for errors
	if ((semid = semget(IPC_PRIVATE, 4, IPC_CREAT | 0666)) == -1){
		perror("semget");
		exit(EXIT_FAILURE);
	}

	// Call sem_init function to initialize the semaphore values
	sem_init(semid);

	// Open the file given as input.
	input_fd = open(argv[1], O_RDONLY);

	// Check for errors while opening the file
	if (input_fd == -1){
		// Print the error associated with the errno set by open
		perror("Error: ");
		exit(EXIT_FAILURE);
	}

	// Calculate the file size
	file_size = lseek(input_fd, 0, SEEK_END);

	// Check for errors while getting the file size
	if (file_size == -1){
		// Print the error associated with the errno set by lseek
		perror("Error: ");
		exit(EXIT_FAILURE);
	}

	// Reset the file offset to the beginning
	lseek(input_fd, 0, SEEK_SET);

	// Allocate space for the entire file
	file_buffer = malloc(file_size * sizeof(char));

	// Read the entire file into the buffer
	if (read(input_fd, file_buffer, file_size) != file_size)
		perror("Error");

	// Split the file into lines. This is done by splitting
	// the buffer into tokens, using the '\n' character as 
	// the delimiter
	token = strtok(file_buffer, "\n");

	// file is treated as an array of lines size. Each element
	// contains a single line from the file
	file = malloc(1 * sizeof(char*));

	// Split the entire file
	while (token != NULL){
		// Allocate space for the line. We use exactly the space needed
		// for each line
		file[lines] = malloc(strlen(token) * sizeof(char));
		// Copy the contents
		strcpy(file[lines], trimwhitespace(token));

		// Increment the line counter
		++lines;

		// Realloc the file pointer, so it points to a region with one
		// char* space.
		file = realloc(file, (lines+1)*sizeof(char*));

		// Get the next token
		token = strtok(NULL, "\n");
	}

	// Some info regarding the file
	//printf("File size: %ldB\n", file_size);
	//printf("Number of lines: %d\n", lines);

	// Start creating child processes. We create k_child processes
	for (int i = 0; i < k_child; ++i){
		pid = fork();
		//The child process does nothing for now
		if (pid == 0)
			break;
	}

	// The parent's code
	if (pid > 0){
		int status;

		// The parent will receive children * transactions requests.
		// So the loop will go up to the product of these two
		for (int i = 0; i < k_child * n_trans; ++i){
			// Down the semaphore #1, so only the parent can access the shared memory
			sem_down(semid, 1);
			// Allocate a temporary string for the line number of the file requested to be saved
			char *temp = malloc(strlen(shmaddr->request_id) * sizeof(char));
			// Copy the contents from the shared memory to the temp string
			strcpy(temp, shmaddr->request_id);
			// Up the semaphore #0, so another child can write to the shared memory
			sem_up(semid, 0);

			// Down the semaphore #2, so only the parent can write to the shared memory
			sem_down(semid, 2);
			// Convert the line number from string to int
			int lid = atoi(temp);
			// Free the space taken up by temp
			free(temp);
			// Copy the line data from the "array" to the shared memory
			strcpy(shmaddr->response, file[lid]);
			// Up the semaphore #3
			sem_up(semid, 3);
		}

		// Check the return codes of all the processes
		for (int i = 0; i < k_child; ++i){
			wait(&status);
		}
	}
	// Child processes code
	else {
		// Set the rand seed, according to each child's pid
		srand(getpid());

		double total_time = 0;
		// Repeat for n transactions
		for (int i = 0; i < n_trans; ++i){
			// Generate a random number in [0, lines)
			int request_id = rand() % (lines);
			// Convert the int to string. Idea taken from
			// https://stackoverflow.com/questions/8257714/how-to-convert-an-int-to-string-in-c
			int len = snprintf(NULL, 0, "%d", request_id);
			char *temp = malloc((len + 1) * sizeof(char));
			snprintf(temp, len + 1, "%d", request_id);

			// Down sem #0
			sem_down(semid, 0);
			// Hold the clock when the child starts the transaction
			clock_t begin = clock();
			// Copy the line number to the shared memory
			stpcpy((shmaddr->request_id), temp);
			// Free the allocated space for the line number
			free(temp);
			// Up sem #1
			sem_up(semid, 1);

			// Down sem #3
			sem_down(semid, 3);

			// Allocate space for the parent's response
			temp = malloc(strlen((shmaddr->response)) * sizeof(char));
			// Copy the contents from the shared memory
			strcpy(temp, shmaddr->response);
			// Print the response
			printf("Child[%d]Line %d -> %s\n", getpid(), request_id, temp);
			// Stop the count
			clock_t end = clock();

			// Add the delay to the total time sum. Used for the average calculation
			total_time += (double)(end - begin) / CLOCKS_PER_SEC;
			// Up sem #2
			sem_up(semid, 2);
		}

		// Print the child's stats
		printf("Child[%d]: Average wait time: %lf[s]\n", getpid(), total_time / n_trans);

		// Exit
		exit(EXIT_SUCCESS);

	}
	
	return 0;
}
