int find_value(int *start, int *end, int val) {
    int *p = start;
    while (p < end) {
        if (*p == val)
            return 1;
        p++;
    }
    return 0;
}

int main() {
    int arr[5];
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;
    arr[4] = 50;
    int found = find_value(arr, arr + 5, 30);
    int not_found = find_value(arr, arr + 5, 99);
    return found * 42 + not_found;
}
