# =============================================================================
# Makefile for the 'tako' project
# =============================================================================

# --- Compiler and Flags ---
# CC: The C compiler to use (gcc is the standard)
# CFLAGS: Flags for the compiler.
#   -Wall: Enables all standard warnings. It's good practice to fix all warnings.
#   -Wextra: Enables even more warnings not covered by -Wall.
#   -g: Includes debugging information in the executable (for use with gdb).
#   -O2: Optimization level 2 (you can use this for a release version).
CC = gcc
CFLAGS = -Wall -Wextra -g

# --- Project Files ---
# EXEC: The name of your final executable program.
# SRCS: A list of all your .c source files.
EXEC = tako
SRCS = tako.c

# --- Automatic Variables ---
# OBJS: Automatically converts the list of .c files to a list of .o (object) files.
#       (e.g., "tako.c" becomes "tako.o")
OBJS = $(SRCS:.c=.o)

# --- Install Location ---
# INSTALL_DIR: The directory where the program will be installed.
#              /usr/local/bin is a standard location for user-installed programs.
INSTALL_DIR = /usr/local/bin


# =============================================================================
# Targets
# =============================================================================

# The 'all' target is the default. Running 'make' will execute this.
# It depends on the executable, so it will trigger the rule to build it.
all: $(EXEC)

# Rule to link the object files (.o) into the final executable.
# The '$^' variable means "all the prerequisites" (all the .o files).
# The '$@' variable means "the target name" (the executable file).
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Pattern rule to compile any .c file into its corresponding .o file.
# The '-c' flag tells the compiler to compile but not link.
# The '$<' variable means "the first prerequisite" (the .c file).
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# The 'clean' target removes all generated files (object files and the executable).
clean:
	rm -f $(OBJS) $(EXEC)

# The 'install' target copies the built program to the system install directory.
# You will likely need to run this with 'sudo make install' because it
# writes to a system directory.
install: all
	@echo "Installing $(EXEC) to $(INSTALL_DIR)..."
	install -d $(INSTALL_DIR)
	install -m 0755 $(EXEC) $(INSTALL_DIR)
	@echo "Installation complete."

# The 'uninstall' target removes the program from the system install directory.
# You will also likely need 'sudo' for this.
uninstall:
	@echo "Uninstalling $(EXEC) from $(INSTALL_DIR)..."
	rm -f $(INSTALL_DIR)/$(EXEC)
	@echo "Uninstallation complete."


# --- Phony Targets ---
# Declares targets that are not actual files. This prevents 'make' from getting
# confused if a file with the same name (e.g., a file named 'clean') exists.
.PHONY: all clean install uninstall
