void func(char *str, int val)
{
    printf("%s:%d\n", str, val);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "test\n");
        return -1;
    }
    func(argv[1], 2);
    return 0;
}
