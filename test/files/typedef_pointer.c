typedef int *intptr;

int main()
{
    int a = 42;
    intptr p = &a;
    return *p;
}
