#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define THREAD_NOT_JOINED -2
#define THREAD_NOT_CREATED -1
#define AMOUNT_LINES 10
#define SUCCESS 0

void* printLine(void *param) {
	write(STDOUT_FILENO, (char*)param,strlen((char*) param) );
	return NULL;
}


int main(int argc, char *argv[]) {
	pthread_t thread;
	int errorCode = pthread_create(&thread, NULL, printLines, "Child\n");
	if(errorCode != 0){
		printf("Thread not created. Error code: %d", errorCode);
		return THREAD_NOT_CREATED;
	}

	errorCode = pthread_join(thread, NULL);
	if(errorCode != SUCCESS){
		printf("Thread not joined. Error code: %d", errorCode);
		return THREAD_NOT_JOINED;
	}

	printLines("Parent\n");
	return 0;
}
