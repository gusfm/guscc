union val { int i; char c; };
struct container { union val v; int tag; };

int main() {
    struct container s;
    s.v.i = 42;
    s.tag = 1;
    return s.v.i;
}
