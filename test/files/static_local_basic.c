int counter()
{
    static int n = 0;
    n = n + 1;
    return n;
}

int main()
{
    counter();
    counter();
    return counter();
}
