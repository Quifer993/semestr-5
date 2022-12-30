#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define THREAD_NOT_CREATED -1
#define AMOUNT_LINES 10


void* print_lines(void *param) {
	for(int i = 0; i < AMOUNT_LINES; i++){
	     write(STDOUT_FILENO, (char*)param, strlen((char*) param) );
	}
	return NULL;
}


int main(int argc, char *argv[]) {
	pthread_t thread;
	int errorCode = pthread_create(&thread, NULL, print_lines, "Child\n");
	if(errorCode != 0){
		printf("Thread not created. Error code: %d", errorCode);
		return THREAD_NOT_CREATED;
	}
	print_lines("Parent\n");
	pthread_exit(NULL);
	return 0;
}
