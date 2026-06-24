#include "test.h"

int test_combination1()
{
    char x;
    return sizeof(x);
}
int test_combination2()
{
    short int x;
    return sizeof(x);
}
int test_combination3()
{
    int short x;
    return sizeof(x);
}
int test_combination4()
{
    int x;
    return sizeof(x);
}
int test_combination5()
{
    long int x;
    return sizeof(x);
}
int test_combination6()
{
    int long x;
    return sizeof(x);
}
int test_combination7()
{
    long long x;
    return sizeof(x);
}

int main()
{
    ASSERT(1, test_combination1());
    ASSERT(2, test_combination2());
    ASSERT(2, test_combination3());
    ASSERT(4, test_combination4());
    ASSERT(8, test_combination5());
    ASSERT(8, test_combination6());
    ASSERT(8, test_combination7());

    printf("OK\n");
    return 0;
}