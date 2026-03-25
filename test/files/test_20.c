/* test_20: while with braces and function call */
int double_it(int n)
{
    return n + n;
}

int main()
{
    int x;
    x = 1;
    while (x < 32) {
        x = double_it(x);
    }
    return x;
}
