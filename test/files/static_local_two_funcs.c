int f()
{
    static int x = 10;
    x = x + 1;
    return x;
}

int g()
{
    static int x = 20;
    x = x + 1;
    return x;
}

int main()
{
    f();
    g();
    /* f() returns 12, g() returns 22; 12 + 22 + 8 = 42 */
    return f() + g() + 8;
}
