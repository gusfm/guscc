struct inner { int a; int b; };
struct outer { struct inner i; int c; };

int main() {
    struct outer o;
    o.i.a = 42;
    struct outer *p = &o;
    return p->i.a;
}
