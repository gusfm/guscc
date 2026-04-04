enum Op { ADD, SUB, MUL };

int main()
{
    enum Op op = MUL;
    int r = 0;
    switch (op) {
    case ADD:
        r = 1;
        break;
    case SUB:
        r = 2;
        break;
    case MUL:
        r = 3;
        break;
    }
    return r;
}
