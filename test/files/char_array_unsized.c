int main()
{
    char tmp[] = "hello";
    if (tmp[0] != 'h') return 1;
    if (tmp[1] != 'e') return 2;
    if (tmp[4] != 'o') return 3;
    if (tmp[5] != 0) return 4;
    return 42;
}
