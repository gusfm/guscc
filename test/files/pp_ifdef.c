#define FOO

int main()
{
#ifdef FOO
    return 42;
#else
    return 0;
#endif
}
