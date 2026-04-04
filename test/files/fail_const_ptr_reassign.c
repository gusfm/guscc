int main()
{
    int x = 5;
    int y = 10;
    int * const p = &x;
    p = &y;
    return *p;
}
