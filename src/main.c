#include <stdint.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <sys/unistd.h>


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



const char* OUT_OF_MEM = "Out of Memory\n";
const char* NUMBER_WARNING = "Numbers front of Instruction %c will be ignored\nLine: %d -> %d%c\n";

noreturn void fatal_error(const char* fmt, ...){
    fprintf(stderr,"Error: ");
    va_list list;
    va_start(list, fmt);
    vfprintf(stderr, fmt, list);
    va_end(list);
    exit(EXIT_FAILURE);
}


void warning(const char* fmt, ...){
    fprintf(stderr,"Warning: ");
    va_list list;
    va_start(list, fmt);
    vfprintf(stderr, fmt, list);
    va_end(list);
}


Tokens tokens_init(){
    Tokens result;
    result.data = malloc(1024 * sizeof(Token));
    if(result.data == NULL) fatal_error(OUT_OF_MEM);
    result.capacity = 1024;
    result.size = 0;
    return result;
}


void tokens_append(Tokens* tokens, Token token){
    if(tokens->size == tokens->capacity){
        tokens->capacity *=2;
        tokens->data = realloc(tokens->data, tokens->capacity);
        if(tokens->data == NULL) fatal_error(OUT_OF_MEM);
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
void check_continous_tokens(FILE* file, char current_char, Tokens* tokens, uint32_t number, uint32_t line){ 
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

    if(number != 0) {
        number -= 1;
    }
    uint32_t total = count + number;

    if(current_char == ',' && count > 1){
        warning("Redunant Use of Instruction ','\nLine %d: Attempting to take user input %d times without incrementing the data pointer\n", line, total);
        total = 0;
    }


    tokens_append(tokens, (Token){current_char, .amount=total}); 

}



#define DEFAULT_PROGAM_SIZE 1000000

void interpret_progam(uint32_t size, Tokens* tokens){
    int8_t* cells = calloc(size, 8);
    if(cells == NULL) fatal_error(OUT_OF_MEM);

    uint32_t dp = 0;    
    uint32_t ip = 0;


    while(ip < tokens->size){
        Token tok = tokens->data[ip];
        switch (tok.type) { 
            case '>': 
                dp += tok.amount;
                break;
            case '<':
                dp -= tok.amount;
                break;
            case '+':
                cells[dp] += tok.amount;
                break;
            case '-':
                cells[dp] -= tok.amount;
                break;
            case '.':
                for(int i = 0; i < tok.amount; i++){
                    fputc(cells[dp], stdout);
                }
                break;
            case ',':
                cells[dp] = fgetc(stdin);
                break;
            case '[':
                if(cells[dp] == 0){ 
                    ip = tok.offset;
                }
                break;
            case ']':
                if(cells[dp] != 0){
                    ip = tok.offset;
                }
                break;
            default:     
                break;
        }
        ip++;

    }
    free(cells);
}





#if defined(_WIN64)
    void compile_progam(const char* file_name, const char* output_file, uint32_t size, Tokens* tokens){
        fatal_error("The compiler is not supported for windows yet\n");
    } 
#elif defined(__APPLE__) && defined(__MACH__)  || defined(__linux__)
    void compile_progam(const char* file_name, const char* output_file, uint32_t size, Tokens* tokens){
        size_t file_name_len = strlen(file_name) + 5;
        char assembly_file[file_name_len];

        char object_file[file_name_len + 7];


        sprintf(assembly_file,"%s.asm", file_name);
        sprintf(object_file, "%s.o", assembly_file);

     
        FILE* asm_stream = fopen(assembly_file, "w");

        if(asm_stream == NULL) fatal_error("Failed to create assembly file\n");


        #if defined (__APPLE__) && defined (__MACH__)
            fprintf(asm_stream, "global start\nsection .bss\ncells: resb %d\nsection .text\n", size);
            fprintf(asm_stream, "print:\nmov rax,0x2000004\nmov rdi, 1\nmov rdx,1\nsyscall\nret\n");
            fprintf(asm_stream, "input:\nmov rax,0x20000000\nmov rdi, 0\nmov rdx,1\nsyscall\nret\n");
            fprintf(asm_stream, "start:\nmov r13,0\n");
        #else
            fprintf(asm_stream, "global _start\nsection .bss\ncells: resb %d\nsection .text\n", size);
            fprintf(asm_stream, "print:\nmov rax,1\nmov rdi, 1\nmov rdx,1\nsyscall\nret\n");
            fprintf(asm_stream, "input:\nmov rax, 0\nmov rdi, 0\nmov rdx, 1\nsyscall\nret\n");
            fprintf(asm_stream, "_start:\nmov r13,0\n");
        #endif

 
        for(uint32_t i = 0; i < tokens->size; i++){
            Token tok = tokens->data[i];
            switch (tok.type) { 
                case '>':
                    fprintf(asm_stream, "add r13, %d\n", tok.amount);
                    break;
                case '<':
                    fprintf(asm_stream, "sub r13, %d\n", tok.amount);
                    break;
                case '+':
                    fprintf(asm_stream, "mov al, byte [cells + r13]\nadd al, %d\nmov byte [cells + r13], al\n", tok.amount);
                    break;
                case '-':
                    fprintf(asm_stream, "mov al, byte [cells + r13]\nsub al, %d\nmov byte [cells + r13], al\n", tok.amount);
                    break;
                case '.':
                    for(int i = 0; i < tok.amount; i++){
                        fprintf(asm_stream, "lea rsi, [cells + r13]\ncall print\n");
                    }
                    break;
                case ',':
                    fprintf(asm_stream, "lea rsi, [cells + r13]\ncall input\n");
                    break;
                case '[':
                    fprintf(asm_stream, "cmp byte [cells + r13], 0\nje label%d\nlabel%d:\n", tok.offset, i);
                    break;
                case ']':
                    fprintf(asm_stream, "cmp byte [cells + r13], 0\njne label%d\nlabel%d:\n", tok.offset, i);
                    break;
                default:     
                    break;
            }


        }

        
        //exit syscall
        fprintf(asm_stream,"mov rax, 60\nxor rdi,rdi\nsyscall\n");
        fclose(asm_stream);

        #define BUF_SIZE 512
        char cmd[BUF_SIZE] = {0};


        size_t cmd_len = snprintf(cmd, BUF_SIZE, "nasm -f elf64 %s -o %s", assembly_file, object_file);
        int ret = system(cmd);
        if(ret != 0) fatal_error("Failed to execute nasm\n");

        memset(cmd,0, cmd_len + 1);


        snprintf(cmd, BUF_SIZE, "ld -s -o %s %s", output_file, object_file);

        ret = system(cmd);
        if(ret != 0) fatal_error("Failed to execute ld\n");


        ret = remove(assembly_file);
        if(ret != 0) fatal_error("Failed to cleanup: %s\n", assembly_file);
        ret = remove(object_file);
        if(ret != 0) fatal_error("Failed to cleanup: %s\n", object_file);


    }



#else
    void compile_progam(const char* file_name, const char* output_file, uint32_t size, Tokens* tokens){
        fatal_error("The compiler is not supported for this platform\n");
    }
#endif


void usage(){
    fprintf(stderr, "./brainfck {input file}\n");
    fprintf(stderr, "Flags: \n");
    fprintf(stderr, "-c, Compiles the progam (Redunant if using -o)\n");
    fprintf(stderr, "Options: \n");
    fprintf(stderr, "-o {output file}, Compiles the progam into an executable named {output_name}\n");
}





int main(int argc, char** argv){ 
    if(argc < 2){
        fprintf(stderr, "./brainfck {input file}\n");
        return 1;
    }

    char* output_name = "a.out";
    char* file_name = NULL;


    bool interpret = true;

    int arg_index = 1;
    while(arg_index < argc){
        char* arg = argv[arg_index];
        
        if(strcmp(arg, "-o") == 0){
            interpret = false;
            if(arg_index + 1 == argc){
                warning("No output file provided but -o flag was passed. Defaulting to %s\n", output_name);
                break;
            }
            arg_index++;
            output_name = argv[arg_index]; 

        } else if(strcmp(arg, "-c") == 0){
            interpret = false;

        } else if(strcmp(arg, "--help") == 0){
            usage();
            return 1;
        } else{
            file_name = arg;
        }

        arg_index++;
    }

    if(file_name == NULL) fatal_error("No input file\n");
     

    FILE* stream = fopen(file_name, "r");
    if(stream == NULL){
        fprintf(stderr,"Failed to open: %s\n", file_name);
        perror("");
        return 1;
    }


    Tokens tokens = tokens_init();
    Stack bracket_stack = {0};
    uint32_t line_count = 1;


    uint32_t number = 0;

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
                check_continous_tokens(stream, c, &tokens, number, line_count);
                number = 0;
                break;
            case '[':
                if(number != 0) warning(NUMBER_WARNING, c, line_count, number,c, number);
                number = 0;
                tokens_append(&tokens, (Token){c, .offset= 0});
                stack_push(&bracket_stack, tokens.size - 1);
                break;                     
            case ']':
                {
                if(number != 0) warning(NUMBER_WARNING, c, line_count, number,c, number);
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
                if(isdigit(c)){
                    while(true){
                        number = number * 10 + (c - 48);
                        fpos_t pos;
                        fgetpos(stream, &pos);
                        char next_char = fgetc(stream);
                        fsetpos(stream, &pos);

                        if(!isdigit(next_char)) break;
                        c = fgetc(stream);

                    }
                }
                break;
        }
    }


    if(bracket_stack.size != 0) fatal_error("No Final Closing Bracket\n");

    if(interpret){
        interpret_progam(DEFAULT_PROGAM_SIZE, &tokens); 
    }
    else{
        compile_progam(file_name, output_name, DEFAULT_PROGAM_SIZE, &tokens);
    } 

    tokens_delete(&tokens);
}
