int main()
{
    int a = 0x2A;
    int b = 0xff;
    int c = 0x100;
    if (a != 42) return 1;
    if (b != 255) return 2;
    if (c != 256) return 3;
    return a;
}
