/*
 * @Author: Václav Valenta (xvalen29) 
 * @Date: 2022-04-19 10:49:42
 * @Last Modified by: Václav Valenta (xvalen29)
 * @Last Modified time: 2022-05-02 21:42:51
 */

#include <limits.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

int *xvalen29_oxygen_count;
int *xvalen29_oxygen_done;
int *xvalen29_hydrogen_count;
int *xvalen29_hydrogen_done;
int *xvalen29_molecules_count;
int *xvalen29_total_count;
int *xvalen29_oxygens_queued;
int *xvalen29_hydrogens_queued;

sem_t *xvalen29_creating_hydrogen_semaphore = NULL;
sem_t *xvalen29_creating_oxygen_semaphore = NULL;
sem_t *xvalen29_hydrogen_semaphore = NULL;
sem_t *xvalen29_molecule_semaphore = NULL;
sem_t *xvalen29_write_semaphore = NULL;
sem_t *xvalen29_done_semaphore = NULL;
sem_t *xvalen29_done_waiting_semaphore = NULL;
sem_t *xvalen29_all_oxygens_semaphore = NULL;
sem_t *xvalen29_all_hydrogens_semaphore = NULL;
sem_t *xvalen29_hydrogen_waiting_semaphore = NULL;

FILE *output_file;

typedef enum {
    ARGC_ERR,
    ARGV_ERR,
    TITB_ERR,
    FORK_ERR,
    SMEM_ERR,
    FILE_ERR,
    SMPH_ERR,
    INTERNAL_ERR
} error_t;

typedef enum {
    W_START,
    W_QUEUE,
    W_BUILD,
    W_MDONE,
    W_HMISS,
    W_OMISS
} write_t;

void print_error(error_t);
void check_arguments(int, char **, int *, int *, int *, int *);
int set_argument(char *);

bool map_memory();
bool umap_memory();
bool map_semaphores();
bool unmap_semaphores();

void rnd_wait(int);
void quit(int);
void file_write(char, write_t, int);

void oxygen_process(int, int, int, int, int);
void hydrogen_process(int, int, int, int);
void build_molecule();