struct mixed { char a; int b; };

int main() {
    struct mixed m;
    m.a = 10;
    m.b = 32;
    return m.a + m.b;
}
