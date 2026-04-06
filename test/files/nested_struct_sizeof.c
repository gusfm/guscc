struct inner { int x; int y; };
struct outer { struct inner i; int z; };

int main() {
    struct outer o;
    return sizeof o.i;
}
