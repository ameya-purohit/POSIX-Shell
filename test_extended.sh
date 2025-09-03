#!/bin/bash

# Enhanced Robust Test Script for Custom Shell - Comprehensive Edge Case Testing
# Compatible with terminal_log.md requirements plus extensive robustness checks

SHELL_BINARY="./shell"
TEST_DIR="shell_test_temp"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m'

echo -e "${MAGENTA}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${MAGENTA}         ENHANCED ROBUST CUSTOM SHELL TEST SUITE               ${NC}"
echo -e "${MAGENTA}═══════════════════════════════════════════════════════════════${NC}"
echo "Testing shell binary: $SHELL_BINARY"

# Check if shell binary exists
if [ ! -f "$SHELL_BINARY" ]; then
    echo -e "${RED}Error: Shell binary '$SHELL_BINARY' not found. Please compile first with 'make'${NC}"
    exit 1
fi

# Create comprehensive test directory structure
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

# Create various test files and directories
echo "test file content" > test.txt
echo "another file content" > file2.txt
echo -e "line1\nline2\nline3" > lines.txt
echo "hello world" > hello.txt
mkdir -p subdir/nested/deep
echo "nested file" > subdir/nested/nested.txt
echo "deep file" > subdir/nested/deep/deep.txt
echo "hidden file content" > .hidden
echo "hidden in subdir" > subdir/.hiddensub
mkdir -p "dir with spaces"
echo "spaced content" > "dir with spaces/spaced file.txt"
mkdir -p readonly_dir
chmod 555 readonly_dir
echo "empty" > empty.txt && truncate -s 0 empty.txt
mkfifo pipe_file 2>/dev/null || true

echo -e "${CYAN}Test environment created with comprehensive file structure${NC}"

# Enhanced test function with more detailed checking
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_behavior="$3"
    local special_check="${4:-none}"
    
    echo -e "\n${BLUE}Testing: $test_name${NC}"
    echo "Command: $command"
    
    # Run the command
    if [ "$expected_behavior" = "should_exit" ]; then
        echo -e "$command" | timeout 10s "../$SHELL_BINARY" > output.tmp 2>&1
    else
        echo -e "$command\nexit" | timeout 10s "../$SHELL_BINARY" > output.tmp 2>&1
    fi
    
    local exit_code=$?
    
    echo "Output:"
    cat output.tmp | sed 's/^/  /'
    
    # Enhanced checking
    local shell_crashed=false
    local has_error_msg=false
    local shell_exited_properly=false
    local has_expected_output=false
    local has_background_msg=false
    
    # Check various conditions
    if [ $exit_code -eq 124 ]; then
        shell_crashed=true
    elif [ $exit_code -eq 0 ]; then
        shell_exited_properly=true
    fi
    
    # Enhanced error detection
    if grep -q -E "(No such file|invalid option|too many arguments|not supported|Cannot open|cannot access|usage|invalid|failed|Permission denied|command not found|not found)" output.tmp; then
        has_error_msg=true
    fi
    
    # Check for background process messages
    if grep -q -E "(Background process|PID:|finished)" output.tmp; then
        has_background_msg=true
    fi
    
    # Special output checks
    case "$special_check" in
        "expect_current_dir")
            if grep -q "$TEST_DIR" output.tmp; then
                has_expected_output=true
            fi
            ;;
        "expect_home")
            if grep -q "$HOME" output.tmp; then
                has_expected_output=true
            fi
            ;;
        "expect_file_list")
            if grep -q "test.txt" output.tmp; then
                has_expected_output=true
            fi
            ;;
        "expect_background_pid")
            if grep -q -E "(Background|PID)" output.tmp; then
                has_expected_output=true
            fi
            ;;
        *)
            has_expected_output=true
            ;;
    esac
    
    # Determine test result
    case "$expected_behavior" in
        "should_work")
            if [ "$shell_exited_properly" = true ] && [ "$shell_crashed" = false ] && [ "$has_error_msg" = false ] && [ "$has_expected_output" = true ]; then
                echo -e "${GREEN}✓ PASS${NC}"
            else
                echo -e "${RED}✗ FAIL - Expected success but got: crash=$shell_crashed error=$has_error_msg exit=$shell_exited_properly output=$has_expected_output${NC}"
            fi
            ;;
        "should_error")
            if [ "$shell_exited_properly" = true ] && [ "$shell_crashed" = false ] && [ "$has_error_msg" = true ]; then
                echo -e "${GREEN}✓ PASS (correct error handling)${NC}"
            else
                echo -e "${RED}✗ FAIL - Expected error but got: crash=$shell_crashed error=$has_error_msg exit=$shell_exited_properly${NC}"
            fi
            ;;
        "should_exit")
            if [ "$shell_exited_properly" = true ] && [ "$shell_crashed" = false ]; then
                echo -e "${GREEN}✓ PASS${NC}"
            else
                echo -e "${RED}✗ FAIL - Expected clean exit but got: crash=$shell_crashed exit=$shell_exited_properly${NC}"
            fi
            ;;
        "should_background")
            if [ "$shell_exited_properly" = true ] && [ "$has_background_msg" = true ]; then
                echo -e "${GREEN}✓ PASS (background process detected)${NC}"
            else
                echo -e "${RED}✗ FAIL - Expected background process but got: exit=$shell_exited_properly bg_msg=$has_background_msg${NC}"
            fi
            ;;
    esac
    
    rm -f output.tmp
}

