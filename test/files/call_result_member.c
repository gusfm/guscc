typedef struct {
    int value;
    int extra;
} pair_t;

pair_t *make_pair() {
    static pair_t p;
    p.value = 30;
    p.extra = 12;
    return &p;
}

int main()
{
    if (make_pair()->value != 30) return 1;
    if (make_pair()->extra != 12) return 2;
    return make_pair()->value + make_pair()->extra;
}
