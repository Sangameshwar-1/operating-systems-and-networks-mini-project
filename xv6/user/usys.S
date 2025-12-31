.globl getreadcount
getreadcount:
    li a7, 23 # syscall number for getreadcount
    ecall
    ret
