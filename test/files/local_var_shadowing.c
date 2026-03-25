/* Local variable shadowing */
int main()
{
    int x = 42;
    if (x > 0) {
        int x = 10;
        return x;
    }
    return x;
}
