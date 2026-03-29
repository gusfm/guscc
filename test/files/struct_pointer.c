struct point { int x; int y; };

int main() {
    struct point p;
    struct point *pp;
    pp = &p;
    pp->x = 40;
    pp->y = 2;
    return pp->x + pp->y;
}
