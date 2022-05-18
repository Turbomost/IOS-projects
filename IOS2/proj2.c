/*
 * @Author: Václav Valenta (xvalen29)
 * @Date: 2022-04-19 10:49:42
 * @Last Modified by: Václav Valenta (xvalen29)
 * @Last Modified time: 2022-05-02 21:42:19
 */

#include "proj2.h"

/**
 * @brief Print error based on argument and exit program
 *
 * @param err input error
 */
void print_error(error_t err) {
    switch (err) {
        case ARGC_ERR:
            fprintf(stderr, "Unexpected number of arguments\n");
            break;
        case ARGV_ERR:
            fprintf(stderr, "Invalid argument\n");
            break;
        case FORK_ERR:
            fprintf(stderr, "Fork err\n");
            break;
        case TITB_ERR:
            fprintf(stderr, "TI and TB values must be in <0, 1000>\n");
            break;
        case SMEM_ERR:
            fprintf(stderr, "Shared memory error\n");
            break;
        case SMPH_ERR:
            fprintf(stderr, "Semaphore memory error\n");
            break;
        case FILE_ERR:
            fprintf(stderr, "Cannot open file\n");
            break;
        case INTERNAL_ERR:
            fprintf(stderr, "Internal error\n");
            break;
        default:
            fprintf(stderr, "Unexpected error (%i)\n", err);
            break;
    }
    quit(EXIT_FAILURE);
}

/**
 * @brief Validate input arguments and set variables
 *
 * @param argc argument count
 * @param argv array of arguments
 * @param NO Number of Oxygens
 * @param NH Number of Hydrogens
 * @param TI Maximum queue insertion time
 * @param TB Maximum creating time
 */
void check_arguments(int argc, char* argv[], int* NO, int* NH, int* TI, int* TB) {
    // Invalid number of arguments
    if (argc != 5) {
        print_error(ARGC_ERR);
    }

    // Parse input arguments
    *NO = set_argument(argv[1]);
    *NH = set_argument(argv[2]);
    *TI = set_argument(argv[3]);
    *TB = set_argument(argv[4]);

    // Check if TI and TB are valid
    if (*TI < 0 || *TI > 1000 || *TB < 0 || *TB > 1000) {
        print_error(TITB_ERR);
    }

    if (*NO == 0 && *NH == 0) {
        print_error(ARGV_ERR);
    }
}

/**
 * @brief Validate input and return integer
 *
 * @param input input value
 * @return int output integer
 */
int set_argument(char input[]) {
    char* endptr;
    long value = strtol(input, &endptr, 10);
    if (endptr == input || value == LONG_MAX || value == LONG_MIN || (*endptr != '\0')) {
        print_error(ARGV_ERR);
    }

    if (value < 0) {
        print_error(ARGV_ERR);
    }

    return value;
}

/**
 * @brief Allocate shared memory
 *
 * @return true if successfull
 * @return false if any allocation fails
 */
