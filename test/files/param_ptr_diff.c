int distance(int *p, int *q) {
    return q - p;
}

int main() {
    int arr[50];
    arr[0] = 0;
    return distance(arr, arr + 42);
}
