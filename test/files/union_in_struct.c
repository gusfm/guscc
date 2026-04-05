union val { int i; char c; };
struct container { union val v; int tag; };

int main() {
    struct container s;
    s.tag = 42;
    return s.tag;
}
