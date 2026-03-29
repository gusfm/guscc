int main()
{
    int a[2];
    int *p;
    a[0] = 1;
    a[1] = 42;
    p = a;
    return *(++p);
}
