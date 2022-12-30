#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SEMAPHORE1 0
#define SEMAPHORE2 1
#define SUCCESS 0
#define NUMBER_OF_LINES 10
#define BUFFER_SIZE 256

int main(int argc, char** argv) {
    pid_t pid = 0;

    sem_t* sem1 = sem_open("sem1", O_CREAT | O_EXCL, S_IREAD | S_IWRITE, 0);
    sem_t* sem2 = sem_open("sem2", O_CREAT | O_EXCL, S_IREAD | S_IWRITE, 1);
    sem_unlink("sem1");
    sem_unlink("sem2");

    pid = fork();
    switch (pid)
    {
    case -1:
        printf("Error fork");
        return 2;
    case 0:
        for (int i = 0; i < NUMBER_OF_LINES; ++i) {
            sem_wait(sem1);
            printf("Child thread - %d\n", i);
            sem_post(sem2);
        }
        break;
    default:
        for (int i = 0; i < NUMBER_OF_LINES; ++i) {
            sem_wait(sem2);
            printf("Parent - %d\n", i);
            sem_post(sem1);
        }
        sem_close(sem1);
        sem_close(sem2);
        break;
    }
}