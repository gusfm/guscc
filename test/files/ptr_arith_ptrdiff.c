int main()
{
    int a[5];
    int *p;
    int *q;
    p = a + 4;
    q = a;
    return (int)(p - q);
}
