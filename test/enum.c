#include "test.h"

int main()
{
    {
        enum
        {
            zero,
            one,
            two
        };
        ASSERT(0, zero);
        ASSERT(1, one);
        ASSERT(2, two);
    }

    {
        enum
        {
            five = 5,
            six,
            seven
        };
        ASSERT(5, five);
        ASSERT(6, six);
    }

    {
        enum
        {
            zero,
            five  = 5,
            three = 3,
            four
        };
        ASSERT(0, zero);
        ASSERT(5, five);
        ASSERT(3, three);
        ASSERT(4, four);
    }

    {
        enum
        {
            zero,
            one,
            two
        } x;
        ASSERT(4, sizeof(x));
    }

    {
        enum t
        {
            zero,
            one,
            two
        };
        enum t y;
        ASSERT(4, sizeof(y));
    }

    return 0;
}