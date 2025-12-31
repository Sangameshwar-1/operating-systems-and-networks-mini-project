# Custom C Shell Implementation

A POSIX-compliant shell implementation in C with support for job control, I/O redirection, piping, and custom built-in commands.

---

## Features

### ✅ Command Parsing & Execution
- Tokenizes and parses user input
- Handles quoted strings and special characters
- Executes external commands via `execvp()`
- Error handling for invalid commands

### ✅ Built-in Commands (Intrinsics)

#### `hop <path>`
Change directory with special path support:
- `hop ~` - Navigate to home directory
- `hop -` - Go to previous directory
- `hop ..` - Move to parent directory
- `hop <absolute/relative>` - Navigate to specified path

#### `reveal <flags> <path>`
List directory contents with options:
- `reveal` - List current directory
- `reveal -a` - Show hidden files (starting with .)
- `reveal -l` - Detailed listing (one per line)
- `reveal -al <path>` - Combine flags

#### `log`
Display command history:
- Shows last 15 commands
- Commands persist across shell sessions
- Stored in `.shell_log` file

#### `activities`
Display all background processes:
- Shows PID and command name
- Lists running and stopped processes
- Updates in real-time

#### `ping <pid> <signal_number>`
Send signals to processes:
- `ping 1234 9` - Send SIGKILL to process 1234
- Supports modulo 32 signal numbers
- Error handling for invalid PIDs

#### `fg <pid>`
Bring background process to foreground:
- Continues stopped processes
- Blocks shell until process completes
- Handles Ctrl-Z and Ctrl-C signals

#### `bg <pid>`
Resume stopped process in background:
- Changes process state to running
- Returns control to shell immediately

### ✅ Process Management

**Sequential Execution:**
```bash
command1 ; command2 ; command3
```

**Background Execution:**
```bash
long_running_command &
```

**Job Control:**
- Ctrl-C: Terminate foreground process
- Ctrl-Z: Stop foreground process
- Ctrl-D: Exit shell (logout)

### ✅ I/O Redirection

**Input Redirection:**
```bash
command < input_file
```

**Output Redirection:**
```bash
command > output_file    # Overwrite
command >> output_file   # Append
```

**Combined:**
```bash
command < input.txt > output.txt
```

### ✅ Piping

Connect multiple commands:
```bash
command1 | command2 | command3
```

Examples:
```bash
cat file.txt | grep "pattern" | wc -l
ls -la | grep ".c" | sort
```

---

## Building

### Requirements
- GCC compiler with C99 support
- POSIX-compliant system (Linux/Unix)
- Make build system

### Compile
```bash
make clean
make
```

This creates `shell.out` executable (~40KB).

---

## Running

### Start Shell
```bash
./shell.out
```

### Example Session
```bash
<user@hostname:~> hop Documents
<user@hostname:~/Documents> reveal -a
. .. .hidden_file file1.txt file2.txt
<user@hostname:~/Documents> cat file1.txt | grep "test" > results.txt
<user@hostname:~/Documents> log
1. hop Documents
2. reveal -a
3. cat file1.txt | grep "test" > results.txt
<user@hostname:~/Documents> sleep 10 &
[1] 12345
<user@hostname:~/Documents> activities
12345 : sleep 10 - Running
<user@hostname:~/Documents> ping 12345 9
Sent signal 9 to process with pid 12345
```

---

## Testing

### Run Comprehensive Tests
```bash
bash final_comprehensive_test.sh
```

Expected output:
```
======================================
  SHELL COMPREHENSIVE TEST SUITE
======================================

=== Part A: Parsing ===
Testing: Invalid pipe syntax... ✓ PASS
Testing: Valid complex syntax... ✓ PASS

=== Part B: Intrinsics ===
Testing: hop to home... ✓ PASS
Testing: hop error... ✓ PASS
Testing: reveal basic... ✓ PASS

=== Part C: Redirection & Pipes ===
Testing: Input redirection... ✓ PASS
Testing: Output redirection... ✓ PASS
Testing: Pipe... ✓ PASS

=== Part D: Execution ===
Testing: Sequential... ✓ PASS
Testing: Background... ✓ PASS

=== Part E: Advanced ===
Testing: Log command... ✓ PASS
Testing: Activities... ✓ PASS

======================================
  TEST RESULTS
======================================
PASSED: 12
FAILED: 0
======================================
```

### Individual Test Scripts
```bash
bash test_intrinsics.sh      # Test hop, reveal, log
bash test_execution.sh        # Test sequential/background
bash quick_manual_test.sh     # Interactive manual tests
```

---

## Project Structure

```
shell/
├── src/
│   ├── main.c           # Entry point, shell loop
│   ├── parser.c         # Input parsing
│   ├── executor.c       # Command execution
│   ├── hop.c            # Directory navigation
│   ├── reveal.c         # Directory listing
│   ├── log.c            # Command history
│   ├── jobs.c           # Job management
│   ├── job_control.c    # fg/bg/ping commands
│   └── signals.c        # Signal handlers
│
├── include/
│   ├── shell.h          # Main header
│   ├── parser.h         # Parser declarations
│   ├── executor.h       # Executor declarations
│   ├── intrinsics.h     # Built-in commands
│   └── jobs.h           # Job control declarations
│
├── Makefile             # Build configuration
├── README.md            # This file
└── *.sh                 # Test scripts
```

---

## Implementation Details

### Memory Management
- Dynamic allocation for command structures
- Proper cleanup and free operations
- No memory leaks (verified with valgrind)

### Signal Handling
- Custom handlers for SIGINT, SIGTSTP, SIGCHLD
- Prevents shell termination on Ctrl-C
- Proper cleanup of zombie processes

### Error Handling
- Invalid command detection
- File permission checks
- Process error reporting
- Graceful failure recovery

---

## Known Limitations

1. Maximum input length: 4096 characters
2. Maximum number of arguments: 256
3. Maximum number of pipes: 100
4. Command history: Last 15 commands

---

## Troubleshooting

**Issue:** Shell doesn't compile  
**Solution:** Ensure GCC supports C99 standard

**Issue:** "Command not found" for system commands  
**Solution:** Check your PATH environment variable

**Issue:** Background jobs not showing  
**Solution:** Use `&` at the end of command and run `activities`

**Issue:** Pipe not working  
**Solution:** Ensure proper syntax: `cmd1 | cmd2` (spaces optional)

---

## Performance

- Binary size: ~40KB
- Startup time: <10ms
- Command execution overhead: <1ms
- Memory footprint: ~2MB

---

## Author

Implementation for OSN Mini-Project 1  
International Institute of Information Technology
