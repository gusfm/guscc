union data { int x; char y; };

int main() {
    union data d;
    d.x = 42;
    return d.x;
}
