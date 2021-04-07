#include "gochanLf.h"

#define test(call) \
    if (!(call)) { \
        fprintf(stderr, "err at line %d\n", __LINE__); \
        return -1; \
    }

#define Nest(call) test(!(call))

static int test_overflow(void)
{
    unsigned
        last_pos = (-1U)/2,
        first_neg = (-1U)/2 + 1,
        last_neg = -1U;

    test(greater_than(1, 0));

    test(greater_than(last_pos, 0));
    Nest(greater_than(last_pos, last_neg));

    Nest(greater_than(first_neg, 0));
    test(greater_than(first_neg, last_pos));

    Nest(greater_than(last_neg, last_pos));
    test(greater_than(last_neg, first_neg));
    test(greater_than(last_neg, last_neg-1));

    Nest(greater_than(0, last_pos));
    Nest(greater_than(0, first_neg));
    test(greater_than(0, first_neg+1));
    test(greater_than(0, last_neg));

    Nest(greater_than(1, last_pos));
    Nest(greater_than(1, first_neg));
    test(greater_than(1, last_neg));

    return 0;
}

int main(void)
{
    return test_overflow();
}
