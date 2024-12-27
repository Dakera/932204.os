#include <iostream>
#include <pthread.h>
#include <windows.h>

using namespace std;

class Monitor {
public:
    int ready = 0;
    pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    void provide(){
        pthread_mutex_lock(&lock);
        if (ready){
            pthread_mutex_unlock(&lock);
            return;
        }

        Sleep(1000);
        ready = 1;
        cout << "Provided\n";
        pthread_cond_signal(&cond1);
        pthread_mutex_unlock(&lock);
    }

    void consume(){
        pthread_mutex_lock(&lock);
        while (!ready){
            pthread_cond_wait(&cond1, &lock);
            cout << "Waiting...\n";
        }
        ready = 0;
        cout << "Consumed\n";
        pthread_mutex_unlock(&lock);
    }

};

void* producerThread(void* arg) {
    Monitor* monitor = static_cast<Monitor*>(arg);
    while (true) {
        monitor->provide();
    }
    return nullptr;
}

void* consumerThread(void* arg) {
    Monitor* monitor = static_cast<Monitor*>(arg);
    while (true) {
        monitor->consume();
    }
    return nullptr;
}


int main()
{
    Monitor monitor;
    
    pthread_t producer, consumer;
    pthread_create(&producer, nullptr, producerThread, &monitor);
    pthread_create(&consumer, nullptr, consumerThread, &monitor);

    pthread_join(producer, nullptr);
    pthread_join(consumer, nullptr);
    return 0;
}
