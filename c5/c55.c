#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "errors.h"

#define LETTERS_SIZE 26

char letters[LETTERS_SIZE] = "abcdefghijklmnopqrstuvwxyz";

int s_id;

void * invert_letters()
{
	struct sembuf lock = {1, -1, SEM_UNDO};
	struct sembuf unlock = {0, 1, SEM_UNDO};
	static char curr_low = 1; /* if letters are currently in lower register */

	while(1) 
	{
		int i;
	
		if (semop(s_id, &lock, 1) == -1)
		{
			perror("semop");
			return NULL;
		}

		for (i = 0; i < LETTERS_SIZE; ++i)
		{
			letters[i] += curr_low ? -32 : 32;
		}	
		curr_low = !curr_low;
		
		if (semop(s_id, &unlock, 1) == -1)
		{
			perror("semop");
			return NULL;
		}

	}
	return NULL; 
}

void * inverse_letters()
{
	struct sembuf lock = {2, -1, SEM_UNDO};
	struct sembuf unlock = {0, 1, SEM_UNDO};

	while(1) 
	{
		int i = 0;
		char swap;

		if (semop(s_id, &lock, 1) == -1)
		{
			perror("semop");
			return NULL;
		}
	
		for (i = 0; i < LETTERS_SIZE / 2; ++i)
		{
			swap = letters[i];
			letters[i] = letters[LETTERS_SIZE - i - 1];
			letters[LETTERS_SIZE - i - 1] = swap; 
		}
	
		if (semop(s_id, &unlock, 1) == -1)
		{
			perror("semop");
			return NULL;
		}
	}
	return NULL;
}

int main()
{
	pthread_t thread1;
	pthread_t thread2;
	struct sembuf lock = {0, -1, SEM_UNDO};
	struct sembuf unlock = {1, 1, SEM_UNDO};
	char launch_first = 1;

	s_id = semget(IPC_PRIVATE, 3, 0666);
	if (s_id == -1)
	{
		perror("semget");
		return SEMGET_FAILCODE;
	}

	pthread_create(&thread1, NULL, invert_letters, NULL);
	pthread_create(&thread2, NULL, inverse_letters, NULL);

	

	while(1)
	{

		printf("%s\n", letters);

		unlock.sem_num = launch_first ? 1 : 2;
		if (semop(s_id, &unlock, 1) == -1)
		{
			perror("semop");
			return SEMOP_FAILCODE;
		}

		sleep(1);

		if (semop(s_id, &lock, 1) == -1)
		{
			perror("semop");
			return SEMOP_FAILCODE;
		}

		launch_first = !launch_first;
	}

	
	return 0;
}

