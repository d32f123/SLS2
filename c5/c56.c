#include <semaphore.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>


char letter[26] = "abcdefghijklmnopqrstuvwxyz";
pthread_mutex_t mutex;

void* invert()
{
	while(1) 
	{
		int i;
		pthread_mutex_lock(&mutex);
	
		for ( i = 0; i < 26; i++)
		{
			letter[i] += (letter[i] >= 'a') ? -32 : 32;
		}	
		
		pthread_mutex_unlock(&mutex);
	}
	return NULL; 
}

void* reorder()
{
	while(1) 
	{
		pthread_mutex_lock(&mutex);

		int i = 0;
		char t;
	
		for ( i = 0; i < 13; i++)
		{
			t  = letter[i];
			letter[i] = letter[25-i];
			letter[25-i] = t; 
		}
	
		pthread_mutex_unlock(&mutex);
	}
	return NULL;
}
void handle_sig(int sig_number)
{
	(void)sig_number;
	pthread_mutex_destroy(&mutex);
	_exit(0);
}

int main()
{
	pthread_t thread1;
	pthread_t thread2;

	pthread_mutex_init(&mutex, NULL);

	pthread_create(&thread1, NULL, invert, NULL);
	pthread_create(&thread2, NULL, reorder, NULL);


	while(1)
	{
		pthread_mutex_lock(&mutex);
		printf("%s\n", letter);
		pthread_mutex_unlock(&mutex);

		sleep(1);

	}

	
	return 0;
}

