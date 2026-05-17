int main()
{
    char *s = "a\"b";
    if (s[0] != 'a') return 1;
    if (s[1] != '"') return 2;
    if (s[2] != 'b') return 3;
    if (s[3] != 0) return 4;
    return 42;
}
