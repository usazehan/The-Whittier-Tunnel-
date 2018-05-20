#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <signal.h>
#include <cstdio>
using namespace std;
// 7 shared variable that will be used in most thread
static int maxcars; // maximum # of cars in the tunnel
static int ncars; // current # of cars in the tunnel
static int delayed; // # of delayed cars
static int whittierBound; // # of cars that arrived in Whittier
static int bearValleyBound; // # of cars that arrived in Bear Valley
static int done = 0;
char traffic; // update the direction value

static pthread_mutex_t traffic_lock = PTHREAD_MUTEX_INITIALIZER; // initialized pthread lock
pthread_cond_t clear = PTHREAD_COND_INITIALIZER; // condition variable
//struct for the stored the car info
struct carInfo{
    carInfo(int arrival, int cross, string direct) : arrivalTime(arrival), crossTime(cross), direction(direct) {} // constructor
    int arrivalTime = 0;
    int crossTime = 0;
    string direction;
};
vector <carInfo> cars;
//tunnel thread will change change the status of the tunnel every 5s
void *tunnel(void *arg){ // it goes in a sequence of Whittier, no traffic, Bear valley, no traffic
    while(!done) {
        if (done) {
            pthread_exit((void*) 0); //exit the thread
        }
        pthread_mutex_lock(&traffic_lock);
        printf("The tunnel now open to Whittier-bound traffic.\n");
        traffic = 'W';
        pthread_cond_broadcast(&clear); //broadcast change
        pthread_mutex_unlock(&traffic_lock);
        sleep(5); // makes the thread sleep for 5 sec

        if (done) {
            pthread_exit((void*) 0); //exit the thread
        }
        pthread_mutex_lock(&traffic_lock);
        printf("The tunnel is now closed to ALL traffic.\n");
        traffic = 'N';
        pthread_cond_broadcast(&clear); //broadcast change
        pthread_mutex_unlock(&traffic_lock);
        sleep(5); // makes the thread sleep for 5 sec

        if (done) {
            pthread_exit((void*) 0); //exit the thread
        }
        pthread_mutex_lock(&traffic_lock);
        printf("The tunnel now open to Whittier-bound traffic.\n");
        traffic = 'B';
        pthread_cond_broadcast(&clear); //broadcast change
        pthread_mutex_unlock(&traffic_lock);
        sleep(5); // makes the thread sleep for 5 sec

        if (done) {
            pthread_exit((void*) 0); //exit the thread
        }
        pthread_mutex_lock(&traffic_lock);
        printf("The tunnel is now closed to ALL traffic.\n");
        traffic = 'N';
        pthread_cond_broadcast(&clear); //broadcast change
        pthread_mutex_unlock(&traffic_lock);
        sleep(5); // makes the thread sleep for 5 sec
    }
}
//a thread for car going to Whittier bound
void *toWittier(void *arg) {
    int carNo = (int)arg;
    bool delay = 0;
    int sleeptime = 0;

    for (int i = carNo; i >= 0; i--){ //fetching the upcoming arrival time to sleep time
        sleeptime += cars.at(i).arrivalTime;
    }
    sleep(sleeptime); // makes thread sleep until arrival time
    pthread_mutex_lock(&traffic_lock);
    printf("Car #%d going to Whittier arrives at the tunnel.\n", carNo + 1);
    while (traffic != 'W' || ncars >= maxcars) {
        if(ncars >= maxcars){
            if(traffic == 'W'){
                delay = 1;
            }
        }
        pthread_cond_wait(&clear, &traffic_lock); //unlocks the mutex before it sleeps
    }
    ncars++;
    whittierBound++;
    if(delay){
        delayed++;
    }
    printf("Car #%d going to Whittier enters the tunnel.\n", carNo + 1);
    pthread_mutex_unlock(&traffic_lock);
    sleep(cars.at(carNo).crossTime); // makes thread sleep until cross time
    pthread_mutex_lock(&traffic_lock);
    ncars --;
    printf("Car #%d going to Whittier exits the tunnel.\n", carNo + 1);
    if (ncars < maxcars){
        pthread_cond_broadcast(&clear); //broadcast change
    }
    pthread_mutex_unlock(&traffic_lock);

}
//a thread for car going to Bear-Valley bound
void *toBearValley(void *arg) {
    int carNo = (int)arg;
    bool delay = 0;
    int sleeptime = 0;

    for (int i = carNo; i >= 0; i--){ //fetching the upcoming arrival time to sleep time
        sleeptime += cars.at(i).arrivalTime;
    }
    sleep(sleeptime); // makes thread sleep until arrival time
    pthread_mutex_lock(&traffic_lock);
    printf("Car #%d going to Bear Valley arrives at the tunnel.\n", carNo + 1);
    while (traffic != 'B' || ncars >= maxcars) {
        if(ncars >= maxcars){
            if(traffic == 'B'){
                delay = 1;
            }
        }
        pthread_cond_wait(&clear, &traffic_lock); //unlocks the mutex before it sleeps
    }
    ncars++;
    bearValleyBound++;
    if(delay){
        delayed++;
    }
    printf("Car #%d going to Bear Valley enters the tunnel.\n", carNo + 1);
    pthread_mutex_unlock(&traffic_lock);
    sleep(cars.at(carNo).crossTime); // makes thread sleep until cross time
    pthread_mutex_lock(&traffic_lock);
    ncars --;
    printf("Car #%d going to Bear Valley exits the tunnel.\n", carNo + 1);
    if (ncars < maxcars){
        pthread_cond_broadcast(&clear); //broadcast change
    }
    pthread_mutex_unlock(&traffic_lock);

}

int main(int argc, char **argv) {
    FILE* input = NULL;
    input = fopen("input.txt", "r");
    if(input == 0){
        printf("File not found\n");
        exit(1);
    }
    fscanf(input, "%d", &maxcars); //reads the first line for the maximum # of cars in the tunnel
    printf("-- Maximum number of cars in the tunnel: %d\n",maxcars);
    char dir[2];
    int arrival_time;
    int cross_time;
    int totalNCars = 0;
    while(fscanf(input, "%d %s %d", &arrival_time, dir, &cross_time) != EOF){ // read the rest of the lines for carInfo
        cars.push_back({arrival_time, cross_time, dir});
        totalNCars++; // increment number cars in the text file
    }
    fclose(input); // done reading the file
    pthread_t tid; // creates tid thread
    pthread_create(&tid, NULL, tunnel, (void *) 0); // create a new thread for tunnel function

    pthread_t cartid[totalNCars]; // creates cartid thread
    for(int i = 0; i < cars.size(); i++){ //checks for the next update direction to go to their specific thread function
        char cardirection = cars.at(i).direction.at(0);
        if (cardirection == 'W') {
            pthread_create(&cartid[i], NULL, toWittier, (void *) i); // create a new thread for  Whittier function
        }
        if (cardirection == 'B') {
            pthread_create(&cartid[i], NULL, toBearValley, (void *) i); // create a new thread for Bear-Valley function
        }
    }
    for (int i = 0; i < totalNCars; i++){
        pthread_join(cartid[i],NULL); //suspends the exec of the cars thread until the main thread terminates
    }
    done = 1;
    pthread_join(tid,NULL); //suspends the exec of the tunnel thread until the main thread terminates
    //prints # of cars arriving at either Bear Valley or Whittier and # of cars delayed
    printf("%d car(s) going to Bear Valley arrived at the tunnel.\n", bearValleyBound);
    printf("%d car(s) going to Whittier arrived at the tunnel.\n", whittierBound);
    printf("%d car(s) were delayed.\n", delayed);
}
