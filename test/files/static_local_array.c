int get(int i)
{
    static int arr[3] = {10, 20, 12};
    return arr[i];
}

int main()
{
    return get(0) + get(1) + get(2);
}
