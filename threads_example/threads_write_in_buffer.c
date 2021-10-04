#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

const int NUM_THREADS = 4, SIZE = 100, timeskip = 5;

pthread_mutex_t mutex;
int array[400];
int in = 0;


void *Write(void *threadid)
{
    int tid = (long)threadid + 1;
    for(int i=0; i<SIZE; ++i)
    {
        pthread_mutex_lock(&mutex);
        array[in] = tid;
        in++;
        pthread_mutex_unlock(&mutex);
        usleep(timeskip);
    }
    pthread_exit(NULL);
}


void *Read(void *threadid)
{
    int tid = (long)threadid + 1;
    char text[] = "fileX";
    text[4] = tid + '0';
    FILE* desc;
    if ((desc = fopen(text, "w")) == NULL)
    {
	printf("ERROR: failed to open file for writing");
	getchar();
	return 0;
    }
    for (int i = 0; i < SIZE; i++)
    {
	pthread_mutex_lock(&mutex);
	if (array[in] == tid)
	{
	    printf("%d) me is %d writing %d\n", in+1, tid, array[in]);
            fprintf(desc, "%d ", array[in]);
            ++in;
	}
	else
	    i--;
	pthread_mutex_unlock(&mutex);
        usleep(timeskip);
    }
    fclose(desc);
    pthread_exit(NULL);
}


int main (int argc, char *argv[])
{
    int wc, wj, rc, rj;
    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&mutex, NULL);

    for (int t=0; t<NUM_THREADS; ++t)
    {
        wc = pthread_create(&threads[t], NULL, Write, (void *)t);
        if (wc){
            printf("ERROR; return code from pthread_create() is %d\n", wc);
            exit(-1);
       }
    }
    for (int t = 0; t < NUM_THREADS; ++t)
    {
	wj = pthread_join(threads[t], NULL);
	if (wj){
            printf("ERROR; return code from pthread_join() is %d\n", rj);
            exit(-1);
       }
    }
    in = 0;
    for (int t = 0; t < NUM_THREADS; ++t)
    {
	rc = pthread_create(&threads[t], NULL, Read, (void *)t);
	if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
       }
    }
    for (int t = 0; t < NUM_THREADS; ++t)
    {
	rj = pthread_join(threads[t], NULL);
	if (rj){
            printf("ERROR; return code from pthread_join() is %d\n", rj);
            exit(-1);
       }
    }	
    pthread_mutex_destroy(&mutex);
    return 0;
}
