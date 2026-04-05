union data { int x; char y; };

int main() {
    union data d;
    union data *p;
    p = &d;
    p->x = 42;
    return p->x;
}
