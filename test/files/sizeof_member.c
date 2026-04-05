struct s {
    int x;
    char y;
};

int main()
{
    struct s v;
    return sizeof v.x + sizeof v.y;
}
