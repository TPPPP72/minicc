#include "test.h"

int test_ptr1()
{
    int x = 3;
    return *&x;
}
int test_ptr2()
{
    int x   = 3;
    int *y  = &x;
    int **z = &y;
    return **z;
}
int test_ptr3()
{
    int x = 3;
    int y = 5;
    return *(&x + 1);
}
int test_ptr4()
{
    int x = 3;
    int y = 5;
    return *(&y - 1);
}
int test_ptr5()
{
    int x = 3;
    int y = 5;
    return *(&x - (-1));
}
int test_ptr6()
{
    int x  = 3;
    int *y = &x;
    *y     = 5;
    return x;
}
int test_ptr7()
{
    int x     = 3;
    int y     = 5;
    *(&x + 1) = 7;
    return y;
}
int test_ptr8()
{
    int x         = 3;
    int y         = 5;
    *(&y - 2 + 1) = 7;
    return x;
}
int test_ptr9()
{
    int x = 3;
    return (&x + 2) - &x + 3;
}
int test_ptr10()
{
    int x, y;
    x = 3;
    y = 5;
    return x + y;
}
int test_ptr11()
{
    int x = 3, y = 5;
    return x + y;
}

int test_arr1()
{
    int x[2];
    int *y = &x;
    *y     = 3;
    return *x;
}
int test_arr2()
{
    int x[3];
    *x       = 3;
    *(x + 1) = 4;
    *(x + 2) = 5;
    return *x;
}
int test_arr3()
{
    int x[3];
    *x       = 3;
    *(x + 1) = 4;
    *(x + 2) = 5;
    return *(x + 1);
}
int test_arr4()
{
    int x[3];
    *x       = 3;
    *(x + 1) = 4;
    *(x + 2) = 5;
    return *(x + 2);
}

int test_arr2d_1()
{
    int x[2][3];
    int *y = x;
    *y     = 0;
    return **x;
}
int test_arr2d_2()
{
    int x[2][3];
    int *y   = x;
    *(y + 1) = 1;
    return *(*x + 1);
}
int test_arr2d_3()
{
    int x[2][3];
    int *y   = x;
    *(y + 2) = 2;
    return *(*x + 2);
}
int test_arr2d_4()
{
    int x[2][3];
    int *y   = x;
    *(y + 3) = 3;
    return **(x + 1);
}
int test_arr2d_5()
{
    int x[2][3];
    int *y   = x;
    *(y + 4) = 4;
    return *(*(x + 1) + 1);
}
int test_arr2d_6()
{
    int x[2][3];
    int *y   = x;
    *(y + 5) = 5;
    return *(*(x + 1) + 2);
}

int test_sub1()
{
    int x[3];
    *x   = 3;
    x[1] = 4;
    x[2] = 5;
    return *x;
}
int test_sub2()
{
    int x[3];
    *x   = 3;
    x[1] = 4;
    x[2] = 5;
    return *(x + 1);
}
int test_sub3()
{
    int x[3];
    *x   = 3;
    x[1] = 4;
    x[2] = 5;
    return *(x + 2);
}
int test_sub4()
{
    int x[3];
    *x    = 3;
    x[1]  = 4;
    2 [x] = 5;
    return *(x + 2);
}

int test_sub2d_1()
{
    int x[2][3];
    int *y = x;
    y[0]   = 0;
    return x[0][0];
}
int test_sub2d_2()
{
    int x[2][3];
    int *y = x;
    y[1]   = 1;
    return x[0][1];
}
int test_sub2d_3()
{
    int x[2][3];
    int *y = x;
    y[2]   = 2;
    return x[0][2];
}
int test_sub2d_4()
{
    int x[2][3];
    int *y = x;
    y[3]   = 3;
    return x[1][0];
}
int test_sub2d_5()
{
    int x[2][3];
    int *y = x;
    y[4]   = 4;
    return x[1][1];
}
int test_sub2d_6()
{
    int x[2][3];
    int *y = x;
    y[5]   = 5;
    return x[1][2];
}

int test_align1()
{
    int x;
    int y;
    char z;
    char *a = &y;
    char *b = &z;
    return b - a;
}

int test_align2()
{
    int x;
    char y;
    int z;
    char *a = &y;
    char *b = &z;
    return b - a;
}

int main()
{
    ASSERT(3, test_ptr1());
    ASSERT(3, test_ptr2());
    ASSERT(5, test_ptr3());
    ASSERT(3, test_ptr4());
    ASSERT(5, test_ptr5());
    ASSERT(5, test_ptr6());
    ASSERT(7, test_ptr7());
    ASSERT(7, test_ptr8());
    ASSERT(5, test_ptr9());
    ASSERT(8, test_ptr10());
    ASSERT(8, test_ptr11());

    ASSERT(3, test_arr1());
    ASSERT(3, test_arr2());
    ASSERT(4, test_arr3());
    ASSERT(5, test_arr4());

    ASSERT(0, test_arr2d_1());
    ASSERT(1, test_arr2d_2());
    ASSERT(2, test_arr2d_3());
    ASSERT(3, test_arr2d_4());
    ASSERT(4, test_arr2d_5());
    ASSERT(5, test_arr2d_6());

    ASSERT(3, test_sub1());
    ASSERT(4, test_sub2());
    ASSERT(5, test_sub3());
    ASSERT(5, test_sub4());

    ASSERT(0, test_sub2d_1());
    ASSERT(1, test_sub2d_2());
    ASSERT(2, test_sub2d_3());
    ASSERT(3, test_sub2d_4());
    ASSERT(4, test_sub2d_5());
    ASSERT(5, test_sub2d_6());

    ASSERT(15, test_align1());
    ASSERT(1, test_align2());

    printf("OK\n");
    return 0;
}