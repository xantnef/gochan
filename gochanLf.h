#include <vector>
#include <atomic>

#include "gochan.h"

template <class T>
class gochanLf : public gochan<T> {
    std::vector<T> elems;
    std::atomic<unsigned> seqAllocated, seqWritten, seqAssigned, seqRead;

    unsigned storsize() const {
        return this->size+1;
    }

public:
    gochanLf(unsigned size);

    void send(const T&);
    T recv();
    void close(void);
};

template <class T>
gochanLf<T>::gochanLf(unsigned size) : gochan<T>(size)
{
    elems.reserve(storsize());

    seqAllocated = seqWritten = seqAssigned = seqRead = 0;
}

/* Overflow mitigation: signed comparison (inspired by linux jiffies).
   Limitation: a is not ahead for more than INT_MAX.
   Or it is mistreated as "already far behind".
 */
bool greater_than(unsigned a, unsigned b)
{
    return ((int)(a-b)) > 0;
}

template <class T>
void gochanLf<T>::send(const T& elem)
{
    unsigned seq = ++seqAllocated;

    while (greater_than(seq, seqRead + storsize())) {
        ;
    }

    elems[seq % storsize()] = elem;

    seq--; // CAS below requires &T as expected value
    while (!seqWritten.compare_exchange_weak(seq, seq+1)) {
        ;
    }

    while (greater_than(seq, seqRead + this->size)) {
        ;
    }
}

template <class T>
T gochanLf<T>::recv(void)
{
    unsigned seq = ++seqAssigned;

    while (greater_than(seq, seqWritten)) {
        ;
    }

    T elem = elems[seq % storsize()];

    seq--; // CAS below requires &T as expected value
    while (!seqRead.compare_exchange_weak(seq, seq+1)) {
        ;
    }

    return elem;
}

template <class T>
void gochanLf<T>::close(void)
{
    throw std::logic_error("not implemented");
}
