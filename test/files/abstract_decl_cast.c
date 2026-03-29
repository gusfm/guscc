int main()
{
    int x = 42;
    int *p = &x;
    char *cp = (char *)p;
    int *ip = (int *)cp;
    return *ip;
}
