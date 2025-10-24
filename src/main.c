#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

typedef struct
{
    int k;
    int experiments_per_thread;
    int *wins1;
    int *wins2;
    int *draws;
    unsigned int seed;
} ThreadArgs;

void *simulate_games(void *arg)
{
    ThreadArgs *args = (ThreadArgs *)arg;
    int wins1 = 0, wins2 = 0, draws = 0;

    for (int i = 0; i < args->experiments_per_thread; i++)
    {
        int sum1 = 0, sum2 = 0;

        for (int j = 0; j < args->k; j++)
        {
            sum1 += rand_r(&args->seed) % 6 + rand_r(&args->seed) % 6 + 2;
            sum2 += rand_r(&args->seed) % 6 + rand_r(&args->seed) % 6 + 2;
        }

        if (sum1 > sum2)
            wins1++;
        else if (sum1 < sum2)
            wins2++;
        else
            draws++;
    }

    *(args->wins1) += wins1;
    *(args->wins2) += wins2;
    *(args->draws) += draws;
    return NULL;
}

int main(int argc, char *argv[])
{
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    int k = 0, total_experiments = 0, max_threads = 1;
    int current_round = 0, score1 = 0, score2 = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-k") == 0)
            k = atoi(argv[++i]);
        else if (strcmp(argv[i], "-r") == 0)
            current_round = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s1") == 0)
            score1 = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s2") == 0)
            score2 = atoi(argv[++i]);
        else if (strcmp(argv[i], "-n") == 0)
            total_experiments = atoi(argv[++i]);
        else if (strcmp(argv[i], "-t") == 0)
            max_threads = atoi(argv[++i]);
    }

    if (k <= 0 || total_experiments <= 0)
    {
        char error_msg[256];
        int len = snprintf(error_msg, sizeof(error_msg),
                           "Usage: %s -k K -r ROUND -s1 SCORE1 -s2 SCORE2 -n EXPERIMENTS [-t THREADS]\n", argv[0]);
        write(2, error_msg, len);
        return 1;
    }

    int wins1 = 0, wins2 = 0, draws = 0;
    pthread_t *threads = malloc(max_threads * sizeof(pthread_t));
    ThreadArgs *thread_args = malloc(max_threads * sizeof(ThreadArgs));
    int experiments_per_thread = total_experiments / max_threads;

    for (int i = 0; i < max_threads; i++)
    {
        thread_args[i].k = k;
        thread_args[i].experiments_per_thread = experiments_per_thread;
        if (i == max_threads - 1)
        {
            thread_args[i].experiments_per_thread += total_experiments % max_threads;
        }
        thread_args[i].wins1 = &wins1;
        thread_args[i].wins2 = &wins2;
        thread_args[i].draws = &draws;
        thread_args[i].seed = time(NULL) ^ getpid() ^ i;
        pthread_create(&threads[i], NULL, simulate_games, &thread_args[i]);
    }

    for (int i = 0; i < max_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(thread_args);

    char buffer[512];
    int len;

    len = snprintf(buffer, sizeof(buffer), "Total experiments: %d\n", total_experiments);
    write(1, buffer, len);

    len = snprintf(buffer, sizeof(buffer), "Player 1 wins: %d %.2f%%\n",
                   wins1, (100.0 * wins1 / total_experiments));
    write(1, buffer, len);

    len = snprintf(buffer, sizeof(buffer), "Player 2 wins: %d %.2f%%\n",
                   wins2, (100.0 * wins2 / total_experiments));
    write(1, buffer, len);

    len = snprintf(buffer, sizeof(buffer), "Draws: %d %.2f%%\n",
                   draws, (100.0 * draws / total_experiments));
    write(1, buffer, len);

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    double execution_time = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    len = snprintf(buffer, sizeof(buffer), "Execution time: %.6f seconds\n", execution_time);
    write(1, buffer, len);

    return 0;
}