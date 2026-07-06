void assert(int expected, int actual, char *code);

#define ASSERT(x, y) assert(x, y, #y)