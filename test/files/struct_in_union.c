struct point { int x; int y; };
union data { struct point p; int raw; };

int main() {
    union data d;
    d.p.x = 40;
    d.p.y = 2;
    return d.p.x + d.p.y;
}
