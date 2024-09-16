/*
Name: Cassidy Nguyen
PSID: 2042567
Date: 04/29/2024
Course Number: COSC 3360
Professor: Paris
Title: Third Assignment
Notes: To test my code with the test cases, I entered into the command line the following: 
g++ -o main Cassidy_Nguyen3.cpp -lpthread && ./main 3 < <INPUT.txt> where INPUT.txt is the input file
In creating my program, I did refer a lot to the Assignment Description pdf as well as the Explanations powerpoint from Professor
Paris. So, a lot of the names I used may be similar or the same.
Additionally, I set the max number of threads/patrons to 100.
*/

#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <unistd.h> // for sleep function

using namespace std;

#define MAXTHREADS 100

// structure of a patron
struct Patron {
    string name; // patron name (will never contain spaces)
    int arrivalDelay; // number of seconds elapsed since the arrival of the previous patron
    int serviceTime; // number of seconds the patron will take to get processed by the clerk
};

// global variables
static int nFreeClerks, nPatrons, nWaited = 0, nNoWait = 0;
static pthread_mutex_t alone;
static pthread_cond_t freeClerks = PTHREAD_COND_INITIALIZER;

// child function AKA thread function
void *patron(void *arg) {
    Patron* pData = static_cast<Patron*>(arg); // original Paris code: lData = (struct pData) arg;

    // check if a clerk is available, if not then patron needs to wait
    cout << pData->name << " arrives at post office." << endl; // arrivalDelay occurs in the main function, so print message
    pthread_mutex_lock(&alone); // enter a critical section
    if (nFreeClerks == 0) {
        nWaited++;
        while (nFreeClerks == 0) {
            pthread_cond_wait(&freeClerks, &alone);
        }
    }
    else {
        nNoWait++;
    }
    nFreeClerks--;
    cout << pData->name << " gets service." << endl;
    pthread_mutex_unlock(&alone); // leave a critical section

    // sleep for serviceTime seconds
    sleep(pData->serviceTime);

    // signal a clerk has become available
    pthread_mutex_lock(&alone); // enter a critical section
    cout << pData->name << " leaves the post office." << endl;
    nFreeClerks++;
    pthread_cond_signal(&freeClerks);
    pthread_mutex_unlock(&alone); // leave a critical section

    // terminate thread
    delete pData;
    pthread_exit((void*) 0);
}

// summary report function
void summary() {
    cout << endl << "SUMMARY REPORT" << endl;
    cout << nPatrons << " patron(s) went to the post office." << endl;
    cout << nWaited << " patron(s) had to wait before getting service." << endl;
    cout << nNoWait << " patron(s) did not have to to wait." << endl;
}

int main(int argc, char* argv[]) {
    // check if the number of clerks is provided
    if (argc < 2) {
        cout << "ERROR: please provide the number of clerks" << endl;
        return 1;
    }

    // get the number of clerks from argument vector
    nFreeClerks = atoi(argv[1]); // initial value is the total number of clerks
    cout << "-- The post office has today " << nFreeClerks << " clerk(s) on duty." << endl;

    pthread_t tid[MAXTHREADS];
    pthread_mutex_init(&alone, NULL);

    // local variables to hold what we read in from standard i/o
    string inputName;
    int inputArrivalDelay, inputServiceTime;

    while (cin >> inputName >> inputArrivalDelay >> inputServiceTime) { // read patron's name, arrivalDelay, and serviceTime
        // sleep for arrivalDelay seconds
        sleep(inputArrivalDelay);

        // create a child thread
        Patron* aPatron = new Patron;
        aPatron->name = inputName;
        aPatron->arrivalDelay = inputArrivalDelay;
        aPatron->serviceTime = inputServiceTime;
        pthread_create(&tid[nPatrons], NULL, patron, aPatron);
        nPatrons++;
    }
    
    // terminate all threads
    for (int i = 0; i < nPatrons; i++) {
        pthread_join(tid[i], NULL);
    }

    // display summary report
    summary();

    pthread_mutex_destroy(&alone);
    pthread_cond_destroy(&freeClerks);

    return 0;
}