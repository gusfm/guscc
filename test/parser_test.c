#include <stdio.h>
#include "parser.h"
#include "ut.h"
#include "utils.h"

static int parser_test_1(void)
{
    parser_t parser;
    char *src = "int main(void)\n{\n\treturn 42;\n}\n";
    FILE *stream = fmemopen(src, sizeof(src), "w+");
    ASSERT(stream != NULL);
    ASSERT(parser_init(&parser, stream) == 0);
    fclose(stream);
    return 0;
}

void parser_test(void)
{
    ut_run(parser_test_1);
}
