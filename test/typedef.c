#include "test.h"

typedef int MyInt, MyInt2[4];

int test_basic()
{
    typedef int t;
    t x = 1;
    return x;
}

int test_struct()
{
    typedef struct
    {
        int a;
    } t;
    t x;
    x.a = 1;
    return x.a;
}

int test_shadowing()
{
    typedef int t;
    t t = 1;
    return t;
}

int test_nested_scope()
{
    typedef struct
    {
        int a;
    } t;
    {
        typedef int t;
    }
    t x;
    x.a = 2;
    return x.a;
}

int test_implicit_type_sizeof()
{
    typedef int t;
    t x;
    return sizeof(x);
}

int main()
{
    ASSERT(1, test_basic());
    ASSERT(1, test_struct());
    ASSERT(1, test_shadowing());
    ASSERT(2, test_nested_scope());
    ASSERT(4, test_implicit_type_sizeof());

    {
        MyInt x = 3;
        ASSERT(3, x);
    }

    {
        MyInt2 x;
        ASSERT(16, sizeof(x));
    }

    printf("OK\n");
    return 0;
}