#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// --- Configuration Constants ---
#define MAX_VARS 100
#define MAX_VAR_NAME 32
#define MAX_ARG_LEN 128
#define MAX_LINES 1000
#define MAX_LINE_LEN 256

// --- Type Definitions ---

// Represents a variable in our language
typedef struct {
    char name[MAX_VAR_NAME];
    int value;
} Variable;

// Encapsulates the entire state of the interpreter
typedef struct {
    Variable vars[MAX_VARS];
    int var_count;
    // We could add more state here later, like function call stacks
} InterpreterState;

// --- Variable Management ---

// Find a variable and return a pointer to it, or NULL if not found.
Variable* find_var(InterpreterState *state, const char *name) {
    for (int i = 0; i < state->var_count; ++i) {
        if (strcmp(state->vars[i].name, name) == 0) {
            return &state->vars[i];
        }
    }
    return NULL;
}

// Get the integer value of a variable or a numeric literal.
// Exits on error if the identifier is not a variable and not a number.
int resolve_value(InterpreterState *state, const char *name_or_literal) {
    // Check if it's a number (handles negative numbers)
    if (isdigit((unsigned char)name_or_literal[0]) || (name_or_literal[0] == '-' && isdigit((unsigned char)name_or_literal[1]))) {
        return atoi(name_or_literal);
    }
    
    // If not a number, it must be a variable
    Variable *var = find_var(state, name_or_literal);
    if (var) {
        return var->value;
    }

    // Error: Undeclared variable
    fprintf(stderr, "Runtime Error: Unknown variable or invalid number '%s'\n", name_or_literal);
    exit(1);
}

// Set a variable's value. Creates it if it doesn't exist.
void set_var(InterpreterState *state, const char *name, int value) {
    Variable *var = find_var(state, name);
    if (var) {
        // Variable exists, update it
        var->value = value;
    } else {
        // Variable is new, create it
        if (state->var_count >= MAX_VARS) {
            fprintf(stderr, "Runtime Error: Maximum number of variables (%d) reached.\n", MAX_VARS);
            exit(1);
        }
        strncpy(state->vars[state->var_count].name, name, MAX_VAR_NAME - 1);
        state->vars[state->var_count].name[MAX_VAR_NAME - 1] = '\0'; // Ensure null-termination
        state->vars[state->var_count].value = value;
        state->var_count++;
    }
}

// --- String and Parsing Helpers ---

// Trim leading and trailing whitespace from a string, in-place.
char* trim_whitespace(char *str) {
    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) // All spaces?
        return str;

    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    *(end + 1) = 0;

    return str;
}

// --- Core Execution Logic ---

void run_script(InterpreterState *state, char lines[][MAX_LINE_LEN], int line_count, int start_line, int end_line);

// Helper to find the matching 'end' for a block statement ('if', 'loop')
int find_matching_end(char lines[][MAX_LINE_LEN], int line_count, int start_line) {
    int depth = 1;
    for (int i = start_line; i < line_count; i++) {
        char *line = trim_whitespace(lines[i]);
        if (strncmp(line, "if ", 3) == 0 || strncmp(line, "loop ", 5) == 0) {
            depth++;
        } else if (strcmp(line, "end") == 0) {
            depth--;
            if (depth == 0) {
                return i;
            }
        }
    }
    return -1; // No matching 'end' found
}

// Executes a single, simple command (not control flow)
void execute_line(InterpreterState *state, char *line) {
    char command[MAX_ARG_LEN];
    char arg1[MAX_ARG_LEN], arg2[MAX_ARG_LEN];

    // --- PARSE PRINT ---
    // print "message" var
    if (sscanf(line, "print \"%[^\"]\" %s", arg1, arg2) == 2) {
        int val = resolve_value(state, arg2);
        printf("%s %d\n", arg1, val);
        return;
    }
    // print "message"
    if (sscanf(line, "print \"%[^\"]\"", arg1) == 1) {
        printf("%s\n", arg1);
        return;
    }
    // print var_or_number
    if (sscanf(line, "print %s", arg1) == 1) {
        printf("%d\n", resolve_value(state, arg1));
        return;
    }

    // --- PARSE SET ---
    // set var = value
    if (sscanf(line, "set %s = %s", arg1, arg2) == 2) {
        int val = resolve_value(state, arg2);
        set_var(state, arg1, val);
        return;
    }

    // --- PARSE ADD ---
    // add var value
    if (sscanf(line, "add %s %s", arg1, arg2) == 2) {
        int current_val = resolve_value(state, arg1);
        int amount = resolve_value(state, arg2);
        set_var(state, arg1, current_val + amount);
        return;
    }

    // --- PARSE SUBTRACT (Example of how easy it is to add a command) ---
    // sub var value
    if (sscanf(line, "sub %s %s", arg1, arg2) == 2) {
        int current_val = resolve_value(state, arg1);
        int amount = resolve_value(state, arg2);
        set_var(state, arg1, current_val - amount);
        return;
    }

    fprintf(stderr, "Syntax Error: Unknown command on line: '%s'\n", line);
}

