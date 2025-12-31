# S.H.A.M. Protocol Implementation

## Simple Handshake and Messaging Protocol

This is a reliable data transfer protocol built on top of UDP, implementing features similar to TCP including:
- 3-way handshake for connection establishment
- 4-way handshake for connection termination
- Sliding window flow control
- Cumulative acknowledgments
- Timeout and retransmission
- Packet loss simulation

## Project Structure

```
networking/
├── sham.h          # Protocol header definitions and constants
├── server.c        # Server implementation
├── client.c        # Client implementation
├── Makefile        # Build configuration
└── README.md       # This file
```

## Protocol Features

### 1. Connection Establishment (3-Way Handshake)
- Client sends SYN packet with initial sequence number
- Server responds with SYN-ACK
- Client sends ACK to complete handshake

### 2. Data Transfer
- Sliding window protocol with configurable window size (default: 10 packets)
- Each packet can carry up to 1024 bytes of data
- Cumulative acknowledgments
- Timeout-based retransmission (default: 500ms)
- Maximum retry limit (default: 10 attempts)

### 3. Flow Control
- Window size advertisement in every packet
- Receiver-side buffer management
- Prevents sender from overwhelming receiver

### 4. Connection Termination (4-Way Handshake)
- Client sends FIN packet
- Server acknowledges with ACK
- Server sends its own FIN
- Client sends final ACK

### 5. Packet Loss Simulation
- Configurable packet loss rate for testing
- Dropped packets trigger timeout and retransmission

## Building the Project

```bash
make clean
make
```

This will compile both `server` and `client` executables.

## Usage

### File Transfer Mode

**Server:**
```bash
./server <port> [loss_rate]
```
- `port`: Port number to listen on
- `loss_rate`: Optional packet loss rate (0.0 to 1.0)

Example:
```bash
./server 8080          # No packet loss
./server 8080 0.1      # 10% packet loss
```

**Client:**
```bash
./client <server_ip> <server_port> <input_file> <output_file_name> [loss_rate]
```
- `server_ip`: IP address of the server
- `server_port`: Port number of the server
- `input_file`: File to send
- `output_file_name`: Name for the received file on server
- `loss_rate`: Optional packet loss rate (0.0 to 1.0)

Example:
```bash
./client 127.0.0.1 8080 test.txt received_test.txt
./client 127.0.0.1 8080 large_file.dat output.dat 0.05  # 5% loss
```

### Chat Mode

**Server:**
```bash
./server <port> --chat [loss_rate]
```

**Client:**
```bash
./client <server_ip> <server_port> --chat [loss_rate]
```

In chat mode:
- Type messages and press Enter to send
- Type `/quit` to exit

Example:
```bash
# Terminal 1 (Server)
./server 8080 --chat

# Terminal 2 (Client)
./client 127.0.0.1 8080 --chat
```

## Testing

### Basic File Transfer Test
```bash
# Terminal 1 (Server)
./server 8080

# Terminal 2 (Client)
echo "Hello World!" > test.txt
./client 127.0.0.1 8080 test.txt output.txt
```

### Test with Packet Loss
```bash
# Terminal 1 (Server with 10% loss)
./server 8080 0.1

# Terminal 2 (Client with 10% loss)
./client 127.0.0.1 8080 test.txt output.txt 0.1
```

### Large File Transfer
```bash
# Create a large test file (10MB)
dd if=/dev/urandom of=large_file.bin bs=1M count=10

# Terminal 1 (Server)
./server 8080

# Terminal 2 (Client)
./client 127.0.0.1 8080 large_file.bin received_file.bin
```

### Chat Mode Test
```bash
# Terminal 1 (Server)
./server 8080 --chat

# Terminal 2 (Client)
./client 127.0.0.1 8080 --chat
# Type messages and press Enter
# Type /quit to exit
```

## Logging

Set the `RUDP_LOG` environment variable to enable detailed logging:

```bash
export RUDP_LOG=1

# Now run server/client
./server 8080
./client 127.0.0.1 8080 test.txt output.txt
```

