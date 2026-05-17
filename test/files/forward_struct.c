typedef struct node node_t;

struct node {
    int value;
    node_t *next;
};

int main()
{
    node_t tail;
    tail.value = 32;
    tail.next = 0;
    node_t head;
    head.value = 10;
    head.next = &tail;
    return head.value + head.next->value;
}
