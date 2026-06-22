#include "test.h"

int test_basic1()
{
    struct
    {
        int a;
        int b;
    } x;
    x.a = 1;
    x.b = 2;
    return x.a;
}

int test_basic2()
{
    struct
    {
        int a;
        int b;
    } x;
    x.a = 1;
    x.b = 2;
    return x.b;
}

int test_basic3()
{
    struct
    {
        char a;
        int b;
        char c;
    } x;
    x.a = 1;
    x.b = 2;
    x.c = 3;
    return x.a;
}

int test_basic4()
{
    struct
    {
        char a;
        int b;
        char c;
    } x;
    x.b = 1;
    x.b = 2;
    x.c = 3;
    return x.b;
}

int test_basic5()
{
    struct
    {
        char a;
        int b;
        char c;
    } x;
    x.a = 1;
    x.b = 2;
    x.c = 3;
    return x.c;
}

int test_array1()
{
    struct
    {
        char a;
        char b;
    } x[3];
    char *p = &x[0].a;
    p[0]    = 0;
    return x[0].a;
}

int test_array2()
{
    struct
    {
        char a;
        char b;
    } x[3];
    char *p = &x[0].a;
    p[1]    = 1;
    return x[0].b;
}

int test_array3()
{
    struct
    {
        char a;
        char b;
    } x[3];
    char *p = &x[0].a;
    p[2]    = 2;
    return x[1].a;
}

int test_array4()
{
    struct
    {
        char a;
        char b;
    } x[3];
    char *p = &x[0].a;
    p[3]    = 3;
    return x[1].b;
}

int test_member_array1()
{
    struct
    {
        char a[3];
        char b[5];
    } x;
    char *p = x.a;
    x.a[0]  = 6;
    return p[0];
}

int test_member_array2()
{
    struct
    {
        char a[3];
        char b[5];
    } x;
    char *p_a = x.a;
    char *p_b = &x.b[0];
    int delta = p_b - p_a;

    x.b[0] = 7;
    return p_a[delta];
}

int test_nested()
{
    struct
    {
        struct
        {
            char b;
        } a;
    } x;
    x.a.b = 6;
    return x.a.b;
}

int test_sizeof1()
{
    struct
    {
        int a;
    } x;
    return sizeof(x);
}
int test_sizeof2()
{
    struct
    {
        int a;
        int b;
    } x;
    return sizeof(x);
}
int test_sizeof3()
{
    struct
    {
        int a[3];
    } x;
    return sizeof(x);
}
int test_sizeof4()
{
    struct
    {
        int a;
    } x[4];
    return sizeof(x);
}
int test_sizeof5()
{
    struct
    {
        int a[3];
    } x[2];
    return sizeof(x);
}
int test_sizeof6()
{
    struct
    {
        char a;
        char b;
    } x;
    return sizeof(x);
}
int test_sizeof7()
{
    struct
    {
        char a;
        int b;
    } x;
    return sizeof(x);
}

int test_tag1()
{
    struct t
    {
        int a;
        int b;
    } x;
    struct t y;
    return sizeof(y);
}

int test_tag2()
{
    struct t
    {
        int a;
        int b;
    };
    struct t y;
    return sizeof(y);
}

int test_tag3()
{
    struct t
    {
        char a[2];
    };
    {
        struct t
        {
            char a[4];
        };
    }
    struct t y;
    return sizeof(y);
}

int test_tag4()
{
    struct t
    {
        int x;
    };
    int t = 1;
    struct t y;
    y.x = 2;
    return t + y.x;
}

int main()
{
    ASSERT(1, test_basic1());
    ASSERT(2, test_basic2());
    ASSERT(1, test_basic3());
    ASSERT(2, test_basic4());
    ASSERT(3, test_basic5());

    ASSERT(0, test_array1());
    ASSERT(1, test_array2());
    ASSERT(2, test_array3());
    ASSERT(3, test_array4());

    ASSERT(6, test_member_array1());
    ASSERT(7, test_member_array2());

    ASSERT(6, test_nested());

    ASSERT(8, test_sizeof1());
    ASSERT(16, test_sizeof2());
    ASSERT(24, test_sizeof3());
    ASSERT(32, test_sizeof4());
    ASSERT(48, test_sizeof5());
    ASSERT(2, test_sizeof6());
    ASSERT(16, test_sizeof7());

    ASSERT(16, test_tag1());
    ASSERT(16, test_tag2());
    ASSERT(2, test_tag3());
    ASSERT(3, test_tag4());

    printf("OK\n");
    return 0;
}