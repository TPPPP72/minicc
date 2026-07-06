#include "test.h"

int g1, g2[4];

int test_var1()
{
    int a;
    a = 3;
    return a;
}
int test_var2()
{
    int a = 3;
    return a;
}
int test_var3()
{
    int a = 3;
    int z = 5;
    return a + z;
}
int test_var4()
{
    int a;
    int b;
    a = b = 3;
    return a + b;
}
int test_var5()
{
    int foo = 3;
    return foo;
}
int test_var6()
{
    int foo123 = 3;
    int bar    = 5;
    return foo123 + bar;
}

int test_sizeof_1()
{
    int x;
    return sizeof(x);
}
int test_sizeof_2()
{
    int x;
    return sizeof x;
}
int test_sizeof_3()
{
    int *x;
    return sizeof(x);
}
int test_sizeof_4()
{
    int x[4];
    return sizeof(x);
}
int test_sizeof_5()
{
    int x[3][4];
    return sizeof(x);
}
int test_sizeof_6()
{
    int x[3][4];
    return sizeof(*x);
}
int test_sizeof_7()
{
    int x[3][4];
    return sizeof(**x);
}
int test_sizeof_8()
{
    int x[3][4];
    return sizeof(**x) + 1;
}
int test_sizeof_9()
{
    int x[3][4];
    return sizeof **x + 1;
}
int test_sizeof_10()
{
    int x[3][4];
    return sizeof(**x + 1);
}
int test_sizeof_11()
{
    int x = 1;
    return sizeof(x = 2);
}
int test_sizeof_12()
{
    int x = 1;
    sizeof(x = 2);
    return x;
}
int test_sizeof_13()
{
    short x;
    return sizeof(x);
}
int test_sizeof_14()
{
    long x;
    return sizeof(x);
}

int test_gvar1()
{
    g1 = 3;
    return g1;
}
int get_g2_0()
{
    g2[0] = 0;
    g2[1] = 1;
    g2[2] = 2;
    g2[3] = 3;
    return g2[0];
}
int get_g2_1()
{
    g2[0] = 0;
    g2[1] = 1;
    g2[2] = 2;
    g2[3] = 3;
    return g2[1];
}
int get_g2_2()
{
    g2[0] = 0;
    g2[1] = 1;
    g2[2] = 2;
    g2[3] = 3;
    return g2[2];
}
int get_g2_3()
{
    g2[0] = 0;
    g2[1] = 1;
    g2[2] = 2;
    g2[3] = 3;
    return g2[3];
}

int test_char1()
{
    char x = 1;
    return x;
}
int test_char2()
{
    char x = 1;
    char y = 2;
    return x;
}
int test_char3()
{
    char x = 1;
    char y = 2;
    return y;
}
int test_sizeof_char1()
{
    char x;
    return sizeof(x);
}
int test_sizeof_char2()
{
    char x[10];
    return sizeof(x);
}

int test_scope1()
{
    int x = 2;
    {
        int x = 3;
    }
    return x;
}
int test_scope2()
{
    int x = 2;
    {
        int x = 3;
    }
    int y = 4;
    return x;
}
int test_scope3()
{
    int x = 2;
    {
        x = 3;
    }
    return x;
}

int test_nested1()
{
    char *x[3];
    return sizeof(x);
}
int test_nested2()
{
    char (*x)[3];
    return sizeof(x);
}
int test_nested3()
{
    char(x);
    return sizeof(x);
}
int test_nested4()
{
    char(x)[3];
    return sizeof(x);
}
int test_nested5()
{
    char(x[3])[4];
    return sizeof(x);
}
int test_nested6()
{
    char(x[3])[4];
    return sizeof(x[0]);
}
int test_nested7()
{
    char *x[3];
    char y;
    x[0] = &y;
    y    = 3;
    return x[0][0];
}
int test_nested8()
{
    char x[3];
    char (*y)[3] = &x;
    y[0][0]      = 4;
    return y[0][0];
}

int main()
{
    ASSERT(3, test_var1());
    ASSERT(3, test_var2());
    ASSERT(8, test_var3());
    ASSERT(6, test_var4());
    ASSERT(3, test_var5());
    ASSERT(8, test_var6());

    ASSERT(4, test_sizeof_1());
    ASSERT(4, test_sizeof_2());
    ASSERT(8, test_sizeof_3());
    ASSERT(16, test_sizeof_4());
    ASSERT(48, test_sizeof_5());
    ASSERT(16, test_sizeof_6());
    ASSERT(4, test_sizeof_7());
    ASSERT(5, test_sizeof_8());
    ASSERT(5, test_sizeof_9());
    ASSERT(4, test_sizeof_10());
    ASSERT(4, test_sizeof_11());
    ASSERT(1, test_sizeof_12());
    ASSERT(2, test_sizeof_13());
    ASSERT(8, test_sizeof_14());

    ASSERT(0, g1);
    ASSERT(3, test_gvar1());
    ASSERT(0, get_g2_0());
    ASSERT(1, get_g2_1());
    ASSERT(2, get_g2_2());
    ASSERT(3, get_g2_3());
    ASSERT(4, sizeof(g1));
    ASSERT(16, sizeof(g2));

    ASSERT(1, test_char1());
    ASSERT(1, test_char2());
    ASSERT(2, test_char3());
    ASSERT(1, test_sizeof_char1());
    ASSERT(10, test_sizeof_char2());

    ASSERT(2, test_scope1());
    ASSERT(2, test_scope2());
    ASSERT(3, test_scope3());

    ASSERT(24, test_nested1());
    ASSERT(8, test_nested2());
    ASSERT(1, test_nested3());
    ASSERT(3, test_nested4());
    ASSERT(12, test_nested5());
    ASSERT(4, test_nested6());
    ASSERT(3, test_nested7());
    ASSERT(4, test_nested8());

    void *x;
    return 0;
}