Log files are generated:
- `server_log.txt` - Server-side events
- `client_log.txt` - Client-side events

Log entries include:
- Timestamps with microsecond precision
- Packet send/receive events
- Sequence numbers and acknowledgments
- Timeout and retransmission events
- Window size updates

## Protocol Constants

Defined in `sham.h`:

```c
#define SHAM_DATA_SIZE 1024        // Max data per packet (bytes)
#define SHAM_WINDOW_SIZE 10        // Sliding window size (packets)
#define SHAM_TIMEOUT_MS 500        // Retransmission timeout (ms)
#define SHAM_MAX_RETRIES 10        // Max retransmission attempts
```

## Packet Structure

### Header (12 bytes)
```c
struct sham_header {
    uint32_t seq_num;      // Sequence number (byte-based)
    uint32_t ack_num;      // Acknowledgment number
    uint16_t flags;        // Control flags (SYN, ACK, FIN)
    uint16_t window_size;  // Flow control window
};
```

### Flags
- `SHAM_SYN (0x1)`: Synchronize - initiate connection
- `SHAM_ACK (0x2)`: Acknowledge
- `SHAM_FIN (0x4)`: Finish - terminate connection

### Complete Packet
```c
struct sham_packet {
    struct sham_header header;  // 12 bytes
    uint8_t data[1024];         // 1024 bytes
};
```

## MD5 Checksum Verification

After file transfer, the server calculates and displays the MD5 checksum of the received file. You can verify file integrity:

```bash
# On client side (original file)
md5sum test.txt

# On server side (received file)
md5sum received_file
```

The checksums should match if the transfer was successful.

## Implementation Details

### Sliding Window Protocol

The client maintains a sliding window of packets:
- Window base: First unacknowledged packet
- Next sequence number: Next packet to send
- Window size: Maximum packets in flight

When an ACK is received:
- Window slides forward (cumulative acknowledgment)
- New packets can be sent

On timeout:
- Unacknowledged packets are retransmitted
- Retry counter increments
- Connection fails after max retries

### Flow Control

The receiver advertises its available buffer space in the window_size field of every packet. The sender respects this limit and won't send more data than the receiver can handle.

### Error Handling

- **Timeout**: Packets not acknowledged within 500ms are retransmitted
- **Max Retries**: Connection terminates after 10 failed attempts
- **Invalid Packets**: Packets smaller than header size are discarded
- **Sequence Numbers**: Out-of-order packets are discarded (simplified implementation)

## Limitations

1. **Simplified Ordering**: Out-of-order packets are not buffered (receiver expects in-order delivery)
2. **Single Connection**: Server handles one client at a time
3. **No Congestion Control**: Fixed timeout and window size (no TCP-style congestion control)
4. **Packet Loss Simulation**: Both sender and receiver can drop packets independently

## Compilation Requirements

- GCC with C99 support
- OpenSSL library (`libcrypto`) for MD5 checksums
- POSIX-compliant system (Linux, macOS, WSL)

## Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# macOS
brew install openssl
```

## Troubleshooting

### Connection Timeout
- Check if server is running and listening on correct port
- Verify IP address and port number
- Check firewall settings

### Compilation Errors
- Ensure OpenSSL development headers are installed
- Check GCC version (requires C99 support)

### File Transfer Fails
- Check file permissions
- Verify sufficient disk space
- Try reducing packet loss rate
- Check logs with `RUDP_LOG=1`

## Performance Notes

- Default window size (10 packets) provides good performance for most network conditions
- Timeout of 500ms works well for local networks; may need adjustment for high-latency networks
- Large files transfer efficiently due to sliding window protocol
- Packet loss up to 20% is generally handled well with retransmissions

## Future Enhancements

Possible improvements:
- Selective acknowledgment (SACK) for out-of-order packets
- Dynamic timeout adjustment (like TCP's RTT estimation)
- Congestion control mechanisms
- Multiple simultaneous connections
- Encryption support
- Checksums for data integrity

## Author

Implementation of the S.H.A.M. (Simple Handshake and Messaging) Protocol for reliable data transfer over UDP.
