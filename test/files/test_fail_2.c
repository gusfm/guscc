/* test_fail_2: while statement not yet implemented; guscc should exit non-zero */
int main()
{
    while (1)
        return 42;
    return 0;
}