bool map_memory() {
    // total count      -> Used as and A: in output
    // oxygen count     -> Used for the oxygen process to know its Oid
    // hydrogen count   -> Used for the hydrogen process to know its Hid
    // oxygen done      -> Check how many oxygens are left
    // hydrogen done    -> Check how many hydrogens are left
    // oxygens queued   -> Wait with writing 'not enough...' until all the oxygens are queued
    // hydrogens queued -> Wait with writing 'not enough...' until all the hydrogens are queued
    // molecules count  -> Used in output as Mid

    if ((xvalen29_oxygen_count = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_oxygen_done = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_hydrogen_count = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_hydrogen_done = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_molecules_count = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_oxygens_queued = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_hydrogens_queued = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_total_count = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED) {
        return false;
    }

    *xvalen29_oxygen_count = 1;
    *xvalen29_oxygen_done = 0;
    *xvalen29_hydrogen_count = 1;
    *xvalen29_hydrogen_done = 0;
    *xvalen29_total_count = 1;
    *xvalen29_oxygens_queued = 1;
    *xvalen29_hydrogens_queued = 1;
    *xvalen29_molecules_count = 3;

    return true;
}

/**
 * @brief Allocate semaphores
 *
 * @return true if successfull
 * @return false if any allocation fails
 */
bool map_semaphores() {
    // write                -> Used for writing one line at the time
    // creating hydrogen    -> Oxygens are sending two signals (to allow two hydrogens to create)
    // creating oxygen      -> One molecule at the time
    // hydrogen             -> Oxygens is wating for two hydrogens
    // molecule             -> Oxygen is confirming that the molecule can be created
    // done                 -> Oxygen is informing that the molecule is done
    // done waiting         -> Proocesses are waiting for the molecule to be done
    // all hydrogens        -> Semaphore that signals wheather all of the hydrogens are in the queue already
    // all oxygens          -> Semaphore that signals wheather all of the oxygens are in the queue already

    if ((xvalen29_creating_hydrogen_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_hydrogen_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_molecule_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_done_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_creating_oxygen_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_done_waiting_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_all_hydrogens_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_all_oxygens_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_hydrogen_waiting_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        (xvalen29_write_semaphore = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0)) == MAP_FAILED ||
        sem_init(xvalen29_creating_hydrogen_semaphore, 1, 2) == -1 ||
        sem_init(xvalen29_hydrogen_semaphore, 1, 0) == -1 ||
        sem_init(xvalen29_molecule_semaphore, 1, 0) == -1 ||
        sem_init(xvalen29_done_semaphore, 1, 0) == -1 ||
        sem_init(xvalen29_creating_oxygen_semaphore, 1, 1) == -1 ||
        sem_init(xvalen29_done_waiting_semaphore, 1, 0) == -1 ||
        sem_init(xvalen29_all_hydrogens_semaphore, 1, 0) == -1 ||
        sem_init(xvalen29_all_oxygens_semaphore, 1, 0) == -1 ||
        sem_init(xvalen29_hydrogen_waiting_semaphore, 1, 0) == -1 ||
        sem_init(xvalen29_write_semaphore, 1, 1) == -1) {
        return false;
    }
    return true;
}

/**
 * @brief Unmap all shared memories
 *
 * @return true if successfull
 * @return false if anything fails
 */
bool umap_memory() {
    if (munmap(xvalen29_oxygen_count, sizeof(int)) == -1 ||
        munmap(xvalen29_oxygen_done, sizeof(int)) == -1 ||
        munmap(xvalen29_hydrogen_count, sizeof(int)) == -1 ||
        munmap(xvalen29_hydrogen_done, sizeof(int)) == -1 ||
        munmap(xvalen29_molecules_count, sizeof(int)) == -1 ||
        munmap(xvalen29_oxygens_queued, sizeof(int)) == -1 ||
        munmap(xvalen29_hydrogens_queued, sizeof(int)) == -1 ||
        munmap(xvalen29_total_count, sizeof(int)) == -1) {
        return false;
    }
    return true;
}

/**
 * @brief Unmap all semaphores
 *
 * @return true if successfull
 * @return false if anything fails
 */
bool unmap_semaphores() {
    if (sem_destroy(xvalen29_creating_hydrogen_semaphore) == -1 ||
        sem_destroy(xvalen29_hydrogen_semaphore) == -1 ||
        sem_destroy(xvalen29_molecule_semaphore) == -1 ||
        sem_destroy(xvalen29_done_semaphore) == -1 ||
        sem_destroy(xvalen29_creating_oxygen_semaphore) == -1 ||
        sem_destroy(xvalen29_done_waiting_semaphore) == -1 ||
        sem_destroy(xvalen29_all_oxygens_semaphore) == -1 ||
        sem_destroy(xvalen29_hydrogen_waiting_semaphore) == -1 ||
        sem_destroy(xvalen29_all_hydrogens_semaphore) == -1 ||
        sem_destroy(xvalen29_write_semaphore) == -1 ||
        munmap(xvalen29_creating_hydrogen_semaphore, sizeof(sem_t)) == -1 ||
        munmap(xvalen29_hydrogen_semaphore, sizeof(sem_t)) == -1 ||
        munmap(xvalen29_molecule_semaphore, sizeof(sem_t)) == -1 ||
        munmap(xvalen29_done_semaphore, sizeof(sem_t)) == -1 ||
        munmap(xvalen29_creating_oxygen_semaphore, sizeof(sem_t)) == -1 ||
        munmap(xvalen29_done_waiting_semaphore, sizeof(sem_t)) == -1 ||
        munmap(xvalen29_all_oxygens_semaphore, sizeof(sem_t)) == -1 ||
        munmap(xvalen29_all_hydrogens_semaphore, sizeof(sem_t)) == -1 ||
        munmap(xvalen29_hydrogen_waiting_semaphore, sizeof(sem_t)) == -1 ||
        munmap(xvalen29_write_semaphore, sizeof(sem_t)) == -1) {
        return false;
    }
    return true;
}

/**
 * @brief Clean all memory, close file and exit program
 *
 * @param exit_code exit code
 */
void quit(int exit_code) {
    if (!umap_memory())
        print_error(SMEM_ERR);
    if (!unmap_semaphores())
        print_error(SMPH_ERR);
    if (output_file)
        fclose(output_file);
    exit(exit_code);
}

/**
 * @brief Waits for random time with maximum of TI ms
 *
 * @param TI max time to wait in ms
 */
void rnd_wait(int TI) {
    srand(time(NULL) & getpid());
    usleep((rand() % (TI + 1)) * 1000);
}

/**
 * @brief Write to file
 *
 * @param element element that performs an operation 'H' or 'O'
 * @param mode mode to be used for writing
 * @param param id of the element
 */
void file_write(char element, write_t mode, int param) {
    sem_wait(xvalen29_write_semaphore);
    switch (mode) {
        case W_START:
            fprintf(output_file, "%i: %c %i: started\n", (*xvalen29_total_count)++, element, param);
            break;
        case W_QUEUE:
            fprintf(output_file, "%i: %c %i: going to queue\n", (*xvalen29_total_count)++, element, param);
            break;
        case W_BUILD:
            fprintf(output_file, "%i: %c %i: creating molecule %i\n", (*xvalen29_total_count)++, element, param, ((*xvalen29_molecules_count)++) / 3);
            break;
        case W_MDONE:
            fprintf(output_file, "%i: %c %i: molecule %i created\n", (*xvalen29_total_count)++, element, param, ((*xvalen29_molecules_count) - 1) / 3);
            break;
        case W_OMISS:
            fprintf(output_file, "%i: %c %i: not enough H\n", (*xvalen29_total_count)++, element, param);
            break;
        case W_HMISS:
            fprintf(output_file, "%i: %c %i: not enough O or H\n", (*xvalen29_total_count)++, element, param);
            break;
        default:
            print_error(INTERNAL_ERR);
            break;
    }
    sem_post(xvalen29_write_semaphore);
}

/**
 * @brief Process for oxygen
 *
 * @param idO id of the oxygen
 * @param TI maximum time for waiting
 * @param TB maximum time for creating molecule
 * @param NH total numbevr of hydrogens
 */
void oxygen_process(int idO, int TI, int TB, int NH, int NO) {
    // Start and join queue
    file_write('O', W_START, idO);
    rnd_wait(TI);
    file_write('O', W_QUEUE, idO);

    // After all oxygens are queued
    if (((*xvalen29_oxygens_queued)++) == NO) {
        sem_post(xvalen29_all_oxygens_semaphore);
    }

    // Creating one molecule at the time
    sem_wait(xvalen29_creating_oxygen_semaphore);

    // Check if the molecule is creatable
    if ((*xvalen29_hydrogen_done) >= NH - 1) {
        // The molecule can't be created - wait for all other processes to enter the queue
        sem_wait(xvalen29_all_oxygens_semaphore);
        sem_wait(xvalen29_all_hydrogens_semaphore);
        file_write('O', W_OMISS, idO);

        sem_post(xvalen29_creating_oxygen_semaphore);
        sem_post(xvalen29_all_oxygens_semaphore);
        sem_post(xvalen29_all_hydrogens_semaphore);
        exit(EXIT_SUCCESS);
    }

    // Waiting for two hydrogens
    sem_wait(xvalen29_hydrogen_semaphore);
    sem_wait(xvalen29_hydrogen_semaphore);

    // Send hydrogens signal that they can create molecule
    sem_post(xvalen29_molecule_semaphore);
    sem_post(xvalen29_molecule_semaphore);

    file_write('O', W_BUILD, idO);

    rnd_wait(TB);

    // Send done signal and increment counters
    (*xvalen29_oxygen_done)++;
    (*xvalen29_hydrogen_done) += 2;
    sem_wait(xvalen29_hydrogen_waiting_semaphore);
    sem_wait(xvalen29_hydrogen_waiting_semaphore);
    file_write('O', W_MDONE, idO);
    sem_post(xvalen29_done_semaphore);
    sem_post(xvalen29_done_semaphore);

    sem_wait(xvalen29_done_waiting_semaphore);
    sem_wait(xvalen29_done_waiting_semaphore);
    sem_post(xvalen29_creating_hydrogen_semaphore);
    sem_post(xvalen29_creating_hydrogen_semaphore);

    // We can create another molecule
    sem_post(xvalen29_creating_oxygen_semaphore);

    exit(EXIT_SUCCESS);
}

/**
 * @brief Process for creating hydrogen
 *
 * @param idN id of the hydrogen
 * @param TI maximum time for waiting
 * @param TB maximum time for creating molecule
 * @param NO total number of oxygens
 */
void hydrogen_process(int idN, int TI, int NH, int NO) {
    // Start and join queue
    file_write('H', W_START, idN);
    rnd_wait(TI);
    file_write('H', W_QUEUE, idN);

    // After all hydrogens are queued
    if (((*xvalen29_hydrogens_queued)++) >= NH) {
        sem_post(xvalen29_all_hydrogens_semaphore);
    }

    // Only Two hydrogens at the time can come
    sem_wait(xvalen29_creating_hydrogen_semaphore);
    sem_post(xvalen29_hydrogen_semaphore);

    // Check if the molecule is creatable
    if (((*xvalen29_oxygen_done) >= NO) || (*xvalen29_hydrogen_done) == NH - 1) {
        // The molecule can't be created - wait for all other processes to enter the queue
        sem_wait(xvalen29_all_oxygens_semaphore);
        sem_wait(xvalen29_all_hydrogens_semaphore);
        file_write('H', W_HMISS, idN);
        sem_post(xvalen29_all_hydrogens_semaphore);
        sem_post(xvalen29_all_hydrogens_semaphore);
        sem_post(xvalen29_all_oxygens_semaphore);
        sem_post(xvalen29_creating_hydrogen_semaphore);
        exit(EXIT_SUCCESS);
    }

    // Wait for confirmation from oxygen
    sem_wait(xvalen29_molecule_semaphore);
    file_write('H', W_BUILD, idN);
    sem_post(xvalen29_hydrogen_waiting_semaphore);

    // Wait until the molecule is done
    sem_wait(xvalen29_done_semaphore);
    file_write('H', W_MDONE, idN);
    sem_post(xvalen29_done_waiting_semaphore);

    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
    // Variables
    int NO, NH, TI, TB;
    pid_t id;

    // Parse command line arguemnts
    check_arguments(argc, argv, &NO, &NH, &TI, &TB);

    // Open file
    if (!(output_file = fopen("proj2.out", "w")))
        print_error(FILE_ERR);

    // Set up v-buffer for correct output
    setvbuf(output_file, NULL, _IOLBF, 0);

    // Map shared memory
    if (!map_memory())
        print_error(SMEM_ERR);

    // Map semaphores
    if (!map_semaphores())
        print_error(SMPH_ERR);

    // If number of oxygens == 0
    if (NO == 0)
        sem_post(xvalen29_all_oxygens_semaphore);

    // If number of hydrogens == 0
    if (NH == 0)
        sem_post(xvalen29_all_hydrogens_semaphore);

    // Create all processes
    for (int index = 0; index < NO + NH; index++) {
        switch (id = fork()) {
            case -1:
                print_error(FORK_ERR);
                break;
            case 0:
                if (index < NO)
                    oxygen_process((*xvalen29_oxygen_count)++, TI, TB, NH, NO);
                else
                    hydrogen_process((*xvalen29_hydrogen_count)++, TI, NH, NO);
                break;
            default:
                break;
        }
    }

    // Main process is waiting for all child processes to finish
    while (wait(NULL) > 0)
        ;
    quit(EXIT_SUCCESS);
    return 0;
}