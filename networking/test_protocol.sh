#!/bin/bash

# S.H.A.M. Protocol Test Script
# Tests various aspects of the protocol implementation

echo "======================================"
echo "S.H.A.M. Protocol Test Suite"
echo "======================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if executables exist
if [ ! -f "./server" ] || [ ! -f "./client" ]; then
    echo -e "${RED}Error: server or client not found. Run 'make' first.${NC}"
    exit 1
fi

# Function to check if server is running
check_server() {
    sleep 1
    if ps aux | grep -v grep | grep "./server" > /dev/null; then
        return 0
    else
        return 1
    fi
}

# Function to stop server
stop_server() {
    pkill -f "./server" 2>/dev/null
    sleep 1
}

# Test 1: Basic File Transfer
echo -e "${YELLOW}Test 1: Basic File Transfer (No Loss)${NC}"
echo "Creating test file..."
echo "This is a test message for S.H.A.M. protocol!" > test1.txt
echo "File content: $(cat test1.txt)"

echo "Starting server on port 8080..."
./server 8080 > server_output.txt 2>&1 &
SERVER_PID=$!
sleep 1

echo "Starting client to send file..."
./client 127.0.0.1 8080 test1.txt output1.txt > client_output.txt 2>&1

echo "Waiting for transfer to complete..."
sleep 2

# Check results
if [ -f "received_file" ]; then
    echo -e "${GREEN}✓ File received successfully${NC}"
    echo "Server MD5:"
    grep "MD5:" server_output.txt
    echo "Original MD5:"
    md5sum test1.txt | awk '{print "MD5: " $1}'
else
    echo -e "${RED}✗ File transfer failed${NC}"
fi

stop_server
echo ""

# Test 2: File Transfer with Packet Loss
echo -e "${YELLOW}Test 2: File Transfer with 10% Packet Loss${NC}"
echo "Creating larger test file..."
dd if=/dev/urandom of=test2.bin bs=1K count=50 2>/dev/null
echo "Test file size: $(ls -lh test2.bin | awk '{print $5}')"

echo "Starting server with 10% packet loss..."
./server 8081 0.1 > server_output2.txt 2>&1 &
SERVER_PID=$!
sleep 1

echo "Starting client with 10% packet loss..."
export RUDP_LOG=1
./client 127.0.0.1 8081 test2.bin output2.bin 0.1 > client_output2.txt 2>&1
export RUDP_LOG=0

echo "Waiting for transfer to complete..."
sleep 2

# Check results
if [ -f "received_file" ]; then
    echo -e "${GREEN}✓ File received successfully with packet loss${NC}"
    echo "Server MD5:"
    grep "MD5:" server_output2.txt
    echo "Original MD5:"
    md5sum test2.bin | awk '{print "MD5: " $1}'
    
    # Check for retransmissions in log
    if [ -f "client_log.txt" ]; then
        RETX_COUNT=$(grep -c "RETX" client_log.txt)
        echo "Retransmissions detected: $RETX_COUNT"
    fi
else
    echo -e "${RED}✗ File transfer failed${NC}"
fi

stop_server
echo ""

# Test 3: Large File Transfer
echo -e "${YELLOW}Test 3: Large File Transfer (1MB)${NC}"
echo "Creating 1MB test file..."
dd if=/dev/urandom of=test3.bin bs=1M count=1 2>/dev/null
echo "Test file size: $(ls -lh test3.bin | awk '{print $5}')"

echo "Starting server..."
./server 8082 > server_output3.txt 2>&1 &
SERVER_PID=$!
sleep 1

echo "Starting client..."
START_TIME=$(date +%s)
./client 127.0.0.1 8082 test3.bin output3.bin > client_output3.txt 2>&1
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

echo "Waiting for transfer to complete..."
sleep 2

# Check results
if [ -f "received_file" ]; then
    echo -e "${GREEN}✓ Large file received successfully${NC}"
    echo "Transfer time: ${DURATION} seconds"
    echo "Server MD5:"
    grep "MD5:" server_output3.txt
    echo "Original MD5:"
    md5sum test3.bin | awk '{print "MD5: " $1}'
else
    echo -e "${RED}✗ Large file transfer failed${NC}"
fi

stop_server
echo ""

# Test 4: Multiple Sequential Transfers
echo -e "${YELLOW}Test 4: Multiple Sequential Transfers${NC}"
for i in 1 2 3; do
    echo "Transfer $i of 3..."
    echo "Test message $i" > test4_$i.txt
    
    ./server 808$((2+i)) > server_output4_$i.txt 2>&1 &
    SERVER_PID=$!
    sleep 1
    
    ./client 127.0.0.1 808$((2+i)) test4_$i.txt output4_$i.txt > client_output4_$i.txt 2>&1
    sleep 2
    
    if [ -f "received_file" ]; then
        echo -e "  ${GREEN}✓ Transfer $i successful${NC}"
    else
        echo -e "  ${RED}✗ Transfer $i failed${NC}"
    fi
    
    stop_server
done
echo ""

# Summary
echo "======================================"
echo "Test Summary"
echo "======================================"
echo ""
echo "Check the following files for details:"
echo "  - server_output*.txt : Server console output"
echo "  - client_output*.txt : Client console output"
echo "  - server_log.txt     : Server debug log (if RUDP_LOG=1)"
echo "  - client_log.txt     : Client debug log (if RUDP_LOG=1)"
echo ""
echo "Cleanup test files with:"
echo "  rm -f test*.txt test*.bin output*.txt output*.bin received_file"
echo "  rm -f server_output*.txt client_output*.txt"
echo "  rm -f server_log.txt client_log.txt"
echo ""
echo "======================================"
echo "Tests Complete!"
echo "======================================"
