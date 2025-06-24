#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// --- Configuration Constants ---
#define MAX_VARS 100
#define MAX_VAR_NAME 32
#define MAX_STRINGS 100
#define MAX_ARG_LEN 128
#define MAX_LINES 1000
#define MAX_LINE_LEN 256

// --- Compiler State and Symbol Tables ---
typedef struct { char name[MAX_VAR_NAME]; } VariableSymbol;
typedef struct { char label[MAX_VAR_NAME]; char value[MAX_ARG_LEN]; } StringSymbol;
typedef struct {
    FILE *outfile;
    int label_count;
    VariableSymbol vars[MAX_VARS];
    int var_count;
    StringSymbol strings[MAX_STRINGS];
    int string_count;
} CompilerState;

// --- Helper Functions ---
char* trim_leading(char *str) { while (isspace((unsigned char)*str)) str++; return str; }
int new_label(CompilerState *state) { return state->label_count++; }

// --- Symbol Table Management ---
int get_var_offset(CompilerState *state, const char *name) {
    for (int i = 0; i < state->var_count; ++i) {
        if (strcmp(state->vars[i].name, name) == 0) return i * 8;
    }
    if (state->var_count >= MAX_VARS) { fprintf(stderr, "Compiler Error: Too many variables.\n"); exit(1); }
    int index = state->var_count++;
    strncpy(state->vars[index].name, name, MAX_VAR_NAME - 1); state->vars[index].name[MAX_VAR_NAME - 1] = '\0';
    return index * 8;
}

const char* get_string_label(CompilerState *state, const char *str) {
    for (int i = 0; i < state->string_count; ++i) {
        if (strcmp(state->strings[i].value, str) == 0) return state->strings[i].label;
    }
    if (state->string_count >= MAX_STRINGS) { fprintf(stderr, "Compiler Error: Too many string literals.\n"); exit(1); }
    int index = state->string_count++;
    sprintf(state->strings[index].label, "str%d", index);
    strncpy(state->strings[index].value, str, MAX_ARG_LEN - 1); state->strings[index].value[MAX_ARG_LEN - 1] = '\0';
    return state->strings[index].label;
}

bool is_number(const char* s) {
    if (!s || *s == '\0') return false;
    if (*s == '-') s++;
    if (*s == '\0') return false;
    while(*s) { if (!isdigit((unsigned char)*s)) return false; s++; }
    return true;
}

// --- Code Generation ---
void emit_load_value(CompilerState *state, const char *value_str) {
    if (is_number(value_str)) {
        fprintf(state->outfile, "    mov rax, %s\n", value_str);
    } else {
        int offset = get_var_offset(state, value_str);
        fprintf(state->outfile, "    mov rax, [vars + %d]\n", offset);
    }
}

void compile_script(CompilerState *state, char lines[][MAX_LINE_LEN], int line_count, int start_line, int end_line);

int find_matching_end(char lines[][MAX_LINE_LEN], int line_count, int start_line) {
    int depth = 1;
    for (int i = start_line; i < line_count; i++) {
        char temp_line[MAX_LINE_LEN]; strncpy(temp_line, lines[i], MAX_LINE_LEN);
        char *line = trim_leading(temp_line);
        if (strncmp(line, "if ", 3) == 0 || strncmp(line, "loop ", 5) == 0) depth++;
        else if (strcmp(line, "end") == 0) {
            depth--;
            if (depth == 0) return i;
        }
    }
    return -1;
}

