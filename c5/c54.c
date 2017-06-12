#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>


#define LETTERS_SIZE 26

char letters[LETTERS_SIZE] = "abcdefghijklmnopqrstuvwxyz";
sem_t sem_array[3];

void * invert_letters()
{
	static char curr_low = 1; /* if letters are currently in lower register */
	while(1) 
	{
		int i;
		sem_wait(sem_array + 1);
	
		for (i = 0; i < LETTERS_SIZE; ++i)
		{
			letters[i] += curr_low ? -32 : 32;
			
		}	
		curr_low = !curr_low;
		sem_post(sem_array);
	}
	return NULL; 
}

void* inverse_letters()
{
	while(1) 
	{
		int i = 0;
		char swap;

		sem_wait(sem_array + 2);
	
		for (i = 0; i < LETTER_SIZE / 2; ++i)
		{
			swap = letters[i];
			letter[i] = letters[LETTER_SIZE - i - 1];
			letter[LETTER_SIZE - i - 1] = swap; 
		}
	
		sem_post(sem_array);
	}
	return NULL;
}

int main()
{
	pthread_t thread1;
	pthread_t thread2;
	int i;
	char launch_first = 1;

	for (i = 0; i < 3; i++)
	{
		if (sem_init(&sem_array[i], 1, (i == 0) ? 1 : 0) == -1)
		{
			perror("sem_init call failed");
			exit(SEMINIT_FAILCODE);
		}
	}
	pthread_create(&thread1, NULL, invert_letters, NULL);
	pthread_create(&thread2, NULL, inverse_letters, NULL);


	while(1)
	{
	
		sem_wait(&sem_array[0]);
		printf("%s\n", letter);

		sem_post(&sem_array[launch_first ? 1 : 2]);
		launch_first = !launch_first;
		sleep(1);
	}

	return 0;
}

