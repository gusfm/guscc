/* test_14: nested function calls and parameter passing */
int double_val(int n)
{
    return n + n;
}

int add(int a, int b)
{
    return a + b;
}

int main()
{
    return add(double_val(10), double_val(11));
}
