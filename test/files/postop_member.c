struct lex {
    char *p;
};

int main()
{
    char buf[8];
    buf[0] = 'A';
    buf[1] = 'B';
    buf[2] = 'C';
    struct lex l;
    l.p = buf;
    int c = *l.p++;
    int d = *l.p++;
    int e = *l.p++;
    if (c != 'A') return 1;
    if (d != 'B') return 2;
    if (e != 'C') return 3;
    return 42;
}
