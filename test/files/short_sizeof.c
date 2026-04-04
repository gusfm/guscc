int main()
{
    /* sizeof(short) == 2, sizeof(long) == 8, sizeof(short int) == 2 */
    /* 2 * 10 + 8 + 2 * 7 = 20 + 8 + 14 = 42 */
    return sizeof(short) * 10 + sizeof(long) + sizeof(short int) * 7;
}
