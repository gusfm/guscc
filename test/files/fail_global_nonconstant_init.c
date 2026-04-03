/* error: non-constant global initializer */
int f()
{
    return 1;
}

int g = f();

int main()
{
    return g;
}
