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

    void wait_in_queue(std::queue<std::condition_variable*>&);
    void wake_queue(std::queue<std::condition_variable*>&);

public:
    gochanQ(unsigned size) : gochan<T>(size), lk(_mtx, std::defer_lock) {}
    ~gochanQ();

    void send(const T& elem);
    T recv(void);
    void close(void);
};

template <class T>
gochanQ<T>::~gochanQ(void)
{
    if (!this->closed)
        close();
}

template <class T>
void gochanQ<T>::wait_in_queue(std::queue<std::condition_variable*>& q)
{
    std::condition_variable cond;
    q.push(&cond);
    cond.wait(lk);
}

template <class T>
void gochanQ<T>::wake_queue(std::queue<std::condition_variable*>& q)
{
    q.front()->notify_one();
    q.pop();
}

template <class T>
void gochanQ<T>::send(const T& elem)
{
    lk.lock();

    if (this->closed)
        throw std::logic_error("send on closed channel");

    elems.push(elem);

    if (!readers.empty())
        wake_queue(readers);

    if (!writers.empty() || elems.size() > this->size)
        wait_in_queue(writers);

    if (this->closed)
        throw std::logic_error("send on closed channel");

    lk.unlock();
}

template <class T>
T gochanQ<T>::recv(void)
{
    lk.lock();

    if (!readers.empty() || (elems.empty() && !this->closed))
        wait_in_queue(readers);

    if (elems.empty()) {
        if (this->closed) {
            T elem;
            lk.unlock();
            return elem;
        }
        throw std::logic_error("unexpected empty channel");
    }

    T elem = elems.front();
    elems.pop();

    if (!writers.empty())
        wake_queue(writers);

    lk.unlock();
    return elem;
}

template <class T>
void gochanQ<T>::close(void)
{
    lk.lock();

    if (this->closed)
        throw std::logic_error("close on closed channel");

    // Do we have to wait for readers/writers woken for execution?
    while (!elems.empty() && !readers.empty()) {
        // just have to yield
        lk.unlock();
        lk.lock();
    }

    this->closed = true;

    while (!readers.empty())
        wake_queue(readers);

    while (!writers.empty())
        wake_queue(writers);

    lk.unlock();
}
