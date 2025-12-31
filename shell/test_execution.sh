#!/bin/bash
echo "=== Testing Sequential and Background Execution ==="

# Test sequential execution
echo "Test 1: Sequential with ;"
echo "echo first ; echo second ; echo third" | timeout 2 ./shell.out 2>&1 | grep -E "(first|second|third)"

echo ""
echo "Test 2: Background execution"
echo "sleep 2 &" | timeout 1 ./shell.out 2>&1 | grep -E "\[.*\]"

echo ""
echo "Test 3: Mixed ; and &"
echo "echo start ; sleep 1 & echo end" | timeout 2 ./shell.out 2>&1 | grep -E "(start|end|\[.*\])"

echo ""
echo "Tests complete"
