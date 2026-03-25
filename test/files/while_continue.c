int main()
{
    int x = 0;
    int i = 0;
    while (i < 6) {
        i = i + 1;
        if (i == 3)
            continue;
        x = x + 1;
    }
    return x;
}
