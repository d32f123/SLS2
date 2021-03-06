#include <semaphore.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>

#include "errors.h"

#define LETTERS_SIZE 26

char letters[LETTERS_SIZE] = "abcdefghijklmnopqrstuvwxyz";
pthread_mutex_t mutex;

useconds_t delays[3]; /* main, invert, inverse */ 

void * invert_letters()
{
	static char curr_low = 1;
	while(1) 
	{
		int i;
		pthread_mutex_lock(&mutex);
	
		for (i = 0; i < LETTERS_SIZE; ++i)
		{
			letters[i] += curr_low ? -32 : 32;
		}	
		curr_low = !curr_low;
		
		pthread_mutex_unlock(&mutex);

		usleep(delays[1]);
	}
	return NULL; 
}

void * inverse_letters()
{
	while(1) 
	{
		int i = 0;
		char swap;

		pthread_mutex_lock(&mutex);
	
		for (i = 0; i < LETTERS_SIZE / 2; ++i)
		{
			swap = letters[i];
			letters[i] = letters[LETTERS_SIZE - i - 1];
			letters[LETTERS_SIZE - i - 1] = swap; 
		}
	
		pthread_mutex_unlock(&mutex);

		usleep(delays[2]);
	}
	return NULL;
}

void sig_handler(int signo)
{
	if (signo == SIGINT)
	{
		printf("%s\n", "Destroying mutex");
		pthread_mutex_destroy(&mutex);
		_exit(0);
	}
}

int main(int argc, char * argv[])
{
	pthread_t thread1;
	pthread_t thread2;
	int i;
	
	for (i = 1; i < 3; ++i)
	{
		delays[i - 1] = i < argc ? strtol(argv[i], NULL, 10) * 1000: 1000000;
		if (delays[i - 1] <= 0)
		{
			delays[i - 1] = 1000000;
		}
	}

	pthread_mutex_init(&mutex, NULL);

	if (signal(SIGINT, sig_handler) == SIG_ERR)
		perror("Could not catch SIGINT\n");

	pthread_create(&thread1, NULL, invert_letters, NULL);
	pthread_create(&thread2, NULL, inverse_letters, NULL);


	while(1)
	{
		pthread_mutex_lock(&mutex);
		printf("%s\n", letters);
		pthread_mutex_unlock(&mutex);

		usleep(delays[0]);
	}

	
	return 0;
}