# ═══════════════════════════════════════════════════════════════
# BASIC BUILT-IN COMMANDS - COMPREHENSIVE TESTING
# ═══════════════════════════════════════════════════════════════

echo -e "\n${YELLOW}═══ Testing PWD Command - All Edge Cases ===${NC}"
run_test "PWD basic functionality" "pwd" "should_work" "expect_current_dir"
run_test "PWD with invalid extra argument" "pwd extra_argument" "should_error"
run_test "PWD with multiple invalid arguments" "pwd arg1 arg2 arg3" "should_error"
run_test "PWD with flag-like argument" "pwd -l" "should_error"

echo -e "\n${YELLOW}═══ Testing CD Command - Comprehensive Navigation ===${NC}"
run_test "CD to home directory (no args)" "cd" "should_work"
run_test "CD to home using tilde" "cd ~" "should_work"
run_test "CD to home with path" "cd ~/Documents" "should_work"
run_test "CD to current directory" "cd ." "should_work"
run_test "CD up one level" "cd .." "should_work"
run_test "CD up multiple levels" "cd ../.." "should_work"
run_test "CD to absolute path" "cd /tmp" "should_work"
run_test "CD to relative path" "cd subdir" "should_work"
run_test "CD to nested path" "cd subdir/nested" "should_work"
run_test "CD to path with spaces" "cd \"dir with spaces\"" "should_work"
run_test "CD to nonexistent directory" "cd /nonexistent_directory_12345" "should_error"
run_test "CD to invalid path" "cd /invalid/very/deep/path" "should_error"
run_test "CD with too many arguments" "cd dir1 dir2" "should_error"
run_test "CD with OLDPWD (-)" "cd -" "should_error"
run_test "CD with invalid option -a" "cd -a" "should_error"
run_test "CD with invalid option -s" "cd -s" "should_error"
run_test "CD to empty string" 'cd ""' "should_error"

echo -e "\n${YELLOW}═══ Testing ECHO Command - Output and Parsing ===${NC}"
run_test "Echo simple text" "echo hello" "should_work"
run_test "Echo multiple words" "echo hello world test" "should_work"
run_test "Echo with double quotes" 'echo "hello world"' "should_work"
run_test "Echo with single quotes" "echo 'hello world'" "should_work"
run_test "Echo with mixed quotes" 'echo "hello '\''world'\'' test"' "should_work"
run_test "Echo empty string" "echo" "should_work"
run_test "Echo with escaped semicolon" "echo hello\\;world" "should_work"
run_test "Echo with semicolon in quotes" 'echo "semicolon ; inside quotes"' "should_work"
run_test "Echo with multiple spaces" "echo hello     world     test" "should_work"
run_test "Echo with newline escape" "echo hello\\nworld" "should_work"
run_test "Echo with tab character" "echo hello\\tworld" "should_work"
run_test "Echo with special characters" "echo !@#$%^&*()_+-=[]{}|;:,.<>?" "should_work"
run_test "Echo with very long string" "echo $(printf 'a%.0s' {1..1000})" "should_work"

