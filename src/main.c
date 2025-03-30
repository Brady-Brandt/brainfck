#include <stdint.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>


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
        size_t file_name_len = strlen(file_name) + 5;
        char assembly_file[file_name_len];

        char object_file[file_name_len + 7];


        sprintf(assembly_file,"%s.asm", file_name);
        sprintf(object_file, "%s.o", assembly_file);

     
        FILE* asm_stream = fopen(assembly_file, "w");


        fprintf(asm_stream, "global main\nextern fputc\nextern exit\nextern fgetc\nextern __acrt_iob_func\n");
        fprintf(asm_stream, "section .bss\ncells: resb %d\nsection .text\n main:\n", size);
        //move stdout pointer into r12
        fprintf(asm_stream, "mov rcx, 1\ncall __acrt_iob_func\nmov r12, rax\n");
        //move stdin pointer into r13
        fprintf(asm_stream, "mov rcx, 0\ncall __acrt_iob_func\nmov r13, rax\n");
        //load dp in r14 and cells pointer in r15
        fprintf(asm_stream, "mov r14,0\nlea r15, [rel cells]\n");

        for(uint32_t i = 0; i < tokens->size; i++){
                Token tok = tokens->data[i];
                switch (tok.type) { 
                    case '>':
                        fprintf(asm_stream, "add r14, %d\n", tok.amount);
                        break;
                    case '<':
                        fprintf(asm_stream, "sub r14, %d\n", tok.amount);
                        break;
                    case '+':
                        fprintf(asm_stream,"add [r15 + r14], byte %d\n", tok.amount);
                        break;
                    case '-':
                        fprintf(asm_stream,"sub [r15 + r14], byte %d\n", tok.amount);
                        break;
                    case '.':
                        for(int i = 0; i < tok.amount; i++){
                            fprintf(asm_stream, "mov rcx, [r15 + r14]\nmov rdx, r12\ncall fputc\n");
                        }
                        break;
                    case ',':
                        fprintf(asm_stream, "mov rcx, r13\ncall fgetc\nmov [r15 + r14], rax\n");
                        break;
                    case '[':
                        fprintf(asm_stream, "cmp byte [r15 + r14], 0\nje label%d\nlabel%d:\n", tok.offset, i);
                        break;
                    case ']':
                        fprintf(asm_stream, "cmp byte [r15 + r14], 0\njne label%d\nlabel%d:\n", tok.offset, i);
                        break;
                    default:     
                        break;
                }
            }


        fprintf(asm_stream,"mov rcx, 0\ncall exit");
        fclose(asm_stream);
        #define BUF_SIZE 512
        char cmd[BUF_SIZE] = {0};
        int ret;
        size_t cmd_len;

        cmd_len = snprintf(cmd, BUF_SIZE, "nasm -f win64 %s -o %s", assembly_file, object_file);
        ret = system(cmd);
        if(ret != 0) fatal_error("Failed to execute nasm\n");

        memset(cmd,0, cmd_len + 1);
        snprintf(cmd, BUF_SIZE, "gcc -o %s %s", output_file, object_file);

        ret = system(cmd);
        if(ret != 0) fatal_error("Failed to execute gcc\n");

        ret = remove(assembly_file);
        if(ret != 0) fatal_error("Failed to cleanup: %s\n", assembly_file);
        ret = remove(object_file);
        if(ret != 0) fatal_error("Failed to cleanup: %s\n", object_file);
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

        #define BUF_SIZE 512
        char cmd[BUF_SIZE] = {0};
        int ret;
        size_t cmd_len;
 

        #if defined(__x86_64__) || defined(_M_X64)
            #if defined (__APPLE__) && defined (__MACH__)
                int exit_syscall = 0x2000001;
                int print_syscall = 0x2000004;
                int input_syscall = 0x2000000;
                const char* obj_type = "macho64";
            #else
                int exit_syscall = 60;
                int print_syscall = 1;
                int input_syscall = 0;
                const char* obj_type = "elf64";
            #endif

            fprintf(asm_stream, "global _start\nsection .bss\ncells: resb %d\nsection .text\n", size);
            fprintf(asm_stream, "print:\nmov rax,%d\nmov rdi, 1\nmov rdx,1\nsyscall\nret\n", print_syscall);
            fprintf(asm_stream, "input:\nmov rax, %d\nmov rdi, 0\nmov rdx, 1\nsyscall\nret\n", input_syscall);
            fprintf(asm_stream, "_start:\nmov r13,0\nlea r12, [rel cells]\n");

     
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
                        fprintf(asm_stream,"add [r12 + r13], byte %d\n", tok.amount);
                        break;
                    case '-':
                        fprintf(asm_stream,"sub [r12 + r13], byte %d\n", tok.amount);
                        break;
                    case '.':
                        for(int i = 0; i < tok.amount; i++){
                            fprintf(asm_stream, "lea rsi, [r12 + r13]\ncall print\n");
                        }
                        break;
                    case ',':
                        fprintf(asm_stream, "lea rsi, [r12 + r13]\ncall input\n");
                        break;
                    case '[':
                        fprintf(asm_stream, "cmp byte [r12 + r13], 0\nje label%d\nlabel%d:\n", tok.offset, i);
                        break;
                    case ']':
                        fprintf(asm_stream, "cmp byte [r12 + r13], 0\njne label%d\nlabel%d:\n", tok.offset, i);
                        break;
                    default:     
                        break;
                }
            } 
            //exit syscall
            fprintf(asm_stream,"mov rax, %d\nxor rdi,rdi\nsyscall\n", exit_syscall);
            fclose(asm_stream);

            cmd_len = snprintf(cmd, BUF_SIZE, "nasm -f %s %s -o %s", obj_type, assembly_file, object_file);
            ret = system(cmd);
            if(ret != 0) fatal_error("Failed to execute nasm\n");

        #elif defined(__aarch64__) || defined(_M_ARM64)   
            #if defined(__linux__)
                //x19 data value, x20 dp
                fprintf(asm_stream, ".global _start\n.bss\ncells: .fill %d,1\n.text\n", size);
                fprintf(asm_stream, "print:\nmov X8,#64\nmov X0, #1\nmov X2,1\nsvc 0\nret\n");
                fprintf(asm_stream, "input:\nmov X8,#63\nmov X0, #0\nmov X2,1\nsvc 0\nret\n");
                fprintf(asm_stream, "_start:\nmov X19,0\nldr X20, =cells\n");
                for(uint32_t i = 0; i < tokens->size; i++){
                    Token tok = tokens->data[i];
                    switch (tok.type) { 
                        case '>':
                            fprintf(asm_stream, "add X20, X20, #%d\n", tok.amount);
                            break;
                        case '<':
                            fprintf(asm_stream, "sub x20, X20, #%d\n", tok.amount);
                            break;
                        case '+':
                            fprintf(asm_stream,"ldrb w19, [X20]\nadd w19, w19,%d\nstrb w19, [X20]\n", tok.amount);
                            break;
                        case '-':
                            fprintf(asm_stream,"ldrb w19, [X20]\nsub w19, w19,%d\nstrb w19, [X20]\n", tok.amount);
                            break;
                        case '.':
                            for(int i = 0; i < tok.amount; i++){
                                fprintf(asm_stream, "mov X1, X20\nbl print\n");
                            }
                            break;
                        case ',':
                            fprintf(asm_stream, "mov X1, X20\nbl input\n");
                            break;
                        case '[':
                            fprintf(asm_stream, "ldrb w19, [X20]\ncmp w19, #0\nb.eq label%d\nlabel%d:\n", tok.offset, i);
                            break;
                        case ']':
                            fprintf(asm_stream, "ldrb w19, [X20]\ncmp w19, #0\nb.ne label%d\nlabel%d:\n", tok.offset, i);
                            break;
                        default:     
                            break;
                    }
                }
           
                //exit
                fprintf(asm_stream, "mov X0, #0\nmov X8, #93\nsvc 0\n");

                fclose(asm_stream);
                cmd_len = snprintf(cmd, BUF_SIZE, "as %s -o %s",assembly_file, object_file);
                ret = system(cmd);
                if(ret != 0) fatal_error("Failed to execute gnu assembler\n");
            #else  
                fclose(asm_stream);
                fatal_error("Compiler not supported for ARM based Macos\n");
            #endif
        #else
            fclose(asm_stream);
            fatal_error("Compiler not supported for this CPU architecture\n");
        #endif



        memset(cmd,0, cmd_len + 1);
        snprintf(cmd, BUF_SIZE, "ld -e _start -static -o %s %s", output_file, object_file);

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

        if(feof(stream)) break;

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
