#include <string.h>
#include <openssl/sha.h>
#include <stdio.h> 
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

// NEED TO IMPLEMENT: I need the next thread's initial attempt to be where the next biggest value is. not just +10000
// Or I need to just put finer lock control. Damn I want to do that but I should spend some time with my family lol. It's thanksgiving after all


typedef struct{
    unsigned short tid;
    unsigned short challenge;
} tinput_t;


unsigned int NSOLUTIONS = 8;    // max number of solutions, accessed by multiple threads but is never written to so it doesn't need a lock

//the following two global variables are used by worker threads to communicate back the found results to the main thread
unsigned short found_solutions = 0;
unsigned long* solutions;

//[IMPLEMENT][TEST][LOCK] we're going to try to get a read/write lock working here
pthread_rwlock_t variable_occupied;

//[TESTCODE] variables for tracking locks
int locks = 0;
int unlocks = 0;

unsigned short divisibility_check(unsigned long n){
    //very not efficient algorithm
    unsigned long i;
    for(i=1000000;i<=1500000;i++){
        if(n%i == 0){
            return 0;
        }
    }
    return 1;
}

short try_solution(unsigned short challenge, unsigned long attempted_solution){
        //check if sha256(attempted_solution) is equal to digest
        //attempted_solution is converted to an 8byte buffer preserving endianness
        //the first 2 bytes of the hash are considered as a little endian 16bit number and compared to challenge
        unsigned char digest[SHA256_DIGEST_LENGTH];
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, &attempted_solution, 8);
        SHA256_Final(digest, &ctx);

        if((*(unsigned short*)digest) == challenge){
            return 1;
        }else{
            return 0;
        }
}

void* worker_thread_function(void *tinput_void){
    char* lock_error_code = 0; //[LOCK] this just is needed because the rwlock functions require a integer

    tinput_t* tinput = (tinput_t*) tinput_void; // gets the tinput
    unsigned long first_tried_solution = (tinput[0].tid) * 10000; //[TEST] first_tried_solution needs to be set to 10,000(or whatever interval) more than the last solution

    //[LOCK] test if locks work
    lock_error_code = pthread_rwlock_wrlock(&variable_occupied);  //[LOCK] read lock [TESTCODE] write lock

    //1000*1000000000L*1000 is just very big number, which we will never reach
    for(unsigned long attempted_solution=first_tried_solution; attempted_solution<1000*1000000000L*1000; attempted_solution++){ // incrementally tries to find a solution
        //condition1: sha256(attempted_solution) == challenge
        if(found_solutions==NSOLUTIONS){    //[BUG][shared variable] needs a lock
            lock_error_code = pthread_rwlock_unlock(&variable_occupied); //[LOCK] write unlock
            return NULL;
        }
        if(try_solution(tinput->challenge, attempted_solution)){
            lock_error_code = pthread_rwlock_wrlock(&variable_occupied);  //[LOCK] read lock [TESTCODE] write lock
            //condition2: the last digit must be different in all the solutions
            short bad_solution = 0;
            //locks++;    //[TESTCODE]
            //printf("Locked %i\n" + tinput[0].tid);
            for(int i=0;i<found_solutions;i++){ //[Shared Variable][TEST]
                if(attempted_solution%10 == solutions[i]%10){   // [shared variable] needs a lock
                    bad_solution = 1;
                }
            }
            //unlocks++;  //[TESTCODE]
            //printf("Unlocked %i\n" + tinput[0].tid);    //[TESTCODE]
            //lock_error_code = pthread_rwlock_unlock(&variable_occupied);  //[LOCK] Unlock read [TESTCODE] write unlock
            if (lock_error_code != 0) {   //[TESTCODE]
                lock_error_code = lock_error_code;
            }

            if(bad_solution){
                continue;
            }

            //condition3: no solution should be divisible by any number in the range [1000000, 1500000]
            if(!divisibility_check(attempted_solution)){
                continue;
            }

            //lock_error_code = pthread_rwlock_wrlock(&variable_occupied); //[LOCK] write lock
            if (lock_error_code != 0) {   //[TESTCODE]
                lock_error_code = lock_error_code;
            }
            solutions[found_solutions] = attempted_solution;    //[BUG][shared variable] needs a lock
            found_solutions++;  //[BUG][shared variable] needs a lock
            if(found_solutions==NSOLUTIONS){    //[BUG][shared variable] needs a lock
                //lock_error_code = pthread_rwlock_unlock(&variable_occupied); //[LOCK] write unlock
                lock_error_code = pthread_rwlock_unlock(&variable_occupied);
                return lock_error_code;
            }
        }
    }
    lock_error_code = pthread_rwlock_unlock(&variable_occupied);
    return lock_error_code;
}

// Create multiple threads to find the eight different solution
void solve_one_challenge(unsigned short challenge, unsigned short nthread){

    pthread_t th[nthread];      //creates an array to hold threads
    tinput_t inputs[nthread];   //An array that holds the struct containing the threads id, tid, and the thread's challenge
    int lock_error_code;    //[TESTCODE] collects the locks error number

    found_solutions = 0;    //[IMPLEMENT][TEST][BUG][shared variable] needs a lock. our thread flag variable
    solutions = (unsigned long*) malloc(NSOLUTIONS * (sizeof(unsigned long)));  //[POTENTIAL BUG][shared variable] shouldn't need a lock yet since no threads have ran
    for(int i=0; i<NSOLUTIONS; i++){    // initializes solutions to 0
        solutions[i] = 0;   //[POTENTIAL BUG][shared variable] shouldn't need a lock yet since no threads have ran
    }

    for(int i=0; i<nthread; i++){   // sets the tid and challenge for each thread and then creates the threads to work on them
        inputs[i].tid = i;
        inputs[i].challenge = challenge;    // the challenges are all going to be the same since its the same problem
        lock_error_code = pthread_create(&(th[i]), NULL, worker_thread_function, &(inputs[i]));
    }

    for(int i=0; i<nthread; i++){   // Wait until all threads finish running
        pthread_join(th[i], NULL);
    }

    printf("%d ", challenge);   // Print the solutions
    for(int i=0; i<NSOLUTIONS; i++){
        printf("%ld ", solutions[i]);   //[shared variable] shouldn't need a lock since we already joined all the threads.
    }
    printf("\n");
    free(solutions);    //[shared variable] shouldn't need a lock since we already joined all the threads.
}


int main(int argc, char* argv[]) {
    //argv[1] is the number of worker threads we must use
    //the other arguments are the challenges we must solve
    unsigned short nthread = strtol(argv[1],NULL,10);   //[POTENTIAL BUG] Does this create all the threads given by the command line?
    pthread_rwlock_init(&variable_occupied,NULL); //[LOCK][TEST] initializes the lock

    for(int i = 2; i<argc; i++){    // starting at the first challange thread until the last challenge thread   [BUG] Does this create all the threads given by the command line?
        unsigned short challenge = strtol(argv[i],NULL,10); //Gets one challenge
        solve_one_challenge(challenge, nthread);    // solves one challenge with the index of the first thread
    }

    return 0;
}


/*
compile using:
gcc -ggdb -O0 -o task task.c -lpthread -lcrypto

It may require installing libssl-dev:
sudo apt-get install libssl-dev
*/

