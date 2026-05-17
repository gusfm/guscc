int main()
{
    int a = 42L;
    int b = 0xFFULL;
    int c = 100u;
    if (a != 42) return 1;
    if (b != 255) return 2;
    if (c != 100) return 3;
    return a;
}
