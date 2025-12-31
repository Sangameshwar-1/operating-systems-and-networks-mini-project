#!/bin/bash
echo "=== Quick Manual Tests ==="

# Test 1: Simple echo
echo "Test 1: Simple echo command"
echo "echo hello" | timeout 2 ./shell.out 2>&1 | tail -2

# Test 2: Input redirection  
echo "Test 2: Input redirection"
echo "This is test content" > input.txt
echo "cat < input.txt" | timeout 2 ./shell.out 2>&1 | grep "test content"

# Test 3: Output redirection
echo "Test 3: Output redirection"
echo "echo test > output.txt" | timeout 2 ./shell.out 2>&1
cat output.txt

# Test 4: Pipe
echo "Test 4: Pipe"
echo "echo hello world | grep world" | timeout 2 ./shell.out 2>&1 | grep world

# Test 5: Background execution
echo "Test 5: Background execution"
echo "sleep 1 &" | timeout 3 ./shell.out 2>&1 | grep "\[.*\]"

echo "Tests complete"
