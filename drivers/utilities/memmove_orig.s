
  @ tells the assembler to use the unified instruction set
  .syntax unified
  @ this directive selects the thumb (16-bit) instruction set
  .thumb
  @ this directive specifies the following symbol is a thumb-encoded function
  .thumb_func
  @ align the next variable or instruction on a 2-byte boundary
  .align 2
  @ make the symbol visible to the linker
  .global memmove_orig
  @ marks the symbol as being a function name
  .type memmove_orig, STT_FUNC
memmove_orig:
	@ r0 = destination addr
	@ r1 = source addr
	@ r2 = num bytes
	@ returns destination addr in r0

  push {r4}             @ store r4 value on stack
  cmp r0, r1            @ determine if source addr is ahead or behind destination
  bls 3f                @ if source is ahead of destination (or same), goto 3
                        @ otherwise source is behind destination
  adds  r3, r1, r2      @ calculate final source address + 1 and store in r3
  cmp   r0, r3          @ compare first destination addr against final source address
  bcs   3f              @ if the first destination addr is >= final source addr + 1, goto 3
  adds  r1, r0, r2      @ calculate final destination addr + 1 and store in r1
  cmp   r2, #0          @ if there are 0 bytes to move
  beq   2f              @ exit
  subs  r2, r3, r2      @ calculate first source address and store in r2
1:                      @ now we copy from the end of source and work backwards
  ldrb  r4, [r3, #-1]!  @ load byte from memory[r3-1] into r4. r3 is updated to r3-1
  cmp   r2, r3          @ check if we are at the first source address
  strb  r4, [r1, #-1]!  @ store r4 byte into memory[r1-1]. r1 is updated to r1-1
  bne   1b              @ if not done, repeat
2:
  pop   {r4}            @ restore previous value of r4
  bx    lr              @ exit function
3:
  cmp   r2, #0          @ if there are 0 bytes to move
  beq   2b              @ exit
  add   r2, r2, r1      @ calculate the final source address + 1 and store in r2
  subs  r3, r0, #1      @ subtract 1 from the first destination address and store in r3
4:
  ldrb  r4, [r1], #1    @ load byte from memory[r1] into r4. r1 is updated to r1+1
  cmp   r2, r1          @ check if we are at the final source address
  strb  r4, [r3, #1]!   @ store r4 byte into memory[r3+1]. r3 is updated to r3+1
  bne   4b              @ if not done, repeat
  pop   {r4}            @ restore previous value of r4
  bx    lr              @ exit function

  .size memmove_orig, . - memmove_orig

