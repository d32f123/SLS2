#include <semaphore.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "errors.h"

#define LETTERS_SIZE 26

char letters[LETTERS_SIZE] = "abcdefghijklmnopqrstuvwxyz";
pthread_mutex_t mutex;

struct timespec delays[3]; /* main, invert, inverse */ 

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

		nanosleep(&delays[1], NULL);
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

		nanosleep(&delays[2], NULL);
	}
	return NULL;
}

int main(int argc, char * argv[])
{
	pthread_t thread1;
	pthread_t thread2;
	int i;
	
	for (i = 1; i < 3; ++i)
	{
		delays[i - 1].tv_nsec = i < argc ? strtol(argv[i], NULL, 10) * 1000 * 1000: 1000000000;
		delays[i - 1].tv_sec = 0;
		if (delays[i - 1].tv_nsec <= 0)
		{
			delays[i - 1].tv_nsec = 1000 * 1000 * 1000;
		}
	}

	pthread_mutex_init(&mutex, NULL);

	pthread_create(&thread1, NULL, invert_letters, NULL);
	pthread_create(&thread2, NULL, inverse_letters, NULL);


	while(1)
	{
		pthread_mutex_lock(&mutex);
		printf("%s\n", letters);
		pthread_mutex_unlock(&mutex);

		nanosleep(&delays[0], NULL);
	}

	
	return 0;
}

