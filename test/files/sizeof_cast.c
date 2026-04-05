int main()
{
    int x = 1;
    return sizeof((char)x) + sizeof((short)x) + sizeof((long)x);
}
