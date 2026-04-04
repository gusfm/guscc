int variadic_func(int, ...);

int variadic_func(int a, ...)
{
    return a;
}

int main()
{
    return variadic_func(10, 20, 30);
}
