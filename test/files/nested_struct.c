struct inner { int x; int y; };
struct outer { struct inner i; int z; };

int main() {
    struct outer o;
    o.i.x = 10;
    o.i.y = 32;
    o.z = 0;
    return o.i.x + o.i.y;
}
