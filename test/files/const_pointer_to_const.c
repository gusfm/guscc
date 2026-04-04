int main()
{
    const int x = 42;
    const int y = 10;
    const int *p = &x;
    p = &y;
    return *p;
}
