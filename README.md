# Custom POSIX Shell Implementation 🐚

A robust Unix-like shell implementation developed in C++ as part of the Advanced Operating Systems course (Monsoon 2025). This shell provides a comprehensive command-line interface with support for built-in commands, external program execution, I/O redirection, pipelines, and advanced features like autocomplete and signal handling.

## 📋 Features

### Core Functionality
- **Dynamic Prompt**: Displays `username@hostname:current_directory>` format with `~` representing the shell's home directory
- **Command Chaining**: Support for semicolon-separated commands (`;`) for executing multiple commands in sequence
- **Background Processes**: Execute commands in the background using the `&` operator with PID tracking
- **Signal Handling**: Proper handling of `Ctrl+C` (SIGINT), `Ctrl+Z` (SIGTSTP), and `Ctrl+D` (EOF)

### Built-in Commands
All built-in commands are implemented without using `execvp` as per assignment requirements:

- **`cd`** - Change directory with support for `.`, `..`, `~`, and `-` flags
- **`pwd`** - Print current working directory (always shows absolute path)
- **`echo`** - Print arguments to stdout with proper space handling
- **`ls`** - List directory contents with `-a` (show hidden) and `-l` (long format) flags
- **`pinfo [pid]`** - Display process information including status, memory usage, and executable path
- **`search <filename>`** - Recursively search for files/directories in current directory
- **`history [num]`** - View command history (stores up to 20 commands, displays 10 by default)
- **`exit`** - Exit the shell gracefully

### Advanced Features
- **I/O Redirection**: Support for `<`, `>`, and `>>` operators
- **Pipelines**: Connect multiple commands using `|` operator with support for any number of pipes
- **Autocomplete**: Tab completion for commands and files/directories using readline library
- **Command History**: Persistent command history with arrow key navigation
- **Quote Handling**: Proper parsing of quoted strings and escaped characters
- **Error Handling**: Comprehensive error handling for all operations

## 🏗️ Architecture

The shell is designed with a modular architecture for maintainability and clarity:

### File Structure
```
2025202006_Assignment2/
├── README.md                 # This file
├── makefile                 # Build configuration
├── include/                 # Header files
│   ├── shell.h             # Main shell declarations
│   ├── builtins.h          # Built-in command declarations
│   ├── pipeline.h          # Pipeline handling declarations
│   ├── redirection.h       # I/O redirection declarations
│   └── autocomplete.h      # Autocomplete functionality declarations
└── src/                    # Source files
    ├── main.cpp            # Entry point and main shell loop
    ├── shell.cpp           # Core shell functionality and tokenization
    ├── builtins.cpp        # Built-in command implementations
    ├── pipeline.cpp        # Pipeline execution logic
    ├── redirection.cpp     # I/O redirection setup
    └── autocomplete.cpp    # Tab completion implementation
```

### Component Responsibilities

- **`main.cpp`**: Main shell loop, signal handlers, semicolon command parsing
- **`shell.cpp`**: Command tokenization, external command execution, prompt generation
- **`builtins.cpp`**: All built-in command implementations and history management
- **`pipeline.cpp`**: Pipeline parsing and execution with proper process management
- **`redirection.cpp`**: File descriptor manipulation for I/O redirection
- **`autocomplete.cpp`**: Readline-based tab completion for commands and files

## 🚀 Getting Started

### Prerequisites
- C++ compiler (g++ recommended)
- GNU Readline library and development headers
- Linux/Unix environment

### Installation
```bash
# Install readline development package (Ubuntu/Debian)
sudo apt-get install libreadline-dev

# Clone or extract the project
cd 2025202006_Assignment2

# Compile the shell
make

# Run the shell
./shell
```

### Usage Examples

#### Basic Commands
```bash
ameya@ameya-hp:~> pwd
/home/ameya/2025202006_Assignment2

ameya@ameya-hp:~> echo "Hello World"
"Hello World"

ameya@ameya-hp:~> ls -la
total 64
drwxrwxr-x 5 ameya ameya 4096 Sep 04 18:30 .
drwxrwxr-x 8 ameya ameya 4096 Sep 04 18:30 ..
-rw-rw-r-- 1 ameya ameya 1024 Sep 04 18:30 README.md
```

#### Command Chaining
```bash
ameya@ameya-hp:~> pwd; ls; echo "Done"
/home/ameya/2025202006_Assignment2
file1.txt  file2.txt  directory/
Done
```

#### Background Processes
```bash
ameya@ameya-hp:~> sleep 10 &
Background process started with PID: 12345

ameya@ameya-hp:~> pinfo 12345
Process Status -- S
memory -- 2048 {Virtual Memory}
Executable Path -- /usr/bin/sleep
```

#### I/O Redirection
```bash
ameya@ameya-hp:~> echo "Hello" > output.txt
ameya@ameya-hp:~> cat < output.txt
Hello
ameya@ameya-hp:~> echo "World" >> output.txt
```

#### Pipelines
```bash
ameya@ameya-hp:~> cat file.txt | grep "pattern" | wc -l
5

ameya@ameya-hp:~> ls -l | grep "txt" > text_files.txt
```

## 🛠️ Technical Implementation

### Tokenization Strategy
- Uses `strtok` for basic command splitting as recommended by assignment
- Custom tokenization for handling quotes, escapes, and special operators
- Proper semicolon parsing that respects quoted strings

### Process Management
- Uses `fork()` and `execvp()` for external command execution
- Proper signal handling with `sigaction()` for robust process control
- Background process tracking and cleanup

### Memory Management
- Careful memory allocation and deallocation to prevent leaks
- Dynamic string handling for variable-length commands
- Proper cleanup in signal handlers using async-signal-safe functions

### Error Handling
- Comprehensive error checking for all system calls
- User-friendly error messages following shell conventions
- Graceful degradation when features are unavailable

## 🔧 Technical Specifications

- **Language**: C++
- **Compilation**: g++ with appropriate flags and linking options
- **Dependencies**: GNU Readline library
- **Standards**: POSIX-compliant system calls
- **Memory Safety**: No memory leaks or segmentation faults
- **Signal Safety**: Async-signal-safe operations in signal handlers

## 📝 Assignment Compliance

This implementation fulfills all requirements of the Advanced Operating Systems Assignment 2:

✅ Dynamic prompt generation without hardcoded values  
✅ Built-in commands (cd, pwd, echo, ls, pinfo, search, history)  
✅ External command execution with background support  
✅ I/O redirection (`<`, `>`, `>>`)  
✅ Pipeline support (`|`) with unlimited command chaining  
✅ Signal handling (Ctrl+C, Ctrl+Z, Ctrl+D)  
✅ Command history with persistence  
✅ Autocomplete functionality  
✅ Semicolon command separation  
✅ Proper error handling and user feedback  
✅ Modular code architecture  
✅ Use of recommended system calls and libraries  

## 🎯 Testing

The shell has been extensively tested with:
- Edge case scenarios for all built-in commands
- Complex pipeline and redirection combinations
- Signal handling under various conditions
- Memory management validation
- Error condition handling

## 👤 Author

**Ameya Purohit** - Roll Number: 2025202006  
MTech CSIS, IIIT Hyderabad  
