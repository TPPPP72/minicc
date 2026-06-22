#include "test.h"

int test_union_size()
{
    union
    {
        int a;
        char b[6];
    } x;
    return sizeof(x);
}

int test_union_byte0()
{
    union
    {
        int a;
        char b[4];
    } x;
    x.a = 515;
    return x.b[0];
}

int test_union_byte1()
{
    union
    {
        int a;
        char b[4];
    } x;
    x.a = 515;
    return x.b[1];
}

int test_union_byte2()
{
    union
    {
        int a;
        char b[4];
    } x;
    x.a = 515;
    return x.b[2];
}

int test_union_byte3()
{
    union
    {
        int a;
        char b[4];
    } x;
    x.a = 515;
    return x.b[3];
}

int main()
{
    ASSERT(8, test_union_size());
    ASSERT(3, test_union_byte0());
    ASSERT(2, test_union_byte1());
    ASSERT(0, test_union_byte2());
    ASSERT(0, test_union_byte3());

    printf("OK\n");
    return 0;
}