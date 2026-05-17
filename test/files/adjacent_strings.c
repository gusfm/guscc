int my_strlen(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

int main()
{
    char *s = "hello " "world" " 42";
    if (my_strlen(s) != 14) return 1;
    if (s[0] != 'h') return 2;
    if (s[6] != 'w') return 3;
    if (s[11] != ' ') return 4;
    return 42;
}
