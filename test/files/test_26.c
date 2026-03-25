int main()
{
    int x = 0;
    int i = 0;
    do {
        i = i + 1;
        if (i == 3)
            continue;
        x = x + 1;
    } while (i < 5);
    return x;
}
