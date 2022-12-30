#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define THREAD_NOT_CREATED -1
#define THREAD_NOT_JOINED -2
#define AMOUNT_LINES 1
#define CYCLE_COUNT 4


void* print_child(void *param) {
	for(int i = 0; i < AMOUNT_LINES; i++){
	     int number = 1;// **((int **)param);
	     char numText[2] = "";
	     numText[0] = number + '0';
	     char* outputText = strcat("Child",numText);
	     write(STDOUT_FILENO, outputText, strlen((char*) outputText) );
	}
	return NULL;
}


int main(int argc, char *argv[]) {
	for(int i = 0; i < CYCLE_COUNT; i++){
		pthread_t thread;
		int errorCode = pthread_create(&thread, NULL, print_child, &i);
		if(errorCode != 0){
			printf("Thread %d not created. Error code: %d", i, errorCode);
			return THREAD_NOT_CREATED;
		}

		errorCode = pthread_join(thread, NULL);
		if(errorCode != 0){
			printf("Thread not joined. Error code : %d", errorCode);
			return THREAD_NOT_JOINED;
		}
	}

	pthread_exit(NULL);
	return 0;
}
