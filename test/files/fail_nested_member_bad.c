struct inner { int x; };
struct outer { struct inner i; };

int main() {
    struct outer o;
    o.i.nonexistent = 1;
    return 0;
}
