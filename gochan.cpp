/* Primitive Go-style channels in C++ */

#include <iostream>

#include "gochanPt.h"
#include "gochanQ.h"

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
    std::vector<gochan<DumbClass>*> chs = {
        new gochanPt<DumbClass>(1),
        new gochanQ<DumbClass>(1)
    };

    DumbClass d42(42);

    for (auto ch : chs) { 
        ch->send(d42);
        std::cout << ch->recv().ninst << std::endl;
    }

    return 0;
}
