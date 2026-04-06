int add_offset(int *p) {
    return *(p + 1) + *(p + 2);
}

int main() {
    int arr[3];
    arr[0] = 1;
    arr[1] = 10;
    arr[2] = 32;
    return add_offset(arr);
}
