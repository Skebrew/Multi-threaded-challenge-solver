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
int n0thread_read_locks = 0;
int n0thread_write_locks = 0;
int n1thread_read_locks = 0;
int n1thread_write_locks = 0;
int n2thread_read_locks = 0;
int n2thread_write_locks = 0;
int n3thread_read_locks = 0;
int n3thread_write_locks = 0;
int n4thread_read_locks = 0;
int n4thread_write_locks = 0;
// thread_num_of_locks[0] = thread 0 read locks
// thread_num_of_locks[0 + 1] = thread 0 write locks
char* n0test_table[5] = {0, 0, 0, 0, 0};
char* n1test_table[5] = {0, 0, 0, 0, 0};
char* n2test_table[5] = {0, 0, 0, 0, 0};
char* n3test_table[5] = {0, 0, 0, 0, 0};
//test_table[0][0] = "Thread#";
//test_table[0][1] = "read locks";
//test_table[0][2] = "write locks";
//test_table[0][3] = "currently"; //either waiting for read lock or write lock or currently reading or writing
//test_table[0][4] = "writing"; //is it currently writing

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

    //test_table[0][0] = "Thread#";
    //test_table[0][1] = "read locks";
    //test_table[0][2] = "write locks";
    //test_table[0][3] = "currently"; //either waiting for read lock or write lock or currently reading or writing
    //test_table[0][4] = "writing"; //is it currently writing

    char* lock_error_code = 0; //[LOCK] this gets any error codes the lock throws
    int thread_number = 0;

    tinput_t* tinput = (tinput_t*) tinput_void;
    unsigned long first_tried_solution = 0; //first_tried_solution needs to be set to something different per thread
    if (tinput[0].tid == 0) {
        first_tried_solution = 0;
        thread_number = 0;
    } else if (tinput[0].tid == 1) {
        first_tried_solution = 100000;
        thread_number = 1;
    } else if (tinput[0].tid == 2) {
        first_tried_solution = 200000;
        thread_number = 2;
    } else if (tinput[0].tid == 3) {
        first_tried_solution = 300000;
        thread_number = 3;
    } else {
        first_tried_solution = (tinput[0].tid) * 100000;
    }
    printf(" ");

    //1000*1000000000L*1000 is just very big number, which we will never reach
    for(unsigned long attempted_solution=first_tried_solution; attempted_solution<1000*1000000000L*1000; attempted_solution++){ // incrementally tries to find a solution
        //condition1: sha256(attempted_solution) == challenge

        if (thread_number == 0) {
            n0test_table[3] = "waiting to read";
        } else if (thread_number == 1) {    // LOCK
            n1test_table[3] = "waiting to read";
        } else if (thread_number == 2) {
            n2test_table[3] = "waiting to read";
        } else if (thread_number == 3) {
            n3test_table[3] = "waiting to read";
        }   //waiting to read
        lock_error_code = pthread_rwlock_rdlock(&variable_occupied);    //[LOCK] read lock
        if (thread_number == 0) {
            n0test_table[3] = "reading";
            n0thread_read_locks++;
            n0test_table[1] = (char*)n0thread_read_locks;
        } else if (thread_number == 1) {
            n1test_table[3] = "reading";
            n1thread_read_locks++;
            n1test_table[1] = (char*)n1thread_read_locks;
        } else if (thread_number == 2) {
            n2test_table[3] = "reading";
            n2thread_read_locks++;
            n2test_table[1] = (char*)n2thread_read_locks;
        } else if (thread_number == 3) {
            n3test_table[3] = "reading";
            n3thread_read_locks++;
            n3test_table[1] = (char*)n3thread_read_locks;
        }   //reading
        if(found_solutions==NSOLUTIONS){    //[BUG][shared variable] needs a lock
            if (thread_number == 0) {  //stopped reading
                n0test_table[3] = "nothing";
                n0thread_read_locks--;
                n0test_table[1] = (char*)n0thread_read_locks;
            } else if (thread_number == 1) {
                n1test_table[3] = "nothing";
                n1thread_read_locks--;
                n1test_table[1] = (char*)n1thread_read_locks;
            } else if (thread_number == 2) {
                n2test_table[3] = "nothing";
                n2thread_read_locks--;
                n2test_table[1] = (char*)n2thread_read_locks;
            }  else if (thread_number == 3) {
                n3test_table[3] = "nothing";
                n3thread_read_locks--;
                n3test_table[1] = (char*)n3thread_read_locks;
            }   //stopped reading
            lock_error_code = pthread_rwlock_unlock(&variable_occupied); //[LOCK] read/write unlock
            return lock_error_code;
        }
        if (thread_number == 0) {  //stopped reading
            n0test_table[3] = "nothing";
            n0thread_read_locks--;
            n0test_table[1] = (char*)n0thread_read_locks;
        } else if (thread_number == 1) {
            n1test_table[3] = "nothing";
            n1thread_read_locks--;
            n1test_table[1] = (char*)n1thread_read_locks;
        } else if (thread_number == 2) {
            n2test_table[3] = "nothing";
            n2thread_read_locks--;
            n2test_table[1] = (char*)n2thread_read_locks;
        }  else if (thread_number == 3) {
            n3test_table[3] = "nothing";
            n3thread_read_locks--;
            n3test_table[1] = (char*)n3thread_read_locks;
        }   //stopped reading
        lock_error_code = pthread_rwlock_unlock(&variable_occupied); //[LOCK] read/write unlock


        //lock_error_code = pthread_rwlock_rdlock(&variable_occupied);  //[LOCK] read lock [TESTCODE] write lock
        if(try_solution(tinput->challenge, attempted_solution)){

            //condition2: the last digit must be different in all the solutions
            short bad_solution = 0;

            if (thread_number == 0) {
                n0test_table[3] = "waiting to read";
            } else if (thread_number == 1) {    // LOCK
                n1test_table[3] = "waiting to read";
            } else if (thread_number == 2) {
                n2test_table[3] = "waiting to read";
            } else if (thread_number == 3) {
                n3test_table[3] = "waiting to read";
            }   //waiting to read
            lock_error_code = pthread_rwlock_rdlock(&variable_occupied);    //[LOCK] read lock
            if (thread_number == 0) {
                n0test_table[3] = "reading";
                n0thread_read_locks++;
                n0test_table[1] = (char*)n0thread_read_locks;
            } else if (thread_number == 1) {
                n1test_table[3] = "reading";
                n1thread_read_locks++;
                n1test_table[1] = (char*)n1thread_read_locks;
            } else if (thread_number == 2) {
                n2test_table[3] = "reading";
                n2thread_read_locks++;
                n2test_table[1] = (char*)n2thread_read_locks;
            } else if (thread_number == 3) {
                n3test_table[3] = "reading";
                n3thread_read_locks++;
                n3test_table[1] = (char*)n3thread_read_locks;
            }   //reading
            for(int i=0;i<found_solutions;i++){ //[Shared Variable]
                if(attempted_solution%10 == solutions[i]%10){   // [shared variable] needs a lock
                    bad_solution = 1;
                }
            }
            if (thread_number == 0) {  //stopped reading
                n0test_table[3] = "nothing";
                n0thread_read_locks--;
                n0test_table[1] = (char*)n0thread_read_locks;
            } else if (thread_number == 1) {
                n1test_table[3] = "nothing";
                n1thread_read_locks--;
                n1test_table[1] = (char*)n1thread_read_locks;
            } else if (thread_number == 2) {
                n2test_table[3] = "nothing";
                n2thread_read_locks--;
                n2test_table[1] = (char*)n2thread_read_locks;
            }  else if (thread_number == 3) {
                n3test_table[3] = "nothing";
                n3thread_read_locks--;
                n3test_table[1] = (char*)n3thread_read_locks;
            }   //stopped reading
            lock_error_code = pthread_rwlock_unlock(&variable_occupied); //[LOCK] read unlock


            if(bad_solution){
                continue;
            }


            if (thread_number == 0) {
                n0test_table[3] = "waiting to read";
            } else if (thread_number == 1) {    // LOCK
                n1test_table[3] = "waiting to read";
            } else if (thread_number == 2) {
                n2test_table[3] = "waiting to read";
            } else if (thread_number == 3) {
                n3test_table[3] = "waiting to read";
            }   //waiting to read
            lock_error_code = pthread_rwlock_rdlock(&variable_occupied); //[LOCK] write lock
            if (thread_number == 0) {
                n0test_table[3] = "reading";
                n0thread_read_locks++;
                n0test_table[1] = (char*)n0thread_read_locks;
            } else if (thread_number == 1) {
                n1test_table[3] = "reading";
                n1thread_read_locks++;
                n1test_table[1] = (char*)n1thread_read_locks;
            } else if (thread_number == 2) {
                n2test_table[3] = "reading";
                n2thread_read_locks++;
                n2test_table[1] = (char*)n2thread_read_locks;
            } else if (thread_number == 3) {
                n3test_table[3] = "reading";
                n3thread_read_locks++;
                n3test_table[1] = (char*)n3thread_read_locks;
            }   //reading
            //condition3: no solution should be divisible by any number in the range [1000000, 1500000]
            if(!divisibility_check(attempted_solution)){
                if (thread_number == 0) {  //stopped reading
                    n0test_table[3] = "nothing";
                    n0thread_read_locks--;
                    n0test_table[1] = (char*)n0thread_read_locks;
                } else if (thread_number == 1) {
                    n1test_table[3] = "nothing";
                    n1thread_read_locks--;
                    n1test_table[1] = (char*)n1thread_read_locks;
                } else if (thread_number == 2) {
                    n2test_table[3] = "nothing";
                    n2thread_read_locks--;
                    n2test_table[1] = (char*)n2thread_read_locks;
                }  else if (thread_number == 3) {
                    n3test_table[3] = "nothing";
                    n3thread_read_locks--;
                    n3test_table[1] = (char*)n3thread_read_locks;
                }   //stopped reading
                lock_error_code = pthread_rwlock_unlock(&variable_occupied); //[LOCK] read unlock
                continue;
            }
            if (thread_number == 0) {  //stopped reading
                n0test_table[3] = "nothing";
                n0thread_read_locks--;
                n0test_table[1] = (char*)n0thread_read_locks;
            } else if (thread_number == 1) {
                n1test_table[3] = "nothing";
                n1thread_read_locks--;
                n1test_table[1] = (char*)n1thread_read_locks;
            } else if (thread_number == 2) {
                n2test_table[3] = "nothing";
                n2thread_read_locks--;
                n2test_table[1] = (char*)n2thread_read_locks;
            }  else if (thread_number == 3) {
                n3test_table[3] = "nothing";
                n3thread_read_locks--;
                n3test_table[1] = (char*)n3thread_read_locks;
            }   //stopped reading
            lock_error_code = pthread_rwlock_unlock(&variable_occupied); //[LOCK] read unlock

            if (thread_number == 0) {
                n0test_table[3] = "waiting to write";
            } else if (thread_number == 1) {
                n1test_table[3] = "waiting to write";
            } else if (thread_number == 2) {
                n2test_table[3] = "waiting to write";
            } else if (thread_number == 3) {
                n3test_table[3] = "waiting to write";
            }   //waiting to write
            lock_error_code = pthread_rwlock_wrlock(&variable_occupied); //[LOCK] write lock
            if (thread_number == 0) {
                n0thread_write_locks++;
                n0test_table[1] = (char*)n0thread_write_locks;
                n0test_table[3] = "writing";
                n0test_table[4] = "YES";
            } else if (thread_number == 1) {
                n1thread_write_locks++;
                n1test_table[1] = (char*)n1thread_write_locks;
                n1test_table[3] = "writing";
                n1test_table[4] = "YES";
            } else if (thread_number == 2) {
                n2thread_write_locks++;
                n2test_table[1] = (char*)n2thread_write_locks;
                n2test_table[3] = "writing";
                n2test_table[4] = "YES";
            } else if (thread_number == 3) {
                n3thread_write_locks++;
                n3test_table[1] = (char*)n3thread_write_locks;
                n3test_table[3] = "writing";
                n3test_table[4] = "YES";
            }   //currently writing

            solutions[found_solutions] = attempted_solution;    //[BUG][shared variable] needs a lock
            found_solutions++;  //[BUG][shared variable] needs a lock

            if (thread_number == 0) {
                n0thread_write_locks--;
                n0test_table[1] = (char*)n0thread_write_locks;
                n0test_table[3] = "nothing";
                n0test_table[4] = "";
            } else if (thread_number == 1) {
                n1thread_write_locks--;
                n1test_table[1] = (char*)n1thread_write_locks;
                n1test_table[3] = "nothing";
                n1test_table[4] = "";
            } else if (thread_number == 2) {
                n2thread_write_locks--;
                n2test_table[1] = (char*)n2thread_write_locks;
                n2test_table[3] = "nothing";
                n2test_table[4] = "";
            } else if (thread_number == 3) {
                n3thread_write_locks--;
                n3test_table[1] = (char*)n3thread_write_locks;
                n3test_table[3] = "nothing";
                n3test_table[4] = "";
            }   //stopped writing
            lock_error_code = pthread_rwlock_unlock(&variable_occupied); //[LOCK] read/write unlock
        }
    }
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
        printf(" ");
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

