int main()
{
    int a[2];
    int *p;
    a[0] = 42;
    a[1] = 0;
    p = a + 1;
    return *(--p);
}
