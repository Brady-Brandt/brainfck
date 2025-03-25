#include <stdint.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <stdarg.h>


/*
> 	Increment the data pointer by one (to point to the next cell to the right).
< 	Decrement the data pointer by one (to point to the next cell to the left).
+ 	Increment the byte at the data pointer by one.
- 	Decrement the byte at the data pointer by one.
. 	Output the byte at the data pointer.
, 	Accept one byte of input, storing its value in the byte at the data pointer.
[ 	If the byte at the data pointer is zero, then instead of moving the instruction pointer forward to the next command, jump it forward to the command after the matching ] command.
] 	If the byte at the data pointer is nonzero, then instead of moving the instruction pointer forward to the next command, jump it back to the command after the matching [ command.[a]

*/



typedef enum {
    TOK_INCREMENT_DP = '>',
    TOK_DECREMENT_DP = '<',
    TOK_INCREMENT_BYTE = '+',
    TOK_DECREMENT_BYTE = '-',
    TOK_PRINT = '.',
    TOK_INPUT = ',',
    TOK_JMP_ZERO = '[',
    TOK_JMP_NON_ZERO = ']',
    TOK_INVALID
} TokenType;



typedef struct { 
    TokenType type;
    union {
        uint8_t amount;
        uint32_t offset; // for JMP INSTRUCTIONS holds the index of its opening/closing counter part
    };
} Token;


typedef struct {
    Token* data;
    uint32_t capacity;
    uint32_t size;
} Tokens;



noreturn void fatal_error(const char* fmt, ...){
    printf("Error: ");
    va_list list;
    va_start(list, fmt);
    vfprintf(stderr, fmt, list);
    va_end(list);
    exit(EXIT_FAILURE);
}


Tokens tokens_init(){
    Tokens result;
    result.data = malloc(1024 * sizeof(Token));
    if(result.data == NULL) fatal_error("Out of Memory\n");
    result.capacity = 1024;
    result.size = 0;
    return result;
}


void tokens_append(Tokens* tokens, Token token){
    if(tokens->size == tokens->capacity){
        tokens->capacity *=2;
        tokens->data = realloc(tokens->data, tokens->capacity);
        if(tokens->data == NULL) fatal_error("Out of Memory\n");
    }
    tokens->data[tokens->size++] = token;
}

void tokens_delete(Tokens* tokens){
    free(tokens->data);
}




#define MAX_NESTED_VALUES 1000
typedef struct {
    uint32_t data[MAX_NESTED_VALUES];
    uint32_t size;
} Stack;


uint32_t stack_pop(Stack* stack){
    if(stack->size > 0){
        return stack->data[--stack->size];

    }
    return UINT32_MAX;
}



void stack_push(Stack* stack, uint32_t value){
    if(stack->size < MAX_NESTED_VALUES){
        stack->data[stack->size++] = value;
        return;
    }

    fatal_error("Exceed Max Nested Scopes %d\n", MAX_NESTED_VALUES);
}


//tries to convert repeated instructions into a number and the instruction
void check_continous_tokens(FILE* file, char current_char, Tokens* tokens){ 
    uint32_t count = 0;
    while(true){
        count++;

        fpos_t pos;
        fgetpos(file, &pos);
        char next_char = fgetc(file);
        fsetpos(file, &pos);

        if(next_char != current_char) break;
        fgetc(file);

    }
    tokens_append(tokens, (Token){current_char, .amount=count}); 

}


void usage(){
    fprintf(stderr, "./brainfck {input file}\n");
}





#define DEFAULT_PROGAM_SIZE 1000000
typedef struct {
    int8_t* cells;
    uint32_t dp;
} Program;


Program init_progam(uint32_t size){
    Program p;
    p.cells = calloc(size, 8);
    if(p.cells == NULL) fatal_error("Out of Memory\n");
    p.dp = 0;
    return p;
}

void delete_program(Program* p){
    free(p->cells); 
}


int main(int argc, char** argv){ 
    if(argc < 2){
        usage();
        return 1;
    }


    char* file_name = argv[1];
    FILE* stream = fopen(file_name, "r");
    if(stream == NULL){
        fprintf(stderr,"Failed to open: %s, ", file_name);
        perror("");
        return 1;
    }


    Tokens tokens = tokens_init();
    Stack bracket_stack = {0};
    uint32_t line_count = 0;

    while(true){
        char c = fgetc(stream);

        if(c == EOF) break;

        switch (c) {
            case '>':
            case '<':
            case '+':
            case '-':
            case '.':
            case ',':
                check_continous_tokens(stream, c, &tokens);
                break;
            case '[': 
                tokens_append(&tokens, (Token){c, .offset= 0});
                stack_push(&bracket_stack, tokens.size - 1);
                break;                     
            case ']':
                {
                if(bracket_stack.size < 1) fatal_error("Mismatched Brackets on Line %d\n", line_count); 
                 uint32_t opening_index = stack_pop(&bracket_stack);
                 Token end_bracket = {c, .offset = opening_index};
                 tokens_append(&tokens, end_bracket);

                 //have the offset of the opening brace point to the end_bracket index
                 tokens.data[opening_index].offset = tokens.size - 1;

                 break;
                }
            case '\n':
                line_count++;
                break;
            default:
                break;
        }
    }


    if(bracket_stack.size != 0) fatal_error("No Final Closing Bracket\n");



    Program p = init_progam(DEFAULT_PROGAM_SIZE);
    
    uint32_t ip = 0;
    while(ip < tokens.size){
        Token tok = tokens.data[ip];
        switch (tok.type) { 
            case '>': 
                p.dp += tok.amount;
                break;
            case '<':
                p.dp -= tok.amount;
                break;
            case '+':
                p.cells[p.dp] += tok.amount;
                break;
            case '-':
                p.cells[p.dp] -= tok.amount;
                break;
            case '.':
                fputc(p.cells[p.dp], stdout);
                break;
            case ',':
                p.cells[p.dp] = fgetc(stdin);
                break;
            case '[':
                if(p.cells[p.dp] == 0){ 
                    ip = tok.offset;
                }
                break;
            case ']':
                if(p.cells[p.dp] != 0){
                    ip = tok.offset;
                }
                break;
            default:     
                break;
        }
        ip++;

    }

    delete_program(&p);
    tokens_delete(&tokens);
}
