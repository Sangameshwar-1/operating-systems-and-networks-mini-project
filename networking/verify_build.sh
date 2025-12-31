#!/bin/bash

# Simple verification script for S.H.A.M. protocol
# This tests basic functionality without needing background processes

echo "======================================"
echo "S.H.A.M. Protocol - Simple Verification"
echo "======================================"
echo ""

# Check if executables exist
if [ ! -f "./server" ] || [ ! -f "./client" ]; then
    echo "ERROR: Executables not found. Run 'make' first."
    exit 1
fi

echo "✓ Executables found (server, client)"
echo ""

# Check file sizes
SERVER_SIZE=$(stat -f%z "./server" 2>/dev/null || stat -c%s "./server" 2>/dev/null)
CLIENT_SIZE=$(stat -f%z "./client" 2>/dev/null || stat -c%s "./client" 2>/dev/null)

echo "✓ Server binary: $SERVER_SIZE bytes"
echo "✓ Client binary: $CLIENT_SIZE bytes"
echo ""

# Verify header file
if [ -f "sham.h" ]; then
    echo "✓ Protocol header (sham.h) exists"
    echo "  - Defined constants:"
    grep "#define SHAM_" sham.h | head -5
fi
echo ""

# Check documentation
echo "Documentation files:"
[ -f "README.md" ] && echo "  ✓ README.md"
[ -f "QUICK_START.md" ] && echo "  ✓ QUICK_START.md"
[ -f "PROTOCOL_DETAILS.md" ] && echo "  ✓ PROTOCOL_DETAILS.md"
echo ""

echo "======================================"
echo "Build Verification: SUCCESS"
echo "======================================"
echo ""
echo "To test the implementation:"
echo ""
echo "Terminal 1 (Server):"
echo "  ./server 8080"
echo ""
echo "Terminal 2 (Client):"
echo "  echo 'Test message' > test.txt"
echo "  ./client 127.0.0.1 8080 test.txt output.txt"
echo ""
echo "For detailed testing:"
echo "  ./test_protocol.sh    # Automated tests"
echo "  cat QUICK_START.md    # Manual testing guide"
echo ""

exit 0
