#!/bin/bash
# Simple test script that properly manages server lifecycle

cd "/mnt/c/Users/SANGAMESHWAR/OneDrive - International Institute of Information Technology/Documents/COMPUTER-PROJECT/OSN/MP1/networking"

echo "Starting server..."
./server 8080 > server_test.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"

# Wait for server to be ready
sleep 1

echo ""
echo "Starting client..."
./client 127.0.0.1 8080 test.txt output.txt

# Get exit codes
CLIENT_EXIT=$?

# Wait for server to finish
wait $SERVER_PID 2>/dev/null
SERVER_EXIT=$?

echo ""
echo "=========================================="
if [ $CLIENT_EXIT -eq 0 ] && [ $SERVER_EXIT -eq 0 ]; then
    echo "✓ Test PASSED"
    echo ""
    echo "Server output:"
    cat server_test.log
    echo ""
    if [ -f "received_file" ]; then
        echo "Received file MD5:"
        md5sum received_file | awk '{print $1}'
        echo ""
        echo "Original file MD5:"
        md5sum test.txt | awk '{print $1}'
    fi
else
    echo "✗ Test FAILED"
    echo "Client exit code: $CLIENT_EXIT"
    echo "Server exit code: $SERVER_EXIT"
    echo ""
    echo "Server output:"
    cat server_test.log
fi
echo "=========================================="
