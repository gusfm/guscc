int main()
{
    char s1[20] = "Test string 1";
    char *s2 = "Test string 2";
    return (int)(s2[0] + s2[12] - (s1[0] + s1[12]));
}
