#include "test.h"

int test_statement_expr_1()
{
    int x;
    if (0)
        x = 2;
    else
        x = 3;
    return x;
}

int test_statement_expr_2()
{
    int x;
    if (1 - 1)
        x = 2;
    else
        x = 3;
    return x;
}

int test_statement_expr_3()
{
    int x;
    if (1)
        x = 2;
    else
        x = 3;
    return x;
}

int test_statement_expr_4()
{
    int x;
    if (2 - 1)
        x = 2;
    else
        x = 3;
    return x;
}

int test_statement_expr_5()
{
    int i = 0;
    int j = 0;
    for (i = 0; i <= 10; i = i + 1)
        j = i + j;
    return j;
}

int test_statement_expr_6()
{
    int i = 0;
    while (i < 10)
        i = i + 1;
    return i;
}

int test_statement_expr_7()
{
    1;
    {
        2;
    }
    return 3;
}

int test_statement_expr_8()
{
    ;
    ;
    ;
    return 5;
}

int test_statement_expr_9()
{
    int i = 0;
    while (i < 10)
        i = i + 1;
    return i;
}

int test_statement_expr_10()
{
    int i = 0;
    int j = 0;
    while (i <= 10)
    {
        j = i + j;
        i = i + 1;
    }
    return j;
}

int test_statement_expr_11()
{
    int j = 0;
    for (int i = 0; i <= 10; i = i + 1)
        j = j + i;
    return j;
}

int test_statement_expr_12()
{
    int i = 3;
    int j = 0;
    for (int i = 0; i <= 10; i = i + 1)
        j = j + i;
    return i;
}

int main()
{
    ASSERT(3, test_statement_expr_1());
    ASSERT(3, test_statement_expr_2());
    ASSERT(2, test_statement_expr_3());
    ASSERT(2, test_statement_expr_4());

    ASSERT(55, test_statement_expr_5());

    ASSERT(10, test_statement_expr_6());

    ASSERT(3, test_statement_expr_7());
    ASSERT(5, test_statement_expr_8());

    ASSERT(10, test_statement_expr_9());
    ASSERT(55, test_statement_expr_10());

    ASSERT(55, test_statement_expr_11());
    ASSERT(3, test_statement_expr_12());
    return 0;
}