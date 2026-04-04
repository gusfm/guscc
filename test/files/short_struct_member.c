struct point {
    short x;
    short y;
};

int main()
{
    struct point p;
    p.x = 20;
    p.y = 22;
    return p.x + p.y;
}
