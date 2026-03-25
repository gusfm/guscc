/* test_13: postfix ++ and -- */
int main()
{
    int x;
    int y;
    x = 41;
    x++;
    y = x;
    y--;
    return x + y - 41;
}