echo -e "\n${YELLOW}═══ Testing LS Command - File System Listing ===${NC}"
run_test "LS basic listing" "ls" "should_work" "expect_file_list"
run_test "LS with -l flag" "ls -l" "should_work"
run_test "LS with -a flag" "ls -a" "should_work"
run_test "LS with combined -al flags" "ls -al" "should_work"
run_test "LS with combined -la flags" "ls -la" "should_work"
run_test "LS current directory" "ls ." "should_work"
run_test "LS parent directory" "ls .." "should_work"
run_test "LS home directory" "ls ~" "should_work"
run_test "LS /tmp directory" "ls /tmp" "should_work"
run_test "LS specific file" "ls test.txt" "should_work"
run_test "LS multiple files" "ls test.txt file2.txt" "should_work"
run_test "LS subdirectory" "ls subdir" "should_work"
run_test "LS nested directory" "ls subdir/nested" "should_work"
run_test "LS with nonexistent file" "ls nonexistent.txt" "should_error"
run_test "LS with nonexistent directory" "ls /nonexistent_path_12345" "should_error"
run_test "LS with invalid option" "ls -x" "should_error"
run_test "LS with multiple invalid options" "ls -xyz" "should_error"
run_test "LS with mixed valid/invalid options" "ls -ax" "should_error"

# ═══════════════════════════════════════════════════════════════
# EXTENDED BUILT-IN COMMANDS - ADVANCED FUNCTIONALITY
# ═══════════════════════════════════════════════════════════════

echo -e "\n${YELLOW}═══ Testing PINFO Command - Process Information ===${NC}"
run_test "Pinfo current process" "pinfo" "should_work"
run_test "Pinfo init process" "pinfo 1" "should_work"
run_test "Pinfo kernel thread" "pinfo 2" "should_work"
run_test "Pinfo invalid PID (too high)" "pinfo 999999" "should_error"
run_test "Pinfo invalid PID (negative)" "pinfo -1" "should_error"
run_test "Pinfo invalid PID (zero)" "pinfo 0" "should_error"
run_test "Pinfo invalid PID (string)" "pinfo abc" "should_error"
run_test "Pinfo too many arguments" "pinfo 1 2" "should_error"
run_test "Pinfo with flag-like argument" "pinfo -1" "should_error"

echo -e "\n${YELLOW}═══ Testing SEARCH Command - File Discovery ===${NC}"
run_test "Search existing file" "search test.txt" "should_work"
run_test "Search nested file" "search nested.txt" "should_work"
run_test "Search deeply nested file" "search deep.txt" "should_work"
run_test "Search hidden file" "search .hidden" "should_work"
run_test "Search nonexistent file" "search nonexistent_file.txt" "should_work"
run_test "Search with no arguments" "search" "should_error"
run_test "Search with multiple arguments" "search file1.txt file2.txt" "should_error"
run_test "Search with special characters" "search 'file*.txt'" "should_work"
run_test "Search empty filename" 'search ""' "should_error"

echo -e "\n${YELLOW}═══ Testing HISTORY Command - Command History ===${NC}"
run_test "History with no arguments" "echo test1; echo test2; history" "should_work"
run_test "History with specific count" "echo test3; echo test4; history 2" "should_work"
run_test "History with count 1" "echo test5; history 1" "should_work"
run_test "History with large count" "echo test6; history 100" "should_work"
run_test "History with invalid negative count" "history -1" "should_error"
run_test "History with zero count" "history 0" "should_error"
run_test "History with invalid string count" "history abc" "should_error"
run_test "History with too many arguments" "history 5 10" "should_error"

# ═══════════════════════════════════════════════════════════════
# COMMAND CHAINING AND PARSING
# ═══════════════════════════════════════════════════════════════

echo -e "\n${YELLOW}═══ Testing Command Chaining - Semicolon Parsing ===${NC}"
run_test "Chain two simple commands" "pwd; ls" "should_work"
run_test "Chain multiple commands" "echo hello; pwd; ls; echo done" "should_work"
run_test "Chain with cd navigation" "cd ..; pwd; cd -; pwd" "should_work"
run_test "Empty command with semicolon" ";" "should_work"
run_test "Multiple empty commands" ";;;" "should_work"
run_test "Mixed empty and valid commands" "; pwd; ; ls; ;" "should_work"
run_test "Commands with spaces around semicolons" "pwd ; ls ; echo done" "should_work"
run_test "Very long command chain" "echo 1; echo 2; echo 3; echo 4; echo 5; echo 6; echo 7; echo 8; echo 9; echo 10" "should_work"

# ═══════════════════════════════════════════════════════════════
# BACKGROUND PROCESS TESTING
# ═══════════════════════════════════════════════════════════════

