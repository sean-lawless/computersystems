;@ Set boot stack address and branch to BOOT_BASE_ADDR to execute
;@ bootloader.
.globl _branch_to_boot
_branch_to_boot:
    mov sp, #BOOT_STACK_ADDR
    mov r0, #BOOT_BASE_ADDR
    bx r0

;@ Set run stack address and branch to RUN_BASE_ADDR to execute
;@ application transferred to RAM with xmodem.
.globl _branch_to_run
_branch_to_run:
    mov sp, #RUN_STACK_ADDR
    mov r0, #RUN_BASE_ADDR
    bx r0

;@ Run location is the RUN_BASE_ADDR
.globl _run_location
_run_location:
    mov r0, #RUN_BASE_ADDR
    bx lr

;@ Run size is the boot base minus the run base
.globl _run_size
_run_size:
    mov r0, #BOOT_BASE_ADDR
    sub r0, #RUN_BASE_ADDR
    bx lr
