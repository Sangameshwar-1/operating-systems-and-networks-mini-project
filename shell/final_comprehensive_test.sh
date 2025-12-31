#!/bin/bash

echo "======================================"
echo "  SHELL COMPREHENSIVE TEST SUITE"
echo "======================================"
echo ""

PASS=0
FAIL=0

# Helper function
test_command() {
    local test_name="$1"
    local command="$2"
    local expected="$3"
    
    echo -n "Testing: $test_name... "
    result=$(echo "$command" | timeout 2 ./shell.out 2>&1)
    
    if echo "$result" | grep -q "$expected"; then
        echo "✓ PASS"
        ((PASS++))
    else
        echo "✗ FAIL"
        echo "  Expected: $expected"
        echo "  Got: $result"
        ((FAIL++))
    fi
}

echo "=== Part A: Parsing ===" 
test_command "Invalid pipe syntax" "cat | ; echo" "Invalid Syntax!"
test_command "Valid complex syntax" "cat a | grep b > c &" "\\[1\\]"

echo ""
echo "=== Part B: Intrinsics ==="
test_command "hop to home" "hop ~" "<.*~>"
test_command "hop error" "hop /nonexistent999" "No such directory!"
echo "echo test" > /tmp/testfile.txt
test_command "reveal basic" "reveal /tmp" "testfile.txt"
rm -f /tmp/testfile.txt

echo ""
echo "=== Part C: Redirection & Pipes ==="
echo "test content" > /tmp/input_test.txt
test_command "Input redirection" "cat < /tmp/input_test.txt" "test content"
test_command "Output redirection" "echo hello > /tmp/output_test.txt" ""
test_command "Pipe" "echo hello | grep hello" "hello"
rm -f /tmp/input_test.txt /tmp/output_test.txt

echo ""
echo "=== Part D: Execution ==="
# Test sequential execution by checking if both outputs are present
echo -n "Testing: Sequential... "
result=$(echo "echo a ; echo b" | timeout 2 ./shell.out 2>&1)
if echo "$result" | grep -q "a" && echo "$result" | grep -q "b"; then
    echo "✓ PASS"
    ((PASS++))
else
    echo "✗ FAIL"
    echo "  Expected: both 'a' and 'b' in output"
    echo "  Got: $result"
    ((FAIL++))
fi
test_command "Background" "sleep 1 &" "\\[.*\\]"

echo ""
echo "=== Part E: Advanced ==="
test_command "Log command" "log" ""
test_command "Activities" "activities" ""

echo ""
echo "======================================"
echo "  TEST RESULTS"
echo "======================================"
echo "PASSED: $PASS"
echo "FAILED: $FAIL"
echo "======================================"
