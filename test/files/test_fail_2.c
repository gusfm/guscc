/* test_fail_2: for loop not yet implemented; guscc should exit non-zero */
int main()
{
    int x;
    for (x = 0; x < 10; x++)
        x = x + 1;
    return 0;
}
