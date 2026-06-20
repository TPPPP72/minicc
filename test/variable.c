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

int main()
{
    ASSERT(3, test_var1());
    ASSERT(3, test_var2());
    ASSERT(8, test_var3());
    ASSERT(6, test_var4());
    ASSERT(3, test_var5());
    ASSERT(8, test_var6());

    ASSERT(8, test_sizeof_1());
    ASSERT(8, test_sizeof_2());
    ASSERT(8, test_sizeof_3());
    ASSERT(32, test_sizeof_4());
    ASSERT(96, test_sizeof_5());
    ASSERT(32, test_sizeof_6());
    ASSERT(8, test_sizeof_7());
    ASSERT(9, test_sizeof_8());
    ASSERT(9, test_sizeof_9());
    ASSERT(8, test_sizeof_10());
    ASSERT(8, test_sizeof_11());
    ASSERT(1, test_sizeof_12());

    ASSERT(0, g1);
    ASSERT(3, test_gvar1());
    ASSERT(0, get_g2_0());
    ASSERT(1, get_g2_1());
    ASSERT(2, get_g2_2());
    ASSERT(3, get_g2_3());
    ASSERT(8, sizeof(g1));
    ASSERT(32, sizeof(g2));

    ASSERT(1, test_char1());
    ASSERT(1, test_char2());
    ASSERT(2, test_char3());
    ASSERT(1, test_sizeof_char1());
    ASSERT(10, test_sizeof_char2());

    ASSERT(2, test_scope1());
    ASSERT(2, test_scope2());
    ASSERT(3, test_scope3());

    printf("OK\n");
    return 0;
}