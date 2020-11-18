#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define XSTR(s) STR(s)
#define STR(s) #s

#define CAP 65536

#define SUCCESS 0
#define UNMATCHED_LEFT 1
#define TOO_LONG 2
#define UNMATCHED_RIGHT 3

#define INPUT_FAIL 1
#define OUTPUT_FAIL 2

#define HELP_MSG \
    "Usage: %s [-h | script]\n" \
    "Options and arguments:\n" \
    "  script  The script file to run.\n" \
    "          If omitted the script will be read from stdin.\n" \
    "  -h      Display this help message and quit.\n"

typedef struct {
    char op;
    short param;
} command;

typedef struct stack {
    unsigned short val;
    struct stack *next;
} *stack;

static stack tmp;

#define PUSH(VAL, ST) { tmp = malloc(sizeof(struct stack)); tmp->val = VAL; tmp->next = ST; ST = tmp; }

#define POP(ST) { tmp = ST->next; free(ST); ST = tmp; }

#define DESTROY(ST) { while (ST) POP(ST) }

static command program[CAP + 1] = {};

char compile(FILE *stream, unsigned *len) {
    unsigned ptr = 0;
    stack loops = NULL;
    char com = fgetc(stream);

#   define MOD_true ++
#   define MOD_false --
#   define INIT_true
#   define INIT_false -

#   define CHECK_CAP(BLOCK) \
    if (ptr <= CAP) BLOCK \
    else { \
        DESTROY(loops) \
        return TOO_LONG; \
    } \
    break;

#   define HANDLE_MOD(YES_NO, TYPE) \
        if (program[ptr].op == TYPE) { \
            program[ptr].param MOD_##YES_NO;\
            if (!program[ptr].param) \
                ptr--; \
        } else CHECK_CAP({ \
            program[++ptr].op = TYPE; \
            program[ptr].param = INIT_##YES_NO 1; \
        })

    for (;;) {
        
        if (com == EOF) {
            if (loops) {
                DESTROY(loops)
                return UNMATCHED_LEFT;
            } else {
                *len = ptr;
                return SUCCESS;
            }
        }

        switch (com) {
            case '+':
                HANDLE_MOD(true, '+')
            case '-':
                HANDLE_MOD(false, '+')
            case '>':
                HANDLE_MOD(true, '>')
            case '<':
                HANDLE_MOD(false, '>')
            case ',':
                HANDLE_MOD(true, ',')
            case '.':
                HANDLE_MOD(true, '.')
            case '[':
                CHECK_CAP({ \
                    program[++ptr].op = '['; \
                    PUSH(ptr, loops) \
                })
            case ']':
                if (!loops)
                    return UNMATCHED_RIGHT;
                CHECK_CAP({ \
                    program[++ptr].op = ']'; \
                    program[ptr].param = loops->val + 1; \
                    program[loops->val].param = ptr + 1; \
                    POP(loops) \
                })
        }

        com = fgetc(stream);
    }
}

char exec(unsigned len) {
    wchar_t data[CAP] = {};
    unsigned short data_ptr = 0;
    unsigned prog_ptr = 1;
    short repeat;
    wint_t c;

    while (prog_ptr <= len) {
        switch (program[prog_ptr].op) {
            case '+':
                data[data_ptr] += program[prog_ptr++].param;
                break;
            case '>':
                data_ptr += program[prog_ptr++].param;
                break;
            case ',':
                repeat = program[prog_ptr++].param;
                for (int i = 0; i < repeat; i++) {
                    c = getwchar();
                    if (c == WEOF)
                        return INPUT_FAIL;
                    else
                        data[data_ptr] = c;
                }
                break;
            case '.':
                repeat = program[prog_ptr++].param;
                for (int i = 0; i < repeat; i++) {
                    if (putwchar(data[data_ptr]) == WEOF)
                        return OUTPUT_FAIL;
                }
                break;
            case '[':
                if (!data[data_ptr])
                    prog_ptr = program[prog_ptr].param;
                else
                    prog_ptr++;
                break;
            case ']':
                if (data[data_ptr])
                    prog_ptr = program[prog_ptr].param;
                else
                    prog_ptr++;
                break;
        }
    }

    return SUCCESS;
}

int main(int argc, char *argv[]) {
    FILE *stream;
    
    if (argc > 1) {
        if (strcmp(argv[1], "-h")) {
            stream = fopen(argv[1], "r");
            if (!stream) {
                fprintf(stderr, "Failed to read file %s.\n", argv[1]);
                return EXIT_FAILURE;
            }
        } else {
            printf(HELP_MSG, argv[0]);
            return EXIT_SUCCESS;
        }
    } else
        stream = stdin;

    unsigned len;
    
#   define FAIL_MSG_UNMATCHED_LEFT "Unmatched left brackets."
#   define FAIL_MSG_TOO_LONG "Program too long (" XSTR(CAP) " commands maximum)."
#   define FAIL_MSG_UNMATCHED_RIGHT "Unmatched right brackets."

#   define HANDLE_COMP_RES(CASE) \
        case CASE: \
            fputs("Compilation failed: " FAIL_MSG_##CASE "\n", stderr); \
            return EXIT_FAILURE;

    switch (compile(stream, &len)) {
        HANDLE_COMP_RES(UNMATCHED_LEFT)
        HANDLE_COMP_RES(TOO_LONG)
        HANDLE_COMP_RES(UNMATCHED_RIGHT)
    }

    if (stream != stdin)
        fclose(stream);
/*
    fprintf(stderr, "%u\n", len);
    for (unsigned ptr = 0; ptr < len; ptr++)
        fprintf(stderr, "%c\t%hd\n", program[ptr].op, program[ptr].param);
*/
#   define FAIL_MSG_INPUT_FAIL "Input error."
#   define FAIL_MSG_OUTPUT_FAIL "Output error."

#   define HANDLE_EXEC_RES(CASE) \
        case CASE: \
            fputs("Execution failed: " FAIL_MSG_##CASE "\n", stderr); \
            return EXIT_FAILURE;
    
    switch (exec(len)) {
        HANDLE_EXEC_RES(INPUT_FAIL)
        HANDLE_EXEC_RES(OUTPUT_FAIL)
    }

    return EXIT_SUCCESS;
}