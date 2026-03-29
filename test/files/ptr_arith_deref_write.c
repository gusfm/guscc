int main()
{
    int x;
    int *p;
    x = 0;
    p = &x;
    *p = 42;
    return x;
}
