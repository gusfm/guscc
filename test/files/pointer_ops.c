/* test_15: pointer parameter and dereference */
int deref(int *p)
{
    return *p;
}

int main()
{
    int x;
    x = 42;
    return deref(&x);
}
