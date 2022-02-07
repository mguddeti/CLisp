#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

#ifdef _WIN32
#include <string.h>

static char buffer[2048];

char *readline(char *prompt)
{
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char *cpy = malloc(strlen(buffer) + 1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy) - 1] = '\0';
    return cpy;
}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

int main(int argc, char **argv)
{
    /* Create Some Parsers */
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Operator = mpc_new("operator");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
              "                                                     \
    number   : /-?[0-9]+/ ;                             \
    operator : '+' | '-' | '*' | '/' ;                  \
    expr     : <number> | '(' <operator> <expr>+ ')' ;  \
    lispy    : /^/ <operator> <expr>+ /$/ ;             \
  ",
              Number, Operator, Expr, Lispy);

    puts("CLisp Version 0.0.0.0.1");
    puts("Press Ctrl+c to Exit\n");

    while (1)
    {

        char *input = readline("clisp> ");

        /* Attempt to Parse the user Input */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r))
        {
            /* On Success Print the AST */
            mpc_ast_print(r.output);
            mpc_ast_delete(r.output);
            /* Load AST from output */
            mpc_ast_t *a = r.output;
            printf("Tag: %s\n", a->tag);
            printf("Contents: %s\n", a->contents);
            printf("Number of children: %i\n", a->children_num);
            mpc_ast_t *c0 = a->children[0];
            printf("First Child Tag: %s\n", c0->tag);
            printf("First Child Contents: %s\n", c0->contents);
            printf("First Child Number of children: %i\n",
                   c0->children_num);
        }
        else
        {
            /* Otherwise Print the Error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }

    /* Undefine and Delete our Parsers */
    mpc_cleanup(4, Number, Operator, Expr, Lispy);
    return 0;
}