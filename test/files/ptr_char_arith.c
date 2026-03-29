int main()
{
    char a[3];
    char *p;
    a[0] = 1;
    a[1] = 2;
    a[2] = 42;
    p = a;
    p = p + 2;
    return *p;
}
