typedef unsigned long size_t;

size_t add(size_t a, size_t b)
{
    return a + b;
}

int main()
{
    size_t x = 20;
    size_t y = 22;
    return add(x, y);
}
