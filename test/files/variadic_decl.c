int variadic_func(int a, ...)
{
    return a;
}

int main()
{
    return variadic_func(42, 1, 2, 3);
}
