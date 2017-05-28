#include <semaphore.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>


char letter[26] = "abcdefghijklmnopqrstuvwxyz";
int s_id;

void* invert()
{
	
	struct sembuf lock = {1, -1, SEM_UNDO};
	struct sembuf unlock = {0, 1, SEM_UNDO};

	while(1) 
	{
		int i;
	
		if (semop(s_id, &lock, 1) == -1)
		{
			perror("semop");
			return NULL;
		}

		for ( i = 0; i < 26; i++)
		{
			letter[i] += (letter[i] >= 'a') ? -32 : 32;
		}	
		
		if (semop(s_id, &unlock, 1) == -1)
		{
			perror("semop");
			return NULL;
		}

	}
	return NULL; 
}

void* reorder()
{
	struct sembuf lock = {2, -1, SEM_UNDO};
	struct sembuf unlock = {0, 1, SEM_UNDO};

	while(1) 
	{
		if (semop(s_id, &lock, 1) == -1)
		{
			perror("semop");
			return NULL;
		}
		
		int i = 0;
		char t;
	
		for ( i = 0; i < 13; i++)
		{
			t  = letter[i];
			letter[i] = letter[25-i];
			letter[25-i] = t; 
		}
	
		if (semop(s_id, &unlock, 1) == -1)
		{
			perror("semop");
			return NULL;
		}
	}
	return NULL;
}
void handle_sig(int sig_number)
{
	(void)sig_number;
	_exit(0);
}

int main()
{
	pthread_t thread1;
	pthread_t thread2;
	struct sembuf lock = {0, -1, SEM_UNDO};
	struct sembuf unlock = {1, 1, SEM_UNDO};

	s_id = semget(IPC_PRIVATE, 3, 0600);
	if (s_id == -1)
	{
		perror("semget");
		return 1;
	}

	pthread_create(&thread1, NULL, invert, NULL);
	pthread_create(&thread2, NULL, reorder, NULL);

	

	while(1)
	{

		printf("%s\n", letter);

		unlock.sem_num = 3 - unlock.sem_num;
		if (semop(s_id, &unlock, 1) == -1)
		{
			perror("semop");
			return 1;
		}

		sleep(1);

		if (semop(s_id, &lock, 1) == -1)
		{
			perror("semop");
			return 1;
		}

	}

	
	return 0;
}

