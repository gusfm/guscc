int main()
{
    int a[3];
    int *p;
    a[0] = 1;
    a[1] = 7;
    a[2] = 99;
    p = a + 2;
    p = p - 1;
    return *p;
}
