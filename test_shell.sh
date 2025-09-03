#!/bin/bash

# Test script for custom shell
# Usage: ./test_shell.sh

SHELL_BINARY="./shell"
TEST_DIR="shell_test_temp"
RESULTS_FILE="test_results.txt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Custom Shell Test Suite ===${NC}"
echo "Testing shell binary: $SHELL_BINARY"

# Check if shell binary exists
if [ ! -f "$SHELL_BINARY" ]; then
    echo -e "${RED}Error: Shell binary '$SHELL_BINARY' not found. Please compile first with 'make'${NC}"
    exit 1
fi

# Create test directory and files
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"
echo "test file" > test.txt
echo "another file" > file2.txt
mkdir -p subdir
echo "hidden file" > .hidden

echo -e "${YELLOW}Starting automated tests...${NC}"

# Function to run a command and capture output
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_behavior="$3"  # "should_work", "should_error", or "should_exit"
    
    echo -e "\n${BLUE}Testing: $test_name${NC}"
    echo "Command: $command"
    
    # Run the command
    if [ "$expected_behavior" = "should_exit" ]; then
        # For exit command, don't add extra commands
        echo -e "$command" | timeout 3s "../$SHELL_BINARY" > output.tmp 2>&1
    else
        # For other commands, add exit to ensure shell terminates
        echo -e "$command\nexit" | timeout 5s "../$SHELL_BINARY" > output.tmp 2>&1
    fi
    local exit_code=$?
    
    echo "Output:"
    cat output.tmp | sed 's/^/  /'
    
    # Check if shell responded appropriately
    local shell_crashed=false
    local has_error_msg=false
    local has_goodbye=false
    
    # Check for crash indicators
    if [ $exit_code -eq 124 ]; then
        shell_crashed=true  # timeout occurred
    fi
    
    # Check for error messages
    if grep -q -E "(No such file|invalid option|too many arguments|not supported|Cannot open|cannot access)" output.tmp; then
        has_error_msg=true
    fi
    
    # Check for proper exit
    if grep -q "Goodbye!" output.tmp; then
        has_goodbye=true
    fi
    
    # Determine if test passed based on expected behavior
    case "$expected_behavior" in
        "should_work")
            if [ "$has_goodbye" = true ] && [ "$shell_crashed" = false ] && [ "$has_error_msg" = false ]; then
                echo -e "${GREEN}✓ PASS${NC}"
            else
                echo -e "${RED}✗ FAIL${NC}"
            fi
            ;;
        "should_error")
            if [ "$has_goodbye" = true ] && [ "$shell_crashed" = false ] && [ "$has_error_msg" = true ]; then
                echo -e "${GREEN}✓ PASS (correct error handling)${NC}"
            else
                echo -e "${RED}✗ FAIL${NC}"
            fi
            ;;
        "should_exit")
            if [ "$has_goodbye" = true ] && [ "$shell_crashed" = false ]; then
                echo -e "${GREEN}✓ PASS${NC}"
            else
                echo -e "${RED}✗ FAIL${NC}"
            fi
            ;;
    esac
    
    rm -f output.tmp
}

# Basic Built-in Commands Tests
echo -e "\n${YELLOW}=== Testing Basic Built-ins ===${NC}"

run_test "PWD command" "pwd" "should_work"
run_test "PWD with extra args" "pwd extra_argument" "should_error"

run_test "CD to home" "cd ~" "should_work"
run_test "CD up one level" "cd .." "should_work"
run_test "CD to current" "cd ." "should_work"
run_test "CD to nonexistent" "cd /nonexistent_dir_12345" "should_error"

run_test "Echo simple" "echo hello" "should_work"
run_test "Echo multiple words" "echo hello world" "should_work"
run_test "Echo with quotes" 'echo "hello world"' "should_work"
run_test "Echo empty" "echo" "should_work"

run_test "Exit command" "exit" "should_exit"

# LS Tests
echo -e "\n${YELLOW}=== Testing LS Command ===${NC}"

run_test "LS basic" "ls" "should_work"
run_test "LS with -l" "ls -l" "should_work"
run_test "LS with -a" "ls -a" "should_work"
run_test "LS with -al" "ls -al" "should_work"
run_test "LS with -la" "ls -la" "should_work"
run_test "LS current dir" "ls ." "should_work"
run_test "LS parent dir" "ls .." "should_work"
run_test "LS nonexistent" "ls /nonexistent_path_12345" "should_error"
run_test "LS invalid option" "ls -x" "should_error"

# Command Chaining Tests
echo -e "\n${YELLOW}=== Testing Command Chaining ===${NC}"

run_test "Chain PWD and LS" "pwd; ls" "should_work"
run_test "Chain multiple commands" "echo hello; pwd; echo world" "should_work"

# Background Process Tests
echo -e "\n${YELLOW}=== Testing Background Processes ===${NC}"

run_test "Background builtin (should fail)" "echo hello &" "should_error"
run_test "Background cd (should fail)" "cd ~ &" "should_error"
run_test "Background external command" "sleep 1 &" "should_work"

# External Commands Tests
echo -e "\n${YELLOW}=== Testing External Commands ===${NC}"

run_test "Date command" "date" "should_work"
run_test "Whoami command" "whoami" "should_work"
run_test "Which ls" "which ls" "should_work"
run_test "Nonexistent command" "nonexistentcommand123" "should_error"

# Edge Cases
echo -e "\n${YELLOW}=== Testing Edge Cases ===${NC}"

run_test "Empty command" "" "should_work"
run_test "Just semicolon" ";" "should_work"
run_test "Multiple semicolons" ";;" "should_work"

# Cleanup
cd ..
rm -rf "$TEST_DIR"

echo -e "\n${BLUE}=== Test Summary ===${NC}"
echo "All automated tests completed."
echo ""
echo -e "${YELLOW}Manual tests still needed:${NC}"
echo "1. Signal handling (Ctrl+C, Ctrl+Z)"
echo "2. Interactive commands (cat, vim)"
echo "3. Command history (arrow keys)"
echo ""
echo -e "${BLUE}To run manual signal tests:${NC}"
echo "  $SHELL_BINARY"
echo "  sleep 10    # Then press Ctrl+C"
echo "  sleep 10    # Then press Ctrl+Z" 
echo "  cat         # Then press Ctrl+D"
echo "  exit"