// Main recursive function to execute a script or a block of it
void run_script(InterpreterState *state, char lines[][MAX_LINE_LEN], int line_count, int start_line, int end_line) {
    for (int i = start_line; i < end_line; i++) {
        char line_buffer[MAX_LINE_LEN];
        strncpy(line_buffer, lines[i], MAX_LINE_LEN - 1);
        line_buffer[MAX_LINE_LEN-1] = '\0';

        char *clean_line = trim_whitespace(line_buffer);

        // Skip empty or comment lines
        if (*clean_line == '\0' || *clean_line == '#') {
            continue;
        }

        // --- Handle Control Flow: LOOP ---
        if (strncmp(clean_line, "loop ", 5) == 0) {
            int loop_count = resolve_value(state, clean_line + 5);
            int block_start = i + 1;
            int block_end = find_matching_end(lines, line_count, block_start);

            if (block_end == -1) {
                fprintf(stderr, "Syntax Error: 'loop' on line %d has no matching 'end'.\n", i + 1);
                exit(1);
            }
            
            for (int j = 0; j < loop_count; j++) {
                run_script(state, lines, line_count, block_start, block_end);
            }
            i = block_end; // Skip interpreter past the handled block
            continue;
        }

        // --- Handle Control Flow: IF ---
        if (strncmp(clean_line, "if ", 3) == 0) {
            char var_name[MAX_ARG_LEN], op[3], val_str[MAX_ARG_LEN];
            if (sscanf(clean_line, "if %s %2s %s", var_name, op, val_str) != 3) {
                 fprintf(stderr, "Syntax Error: Malformed 'if' statement on line %d.\n", i + 1);
                 exit(1);
            }

            int left_val = resolve_value(state, var_name);
            int right_val = resolve_value(state, val_str);
            bool condition = false;

            if (strcmp(op, "==") == 0) condition = (left_val == right_val);
            else if (strcmp(op, "!=") == 0) condition = (left_val != right_val);
            else if (strcmp(op, ">") == 0) condition = (left_val > right_val);
            else if (strcmp(op, "<") == 0) condition = (left_val < right_val);
            else if (strcmp(op, ">=") == 0) condition = (left_val >= right_val);
            else if (strcmp(op, "<=") == 0) condition = (left_val <= right_val);
            else {
                fprintf(stderr, "Syntax Error: Unknown operator '%s' in 'if' on line %d.\n", op, i + 1);
                exit(1);
            }

            int block_start = i + 1;
            int block_end = find_matching_end(lines, line_count, block_start);
            
            if (block_end == -1) {
                fprintf(stderr, "Syntax Error: 'if' on line %d has no matching 'end'.\n", i + 1);
                exit(1);
            }

            if (condition) {
                run_script(state, lines, line_count, block_start, block_end);
            }
            i = block_end; // Skip interpreter past the handled block
            continue;
        }

        // If it's not a control flow keyword, execute it as a simple command
        execute_line(state, clean_line);
    }
}


// --- Main Program ---
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <script_file.tako>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    char lines[MAX_LINES][MAX_LINE_LEN];
    int line_count = 0;
    while (line_count < MAX_LINES && fgets(lines[line_count], MAX_LINE_LEN, fp)) {
        // Remove newline characters
        lines[line_count][strcspn(lines[line_count], "\r\n")] = 0;
        line_count++;
    }
    fclose(fp);
    
    if (line_count >= MAX_LINES) {
        fprintf(stderr, "Warning: Reached maximum line limit of %d. File may be truncated.\n", MAX_LINES);
    }

    // Initialize the interpreter state
    InterpreterState state;
    memset(&state, 0, sizeof(InterpreterState));

    // Run the script!
    run_script(&state, lines, line_count, 0, line_count);

    return 0;
}