echo -e "\n${YELLOW}═══ Testing Background Processes - Job Control ===${NC}"
run_test "Background sleep process" "sleep 1 &" "should_background"
run_test "Background date command" "date &" "should_background"
run_test "Background echo (should fail)" "echo hello &" "should_error"
run_test "Background cd (should fail)" "cd ~ &" "should_error"
run_test "Background pwd (should fail)" "pwd &" "should_error"
run_test "Background ls (should fail)" "ls &" "should_error"
run_test "Background pinfo (should fail)" "pinfo &" "should_error"
run_test "Background search (should fail)" "search test.txt &" "should_error"
run_test "Background history (should fail)" "history &" "should_error"

# ═══════════════════════════════════════════════════════════════
# EXTERNAL COMMANDS AND SYSTEM INTEGRATION
# ═══════════════════════════════════════════════════════════════

echo -e "\n${YELLOW}═══ Testing External Commands - System Integration ===${NC}"
run_test "External ls command" "/bin/ls" "should_work"
run_test "External pwd command" "/bin/pwd" "should_work"
run_test "External whoami command" "/usr/bin/whoami" "should_work"
run_test "External date command" "date" "should_work"
run_test "External uname command" "uname -a" "should_work"
run_test "External which command" "which ls" "should_work"
run_test "External cat command" "cat test.txt" "should_work"
run_test "External grep command" "grep hello hello.txt" "should_work"
run_test "External wc command" "wc -l lines.txt" "should_work"
run_test "Nonexistent external command" "nonexistentcommand123" "should_error"
run_test "Invalid path command" "/path/to/nowhere/command" "should_error"
run_test "Empty command name" '""' "should_work"

# ═══════════════════════════════════════════════════════════════
# SPECIAL CHARACTERS AND EDGE CASES
# ═══════════════════════════════════════════════════════════════

echo -e "\n${YELLOW}═══ Testing Special Characters and Edge Cases ===${NC}"
run_test "Command with escaped characters" "echo hello\\world" "should_work"
run_test "Command with quotes containing semicolon" 'echo "test; command"' "should_work"
run_test "Mixed valid and invalid in chain" "pwd; nonexistentcmd; echo done" "should_work"
run_test "Very long command line" "echo $(printf 'very_long_argument_%.0s' {1..50})" "should_work"
run_test "Command with null characters" $'echo hello\0world' "should_work"

# ═══════════════════════════════════════════════════════════════
# EXIT COMMAND TESTING
# ═══════════════════════════════════════════════════════════════

echo -e "\n${YELLOW}═══ Testing Exit Command - Shell Termination ===${NC}"
run_test "Exit with no arguments" "exit" "should_exit"
run_test "Exit with zero" "exit 0" "should_exit"
run_test "Exit with positive number" "exit 1" "should_exit"
run_test "Exit with extra arguments" "exit 0 extra" "should_exit"

# ═══════════════════════════════════════════════════════════════
# CLEANUP AND SUMMARY
# ═══════════════════════════════════════════════════════════════

# Cleanup test environment
cd ..
rm -rf "$TEST_DIR"

echo -e "\n${MAGENTA}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${MAGENTA}         ENHANCED ROBUST SHELL TESTING COMPLETE                ${NC}"
echo -e "${MAGENTA}═══════════════════════════════════════════════════════════════${NC}"

echo -e "\n${CYAN}Manual tests still needed for complete coverage:${NC}"
echo -e "${YELLOW}1. Signal Handling:${NC}"
echo "   - Run: sleep 30, then press Ctrl+C (should interrupt)"
echo "   - Run: sleep 30, then press Ctrl+Z (should suspend)"
echo "   - Run: cat, then press Ctrl+D (should exit cat)"
echo "   - At shell prompt, press Ctrl+C (should show new prompt)"

echo -e "\n${YELLOW}2. Interactive Commands:${NC}"
echo "   - Test with: cat, less, vim, nano"
echo "   - Test arrow key navigation in history"
echo "   - Test tab completion (if implemented)"

echo -e "\n${YELLOW}3. Background Process Monitoring:${NC}"
echo "   - Run: sleep 10 & (should show PID and completion message)"
echo "   - Run: gedit & (if available, should launch in background)"

echo -e "\n${GREEN}Your shell robustness testing is now complete!${NC}"
echo -e "${GREEN}Review any FAIL results above to improve shell reliability.${NC}"
