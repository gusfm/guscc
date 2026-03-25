/* test_18: if/else with braces and function call in condition */
int square(int n)
{
    return n * n;
}

int main()
{
    int x;
    if (square(3) == 9) {
        x = 5;
    } else {
        x = 0;
    }
    return x;
}
