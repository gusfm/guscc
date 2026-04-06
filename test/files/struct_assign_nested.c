struct inner { int x; int y; };
struct outer { struct inner i; int z; };

int main() {
    struct outer a;
    a.i.x = 10;
    a.i.y = 32;
    a.z = 0;
    struct outer b;
    b.i = a.i;
    return b.i.x + b.i.y;
}
