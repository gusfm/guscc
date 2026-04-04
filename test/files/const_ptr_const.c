int main()
{
    int x = 42;
    int * const p = &x;
    return *p;
}
