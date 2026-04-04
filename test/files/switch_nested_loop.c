int main()
{
    int sum = 0;
    int i;
    for (i = 0; i < 5; i = i + 1) {
        switch (i) {
        case 0:
            sum = sum + 1;
            break;
        case 1:
            sum = sum + 2;
            break;
        default:
            sum = sum + 0;
            break;
        }
    }
    return sum;
}
