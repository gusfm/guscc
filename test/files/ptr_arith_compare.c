int main()
{
    int a[3];
    int *p;
    int *q;
    p = a;
    q = a + 1;
    if (p < q)
        return 42;
    return 0;
}
