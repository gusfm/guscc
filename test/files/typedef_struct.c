struct point {
    int x;
    int y;
};

typedef struct point point_t;

int main()
{
    point_t p;
    p.x = 40;
    p.y = 2;
    return p.x + p.y;
}
