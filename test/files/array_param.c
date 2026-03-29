int sum_first(int *arr)
{
    return arr[0];
}

int main()
{
    int a[3];
    a[0] = 42;
    a[1] = 10;
    a[2] = 20;
    return sum_first(a);
}
