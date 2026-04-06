int advance(int *p) {
    p += 2;
    return *p;
}

int retreat(int *p) {
    p -= 1;
    return *p;
}

int main() {
    int arr[4];
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 99;
    int a = advance(arr);
    int b = retreat(arr + 2);
    return a + b - 8;
}
