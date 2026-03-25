int main()
{
    int x = 0;
    int i = 0;
    while (i < 3) {
        i = i + 1;
        int j = 0;
        while (j < 3) {
            j = j + 1;
            if (j == 2)
                break;
            x = x + 1;
        }
    }
    return x;
}
