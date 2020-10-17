#pragma once

template <class T>
class gochan {
protected:
    unsigned size;

public:
    gochan(unsigned size) : size(size) {}
    virtual ~gochan() {}

    virtual void send(const T&) = 0;
    virtual T recv() = 0;
};
