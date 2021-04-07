class Locker {
public:
    Locker(int ncond) {}
    virtual ~Locker() {}

    virtual void lock(void) = 0;
    virtual void unlock(void) = 0;
    virtual void wait(int ncond) = 0;
    virtual void wake(int ncond) = 0;
    virtual void wakeAll(int ncond) = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

#include <pthread.h>

class PthreadLocker : public Locker {
    pthread_mutex_t mtx;
    pthread_cond_t* cond;
public:
    PthreadLocker(int ncond) : Locker(ncond) {
        pthread_mutex_init(&mtx, NULL);
        cond = new pthread_cond_t[ncond];
        for (int i = 0; i < ncond; i++) {
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

#include <queue>

#include "gochan.h"

template <class T>
class gochanPt : public gochan<T> {
    Locker* locker;

    std::queue<T> elems;
    unsigned seqRead, seqWritten;

    enum {
        writeEvent,
        readEvent,
        numEvents
    };

public:
    gochanPt(unsigned size);
    ~gochanPt() {
        if (!this->closed)
            close();
        delete locker;
    }

    void send(const T&);
    T recv();
    void close(void);
};

template <class T>
gochanPt<T>::gochanPt(unsigned size) : gochan<T>(size)
{
    locker = new PthreadLocker(numEvents);
    seqRead = seqWritten = 0;
}

template <class T>
void gochanPt<T>::send(const T& elem)
{
    unsigned seq;
    locker->lock();

    if (this->closed)
        throw std::logic_error("send on closed channel");

    elems.push(elem);
    seq = ++seqWritten;

    locker->wake(writeEvent);

    while (seq > seqRead + this->size) {
        if (this->closed)
            throw std::logic_error("send on closed channel");
        locker->wait(readEvent);
    }

    locker->unlock();
}

template <class T>
T gochanPt<T>::recv(void)
{
    locker->lock();

    while (seqWritten == seqRead && !this->closed) {
        locker->wait(writeEvent);
    }

    if (seqWritten == seqRead) {
        T elem;
        locker->unlock();
        return elem;
    }

    T elem = elems.front();
    elems.pop();
    seqRead++;

    // TODO too bad this has to be a broadcast
    locker->wakeAll(readEvent);
    locker->unlock();
    return elem;
}

template <class T>
void gochanPt<T>::close(void)
{
    locker->lock();

    if (this->closed)
        throw std::logic_error("close on closed channel");

    this->closed = true;

    locker->wakeAll(readEvent);
    locker->wakeAll(writeEvent);

    locker->unlock();
}
