int get()
{
    static int x;
    return x;
}

int main()
{
    return get() + 42;
}