void compile_script(CompilerState *state, char lines[][MAX_LINE_LEN], int line_count, int start_line, int end_line) {
    for (int i = start_line; i < end_line; ++i) {
        char line_buffer[MAX_LINE_LEN]; strncpy(line_buffer, lines[i], MAX_LINE_LEN - 1); line_buffer[MAX_LINE_LEN-1] = '\0';
        char *clean_line = trim_leading(line_buffer);
        if (*clean_line == '\0' || *clean_line == '#') continue;
        fprintf(state->outfile, "\n    ; Line %d: %s\n", i + 1, clean_line);
        char arg1[MAX_ARG_LEN] = {0}, arg2[MAX_ARG_LEN] = {0}, op[4] = {0};

        if (sscanf(clean_line, "set %s = %s", arg1, arg2) == 2) { int offset = get_var_offset(state, arg1); emit_load_value(state, arg2); fprintf(state->outfile, "    mov [vars + %d], rax\n", offset); }
        else if (sscanf(clean_line, "add %s %s", arg1, arg2) == 2) { int offset = get_var_offset(state, arg1); emit_load_value(state, arg2); fprintf(state->outfile, "    add [vars + %d], rax\n", offset); }
        else if (sscanf(clean_line, "sub %s %s", arg1, arg2) == 2) { int offset = get_var_offset(state, arg1); emit_load_value(state, arg2); fprintf(state->outfile, "    sub [vars + %d], rax\n", offset); }
        else if (sscanf(clean_line, "print \"%[^\"]\" %s", arg1, arg2) == 2) { const char* str_label = get_string_label(state, arg1); fprintf(state->outfile, "    mov rdi, %s\n    call print_string\n    mov rdi, ' '\n    call print_char\n", str_label); emit_load_value(state, arg2); fprintf(state->outfile, "    mov rdi, rax\n    call print_int\n    call print_newline\n"); }
        else if (sscanf(clean_line, "print \"%[^\"]\"", arg1) == 1) { const char* str_label = get_string_label(state, arg1); fprintf(state->outfile, "    mov rdi, %s\n    call print_string\n    call print_newline\n", str_label); }
        else if (sscanf(clean_line, "print %s", arg1) == 1) { emit_load_value(state, arg1); fprintf(state->outfile, "    mov rdi, rax\n    call print_int\n    call print_newline\n"); }
        else if (strncmp(clean_line, "loop ", 5) == 0) { int start_label = new_label(state), end_label = new_label(state); emit_load_value(state, clean_line + 5); fprintf(state->outfile, "    mov rcx, rax\n.L%d:\n    cmp rcx, 0\n    jle .L%d\n", start_label, end_label); int block_end = find_matching_end(lines, line_count, i + 1); if (block_end == -1) { fprintf(stderr, "Syntax Error: 'loop' on line %d has no matching 'end'.\n", i + 1); exit(1); } compile_script(state, lines, line_count, i + 1, block_end); fprintf(state->outfile, "    dec rcx\n    jmp .L%d\n.L%d:\n", start_label, end_label); i = block_end; }
        else if (strncmp(clean_line, "if ", 3) == 0) { if (sscanf(clean_line, "if %s %s %s", arg1, op, arg2) != 3) { fprintf(stderr, "Syntax Error: Malformed 'if' statement on line %d.\n", i + 1); exit(1); } int end_label = new_label(state); emit_load_value(state, arg1); fprintf(state->outfile, "    push rax\n"); emit_load_value(state, arg2); fprintf(state->outfile, "    pop rbx\n    cmp rbx, rax\n"); if (strcmp(op, "==") == 0) fprintf(state->outfile, "    jne .L%d\n", end_label); else if (strcmp(op, "!=") == 0) fprintf(state->outfile, "    je .L%d\n", end_label); else if (strcmp(op, ">") == 0) fprintf(state->outfile, "    jle .L%d\n", end_label); else if (strcmp(op, "<") == 0) fprintf(state->outfile, "    jge .L%d\n", end_label); else if (strcmp(op, ">=") == 0) fprintf(state->outfile, "    jl .L%d\n", end_label); else if (strcmp(op, "<=") == 0) fprintf(state->outfile, "    jg .L%d\n", end_label); else { fprintf(stderr, "Syntax Error: Unknown operator '%s' in 'if' on line %d.\n", op, i + 1); exit(1); } int block_end = find_matching_end(lines, line_count, i + 1); if (block_end == -1) { fprintf(stderr, "Syntax Error: 'if' on line %d has no matching 'end'.\n", i + 1); exit(1); } compile_script(state, lines, line_count, i + 1, block_end); fprintf(state->outfile, ".L%d:\n", end_label); i = block_end; }
        else if (strcmp(clean_line, "end") != 0) { fprintf(stderr, "Syntax Error: Unknown command on line %d: '%s'\n", i+1, clean_line); exit(1); }
    }
}

