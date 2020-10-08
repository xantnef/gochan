/* Primitive Go-style channels in C++ */

#include <queue>
#include <iostream>

#include <pthread.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

class Locker {
public:
    Locker(int ncond) {}

    virtual void lock(void) = 0;
    virtual void unlock(void) = 0;
    virtual void wait(int ncond) = 0;
    virtual void wake(int ncond) = 0;
    virtual void wakeAll(int ncond) = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class PthreadLocker : public Locker {
    pthread_mutex_t mtx;
    pthread_cond_t* cond;
public:
    PthreadLocker(int ncond) : Locker(ncond) {
        pthread_mutex_init(&mtx, NULL);
        cond = new pthread_cond_t[ncond];
        for (int i = 0; i < sizeof(cond)/sizeof(cond[0]); i++) {
            pthread_cond_init(&cond[i], NULL);
        }
    }
    ~PthreadLocker() {
        delete [] cond;
    }

    void lock(void) {
        pthread_mutex_lock(&mtx);
    }
    void unlock(void) {
        pthread_mutex_unlock(&mtx);
    }
    void wait(int ncond) {
        pthread_cond_wait(&cond[ncond], &mtx);
    }
    void wake(int ncond) {
        pthread_cond_signal(&cond[ncond]);
    }
    void wakeAll(int ncond) {
        pthread_cond_broadcast(&cond[ncond]);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
class gochan {
    unsigned size;
    Locker* locker;

    std::queue<T> elems;
    unsigned seqRead, seqWritten;

    enum {
        writeEvent,
        readEvent,
        numEvents
    };

public:
    gochan(unsigned size);
    ~gochan() {
        delete locker;
    }

    void send(const T&);
    T recv();
};

template <class T>
gochan<T>::gochan(unsigned size)
{
    this->size = size;
    locker = new PthreadLocker(numEvents);
    seqRead = seqWritten = 0;
}

template <class T>
void gochan<T>::send(const T& elem)
{
    unsigned seq;
    locker->lock();

    elems.push(elem);
    seq = ++seqWritten;

    locker->wake(writeEvent);

    while (seq > seqRead + size) {
        locker->wait(readEvent);
    }
    locker->unlock();
}

template <class T>
T gochan<T>::recv(void)
{
    locker->lock();

    while (seqWritten == seqRead) {
        locker->wait(writeEvent);
    }

    T elem = elems.front();
    elems.pop();
    seqRead++;

    // TODO too bad this has to be a broadcast
    locker->wakeAll(readEvent);
    locker->unlock();
    return elem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

class DumbClass final {
public:
    static int seq;
    int id;
    int ninst;

    DumbClass(int id) : id(id), ninst(++seq) {
        std::cout << "dumb#" << id << "/" << ninst << std::endl;
    }
    DumbClass(const DumbClass& src) : DumbClass(src.id) {}
    DumbClass(const DumbClass&& src) : DumbClass(src.id) {
        std::cout << "  (moved)\n";
    }
    ~DumbClass() {
        std::cout << "destroy " << id << "/" << ninst << std::endl;
    }
};

int DumbClass::seq = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
    gochan<DumbClass> ch(1);
    DumbClass d42(42);

    ch.send(d42);
    std::cout << ch.recv().ninst << std::endl;

    return 0;
}
