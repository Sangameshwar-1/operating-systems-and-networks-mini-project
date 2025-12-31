#!/bin/bash
echo "=== Testing Intrinsics ==="

# Test hop
echo "Test: hop ~"
echo -e "hop ~\npwd" | timeout 2 ./shell.out 2>&1 | tail -3

echo ""
echo "Test: hop .. then hop -"
mkdir -p testdir
echo -e "hop testdir\nhop ..\nhop -\npwd" | timeout 2 ./shell.out 2>&1 | tail -3

echo ""
echo "Test: hop to non-existent"
echo -e "hop /nonexistent123" | timeout 2 ./shell.out 2>&1 | grep "No such directory"

echo ""
echo "Test: reveal basic"
touch file1.txt file2.txt
echo "reveal" | timeout 2 ./shell.out 2>&1 | tail -2

echo ""
echo "Test: reveal -a (should show hidden)"
touch .hidden
echo "reveal -a" | timeout 2 ./shell.out 2>&1 | tail -2 | head -1

echo ""
echo "Test: reveal -l (line by line)"
echo "reveal -l" | timeout 2 ./shell.out 2>&1 | tail -5 | head -3

# Cleanup
rm -f file1.txt file2.txt .hidden
rm -rf testdir

echo ""
echo "Tests complete"