void emit_prologue(CompilerState *state) {
    fprintf(state->outfile, "section .data\n    minus_sign db '-', 0\n    newline db 10\n");
    for (int i = 0; i < state->string_count; i++) { fprintf(state->outfile, "    %s db \"%s\", 0\n", state->strings[i].label, state->strings[i].value); }
    fprintf(state->outfile, "\nsection .bss\n    vars resq %d\n    int_buffer resb 21\n", MAX_VARS);
    fprintf(state->outfile, "\nsection .text\n    global _start\n\n");
    fprintf(state->outfile, "print_string:\n    mov rbx, rdi\n    mov rdx, 0\n.strlen_loop:\n    cmp byte [rbx], 0\n    je .strlen_done\n    inc rdx\n    inc rbx\n    jmp .strlen_loop\n.strlen_done:\n    mov rax, 1\n    mov rsi, rdi\n    mov rdi, 1\n    syscall\n    ret\n\n");
    fprintf(state->outfile, "print_int:\n    mov rax, rdi\n    mov rsi, int_buffer + 20\n    mov byte [rsi], 10\n    mov rcx, 10\n    cmp rax, 0\n    jge .p_loop\n    neg rax\n    push rax\n    mov rdi, minus_sign\n    call print_string\n    pop rax\n.p_loop:\n    xor rdx, rdx\n    div rcx\n    add rdx, '0'\n    dec rsi\n    mov [rsi], dl\n    test rax, rax\n    jnz .p_loop\n    inc rsi\n    mov rdi, rsi\n    mov rbx, int_buffer + 21\n    sub rbx, rsi\n    mov rdx, rbx\n    mov rax, 1\n    mov rsi, rdi\n    mov rdi, 1\n    syscall\n    ret\n\n");
    fprintf(state->outfile, "print_newline:\n    mov rax, 1\n    mov rdi, 1\n    mov rsi, newline\n    mov rdx, 1\n    syscall\n    ret\n\n");
    fprintf(state->outfile, "print_char:\n    mov [int_buffer], dil\n    mov rax, 1\n    mov rdi, 1\n    mov rsi, int_buffer\n    mov rdx, 1\n    syscall\n    ret\n\n");
    fprintf(state->outfile, "_start:\n");
}

void emit_epilogue(CompilerState *state) { fprintf(state->outfile, "\n    mov rax, 60\n    xor rdi, rdi\n    syscall\n"); }

// --- Main Compiler Driver ---
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source_file.tiny> <output_executable_name>\n", argv[0]);
        return 1;
    }
    const char *source_filename = argv[1];
    const char *output_filename = argv[2];

    FILE *fp = fopen(source_filename, "r");
    if (!fp) { perror("Error opening source file"); return 1; }
    static char lines[MAX_LINES][MAX_LINE_LEN];
    int line_count = 0;
    while (line_count < MAX_LINES && fgets(lines[line_count], MAX_LINE_LEN, fp)) {
        lines[line_count][strcspn(lines[line_count], "\r\n")] = 0;
        line_count++;
    }
    fclose(fp);

    char asm_filename[256], obj_filename[256];
    snprintf(asm_filename, sizeof(asm_filename), "%s.asm", output_filename);
    snprintf(obj_filename, sizeof(obj_filename), "%s.o", output_filename);
    
    FILE *outfile = fopen(asm_filename, "w");
    if (!outfile) { perror("Error creating assembly file"); return 1; }

    CompilerState state;
    memset(&state, 0, sizeof(CompilerState));
    state.outfile = outfile;

    for (int i=0; i < line_count; i++) {
        char line_buffer[MAX_LINE_LEN], arg1[MAX_ARG_LEN];
        strncpy(line_buffer, lines[i], MAX_LINE_LEN);
        char *clean_line = trim_leading(line_buffer);
        if (sscanf(clean_line, "print \"%[^\"]\"", arg1) == 1 || sscanf(clean_line, "print \"%[^\"]\" %*s", arg1) == 1) {
            get_string_label(&state, arg1);
        }
    }

    emit_prologue(&state);
    compile_script(&state, lines, line_count, 0, line_count);
    emit_epilogue(&state);
    fclose(outfile);
    printf("Generated assembly file: %s\n", asm_filename);

    char command[512];
    
    // Assemble with NASM
    snprintf(command, sizeof(command), "nasm -f elf64 -o %s %s", obj_filename, asm_filename);
    printf("Running: %s\n", command);
    if (system(command) != 0) {
        fprintf(stderr, "Assembly failed. Make sure 'nasm' is installed.\n");
        return 1;
    }

    // Link with GCC driver - this is the robust way
    
    snprintf(command, sizeof(command), "gcc -no-pie -nostartfiles -o %s %s", output_filename, obj_filename);
    printf("Running: %s\n", command);
    if (system(command) != 0) {
        fprintf(stderr, "Linking failed. Make sure 'gcc' is installed.\n");
        return 1;
    }

    printf("\nSuccess! Created executable: %s\n", output_filename);

    // Clean up intermediate files
    remove(asm_filename);
    remove(obj_filename);

    return 0;
}