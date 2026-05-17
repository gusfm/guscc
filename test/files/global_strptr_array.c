static const char *names[3] = {"foo", "bar", "baz"};

int main()
{
    char *a = names[0];
    char *b = names[1];
    char *c = names[2];
    if (a[0] != 'f') return 1;
    if (b[0] != 'b') return 2;
    if (c[2] != 'z') return 3;
    return 42;
}
