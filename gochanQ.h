#include <queue>
#include <condition_variable>
#include <mutex>

////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
class gochanQ : public gochan<T> {
    std::queue<T> elems;
    std::queue<std::condition_variable*> writers;
    std::queue<std::condition_variable*> readers;

    std::mutex _mtx;
    std::unique_lock<std::mutex> lk;

public:
    gochanQ(unsigned size) : gochan<T>(size), lk(_mtx, std::defer_lock) {}

    void send(const T& elem);
    T recv(void);
};

template <class T>
void gochanQ<T>::send(const T& elem)
{
    lk.lock();

    elems.push(elem);

    if (!readers.empty()) {
        readers.front()->notify_one();
        readers.pop();
    }

    if (!writers.empty() || elems.size() > this->size) {
        std::condition_variable cond;
        writers.push(&cond);
        cond.wait(lk);
    }

    lk.unlock();
}

template <class T>
T gochanQ<T>::recv(void)
{
    lk.lock();

    if (elems.empty() || !readers.empty()) {
        std::condition_variable cond;
        readers.push(&cond);
        cond.wait(lk);
    }

    if (elems.empty())
        throw std::logic_error("unexpected elems.size()");

    T elem = elems.front();
    elems.pop();

    if (!writers.empty()) {
        writers.front()->notify_one();
        writers.pop();
    }

    lk.unlock();
    return elem;
}
