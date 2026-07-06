#include "test.h"

int test_size()
{
    union
    {
        int a;
        char b[6];
    } x;
    return sizeof(x);
}

int test_byte0()
{
    union
    {
        int a;
        char b[4];
    } x;
    x.a = 515;
    return x.b[0];
}
int test_byte1()
{
    union
    {
        int a;
        char b[4];
    } x;
    x.a = 515;
    return x.b[1];
}
int test_byte2()
{
    union
    {
        int a;
        char b[4];
    } x;
    x.a = 515;
    return x.b[2];
}
int test_byte3()
{
    union
    {
        int a;
        char b[4];
    } x;
    x.a = 515;
    return x.b[3];
}

int test_assign1()
{
    union
    {
        int a, b;
    } x, y;
    x.a = 3;
    y.a = 5;
    y   = x;
    return y.a;
}
int test_assign2()
{
    union
    {
        struct
        {
            int a, b;
        } c;
    } x, y;
    x.c.b = 3;
    y.c.b = 5;
    y     = x;
    return y.c.b;
}

int main()
{
    ASSERT(8, test_size());

    ASSERT(3, test_byte0());
    ASSERT(2, test_byte1());
    ASSERT(0, test_byte2());
    ASSERT(0, test_byte3());

    ASSERT(3, test_assign1());
    ASSERT(3, test_assign2());
    return 0;
}