struct point {
    int x;
    int y;
};

int main()
{
    struct point p;
    int a[5];
    int *ptr = a;
    return sizeof(struct point) + sizeof a[0] + sizeof *ptr + sizeof p.x + sizeof "hello" + sizeof 42 + sizeof(char);
}
