#pragma once

template <class T>
class gochan {
protected:
    unsigned size;
    bool closed;

public:
    gochan(unsigned size) : size(size), closed(false) {}
    virtual ~gochan() {}

    virtual void send(const T&) = 0;
    virtual T recv() = 0;
    virtual void close(void) = 0;
};
