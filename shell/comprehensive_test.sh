#!/bin/bash
# Comprehensive Shell Test Script

SHELL_BIN="./shell.out"
TEST_DIR="test_workspace"
FAILED=0
PASSED=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo "=== Shell Comprehensive Test Suite ==="
echo ""

# Setup test environment
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

# Test 1: Basic prompt (visual inspection needed)
echo "Test 1: Prompt Display (check manually)"
echo "exit" | $SHELL_BIN 2>&1 | head -1
echo ""

# Test 2: Invalid syntax detection
echo "Test 2: Invalid Syntax Detection"
result=$(echo "cat meow.txt | ; meow" | $SHELL_BIN 2>&1 | grep "Invalid Syntax!")
if [ -n "$result" ]; then
    echo -e "${GREEN}PASS${NC}: Invalid syntax detected"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}: Should detect invalid syntax"
    ((FAILED++))
fi

# Test 3: Valid syntax acceptance
echo "Test 3: Valid Syntax Acceptance"
result=$(echo "cat meow.txt | meow; meow > meow.txt &" | $SHELL_BIN 2>&1 | grep "Invalid Syntax!")
if [ -z "$result" ]; then
    echo -e "${GREEN}PASS${NC}: Valid syntax accepted"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}: Valid syntax rejected"
    ((FAILED++))
fi

# Test 4: hop to home
echo "Test 4: hop (home directory)"
mkdir -p testdir
result=$(echo -e "hop testdir\nhop ~\npwd" | $SHELL_BIN 2>&1 | tail -2 | head -1)
if [[ "$result" == *"$HOME"* ]]; then
    echo -e "${GREEN}PASS${NC}: hop ~ works"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}: hop ~ failed"
    ((FAILED++))
fi

# Test 5: hop with multiple args
echo "Test 5: hop with multiple arguments"
# Should navigate through multiple directories
echo "Skip for now (complex to test)"

# Test 6: reveal basic
echo "Test 6: reveal (basic listing)"
touch file1.txt file2.txt .hidden
result=$(echo "reveal" | $SHELL_BIN 2>&1 | grep "file1.txt")
if [ -n "$result" ]; then
    echo -e "${GREEN}PASS${NC}: reveal shows files"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}: reveal doesn't show files"
    ((FAILED++))
fi

# Test 7: reveal -a (show hidden)
echo "Test 7: reveal -a (show hidden files)"
result=$(echo "reveal -a" | $SHELL_BIN 2>&1 | grep ".hidden")
if [ -n "$result" ]; then
    echo -e "${GREEN}PASS${NC}: reveal -a shows hidden files"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}: reveal -a doesn't show hidden"
    ((FAILED++))
fi

# Test 8: reveal -l (long format)
echo "Test 8: reveal -l (line by line)"
result=$(echo "reveal -l" | $SHELL_BIN 2>&1)
lines=$(echo "$result" | grep -E "file[12].txt" | wc -l)
if [ "$lines" -ge 2 ]; then
    echo -e "${GREEN}PASS${NC}: reveal -l shows multiple lines"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}: reveal -l format wrong"
    ((FAILED++))
fi

echo ""
echo "=== Test Summary ==="
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"

cd ..
rm -rf "$TEST_DIR"
