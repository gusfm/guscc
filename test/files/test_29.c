int main()
{
    int x = 0;
    int i;
    for (i = 1; i <= 5; i = i + 1) {
        if (i == 3) break;
        x = x + 1;
    }
    return x;
}
