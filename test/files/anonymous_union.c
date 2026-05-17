typedef struct outer outer_t;
typedef struct outer {
    int kind;
    union {
        struct { int a; int b; } pair;
        int single;
    };
} outer_t;
int main()
{
    outer_t x;
    x.kind = 1;
    x.pair.a = 10;
    x.pair.b = 32;
    return x.pair.a + x.pair.b;
}
