union val { int i; char c; };

int main() {
    union val a;
    a.i = 42;
    union val b;
    b = a;
    return b.i;
}
