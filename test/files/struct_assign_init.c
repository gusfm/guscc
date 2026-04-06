struct point { int x; int y; };

int main() {
    struct point a;
    a.x = 10;
    a.y = 32;
    struct point b = a;
    return b.x + b.y;
}
