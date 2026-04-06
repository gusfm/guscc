int test_postinc(int *p) {
    p++;
    return *p;
}

int test_preinc(int *p) {
    ++p;
    return *p;
}

int test_predec(int *p) {
    --p;
    return *p;
}

int main() {
    int arr[4];
    arr[0] = 10;
    arr[1] = 30;
    arr[2] = 18;
    arr[3] = 99;
    int a = test_postinc(arr);
    int b = test_preinc(arr);
    int c = test_predec(arr + 3);
    return a + b - c;
}
