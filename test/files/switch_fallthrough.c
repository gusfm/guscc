int main()
{
    int x = 1;
    int r = 0;
    switch (x) {
    case 1:
        r = r + 1;
    case 2:
        r = r + 1;
    case 3:
        r = r + 1;
        break;
    }
    return r;
}
