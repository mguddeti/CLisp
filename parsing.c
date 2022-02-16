#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"

#ifdef _WIN32
#include <string.h>

static char buffer[2048];

char *readline(char *prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char *cpy = malloc(strlen(buffer) + 1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy) - 1] = '\0';
    return cpy;
}

#else
#include <editline/history.h>
#include <editline/readline.h>
#endif
int number_of_nodes(mpc_ast_t *t);
int nodal_value(mpc_ast_t *t);
long eval(mpc_ast_t *t);
long eval_op(long x, char *op, long y);

int main(int argc, char **argv) {
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

    while (1) {
        char *input = readline("clisp> ");

        /* Attempt to Parse the user Input */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            /* On Success Print the AST */
            mpc_ast_delete(r.output);
            /* Load AST from output */
            mpc_ast_t *a = r.output;
            long result = eval(a);
            printf("%li\n", result);
            printf("theirs: %d mine: %d\n", number_of_nodes(a), nodal_value(a));
            mpc_ast_delete(r.output);
        } else {
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

int number_of_nodes(mpc_ast_t *t) {
    if (t->children_num == 0) {
        return 1;
    }
    if (t->children_num >= 1) {
        int total = 1;
        for (int i = 0; i < t->children_num; i++) {
            total = total + number_of_nodes(t->children[i]);
        }
        return total;
    }
    return 0;
}

int nodal_value(mpc_ast_t *t) {
    if (t->children_num == 0) {
        return 1;
    } else {
        int total = 1;
        for (int i = 0; i < t->children_num; i++) {
            total += nodal_value(t->children[i]);
        }
        return total;
    }
}

long eval(mpc_ast_t *t) {
    /* If tagged as number return it directly. */
    if (strstr(t->tag, "number")) {
        return atoi(t->contents);
    }

    /* The operator is always second child. */
    char *op = t->children[1]->contents;

    /* We store the third child in `x` */
    long x = eval(t->children[2]);
    /* Iterate the remaining children and combining. */
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

/* Use operator string to see which operation to perform */
long eval_op(long x, char *op, long y) {
    if (strcmp(op, "+") == 0) {
        return x + y;
    }
    if (strcmp(op, "-") == 0) {
        return x - y;
    }
    if (strcmp(op, "*") == 0) {
        return x * y;
    }
    if (strcmp(op, "/") == 0) {
        return x / y;
    }
    return 0;
}

/* Declare New lval Struct */
typedef struct
{
    int type;
    long num;
    int err;
} lval;

/* Create Enumeration of Possible lval Types */
enum {
    LVAL_NUM,
    LVAL_ERR
};

/* Create Enumeration of Possible Error Types */
enum {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM
};

/* Create a new number type lval */
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

/* Create a new error type lval */
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

/* Print an "lval" */
void lval_print(lval v) {
    switch (v.type) {
        /* In the case the type is a number print it */
        /* Then 'break' out of the switch. */
        case LVAL_NUM:
            printf("%li", v.num);
            break;

        /* In the case the type is an error */
        case LVAL_ERR:
            /* Check what type of error it is and print it */
            if (v.err == LERR_DIV_ZERO) {
                printf("Error: Division By Zero!");
            }
            if (v.err == LERR_BAD_OP) {
                printf("Error: Invalid Operator!");
            }
            if (v.err == LERR_BAD_NUM) {
                printf("Error: Invalid Number!");
            }
            break;
    }
}

/* Print an "lval" followed by a newline */
void lval_println(lval v) {
    lval_print(v);
    putchar('\n');
}

lval eval_op(lval x, char *op, lval y) {
    /* If either value is an error return it */
    if (x.type == LVAL_ERR) {
        return x;
    }
    if (y.type == LVAL_ERR) {
        return y;
    }

    /* Otherwise do maths on the number values */
    if (strcmp(op, "+") == 0) {
        return lval_num(x.num + y.num);
    }
    if (strcmp(op, "-") == 0) {
        return lval_num(x.num - y.num);
    }
    if (strcmp(op, "*") == 0) {
        return lval_num(x.num * y.num);
    }
    if (strcmp(op, "/") == 0) {
        /* If second operand is zero return error */
        return y.num == 0
                   ? lval_err(LERR_DIV_ZERO)
                   : lval_num(x.num / y.num);
    }

    return lval_err(LERR_BAD_OP);
}