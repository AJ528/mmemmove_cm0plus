
  @ tells the assembler to use the unified instruction set
  .syntax unified
  @ this directive selects the thumb (16-bit) instruction set
  .thumb
  @ this directive specifies the following symbol is a thumb-encoded function
  .thumb_func
  @ align the next variable or instruction on a 2-byte boundary
  .align 2
  @ make the symbol visible to the linker
  .global memmove_
  @ marks the symbol as being a function name
  .type memmove_, STT_FUNC
memmove_:
@ r0 = destination addr
@ r1 = source addr
@ r2 = num bytes
@ returns destination addr in r0
  cmp   r2, #0            @ if there are 0 bytes to move
  push  {r4, r5}          @ store r4 & r5 values on stack
  beq   exit              @ exit

  add   r3, r1, r2      @ calculate final source address + 1 and store in r3
  subs  r5, r0, r1      @ subtract source addr from destination addr, update flags, and store result in r5
  blo   copy_f          @ if destination < source (source ahead), copy forward
  beq   exit            @ if source=destination, nothing to do

@ source is behind destination, check for overlap
  cmp   r0, r3          @ compare first destination addr against final source address + 1
  bhs   copy_f          @ if the first destination addr is >= final source addr + 1, there is no overlap

@ otherwise we must copy backwards
  add   r4, r0, r2      @ calculate final destination addr + 1 and store in r4
  cmp   r2, #4          @ check if there are 4 or more bytes to copy
  blo   copy_bck_single @ if not, copy one at a time
  cmp   r2, #16         @ check if there are 16 or more bytes to copy
  blo   quad_b_copy     @ if not, copy 4 bytes at a time
  tst   r5, #3          @ check if dest-source is a multiple of 4
  bne   quad_b_copy     @ if not, copy 4 bytes at a time
  tst   r1, #3          @ check if the source address is 4-byte aligned
  bne   quad_b_copy     @ if not, copy 4 bytes at a time


.balign 8                   @ align the loop to an 8 byte boundary, also force encodings for speed/alignment
  nop.n                     @ offset by 2 bytes so the LDR/STR instructions align nicely
@ if there are 16 or more bytes to copy and the src and dest are 4-byte aligned, can copy word-wise
quad_word_b_copy:
  sub.w   r2, r2, #16       @ decrement remaining bytes by 4
  cmp.n   r2, #16           @ check if there are 16 or more bytes to copy
  ldr.w   r5, [r3, #-4]!    @ load word from memory[r3-4] into r5. r3 is decremented by 4
  str.w   r5, [r4, #-4]!    @ store r5 word into memory[r4-4]. r4 is decremented by 4
  ldr.w   r5, [r3, #-4]!    @ repeat 3 more times
  str.w   r5, [r4, #-4]!
  ldr.w   r5, [r3, #-4]!
  str.w   r5, [r4, #-4]!
  ldr.w   r5, [r3, #-4]!
  str.w   r5, [r4, #-4]!
  bhs.n   quad_word_b_copy  @ if there are 16+ bytes left, quad word copy again
  cmp   r2, #4              @ if there are 4 or more bytes to copy
  bhs   quad_b_copy
  cmp   r3, r1              @ check if we are at the final source address
  bne   copy_bck_single     @ if not, finish with single byte copying
  b     exit                @ otherwise, exit


.balign 8                 @ align the loop to an 8 byte boundary, also force encodings for speed/alignment
  nop.n                   @ offset by 2 bytes so the LDR/STR instructions align nicely
quad_b_copy:              @ copy backwards 4 bytes at a time
  sub.w   r2, r2, #4      @ decrement remaining bytes by 4
  cmp.n   r2, #4          @ check if there are 4 or more bytes to copy
  ldrb.w  r5, [r3, #-1]!  @ load byte from memory[r3-1] into r5. r3 is decremented by 1
  strb.w  r5, [r4, #-1]!  @ store r5 byte into memory[r4-1]. r4 is decremented by 1
  ldrb.w  r5, [r3, #-1]!  @ repeat 3 more times
  strb.w  r5, [r4, #-1]!
  ldrb.w  r5, [r3, #-1]!
  strb.w  r5, [r4, #-1]!
  ldrb.w  r5, [r3, #-1]!
  strb.w  r5, [r4, #-1]!
  bhs.n   quad_b_copy     @ if so, quad copy again
  
  cmp   r3, r1          @ check if we are at the final source address
  beq     exit          @ if so, exit
@ otherwise, there are <4 bytes left to copy

copy_bck_single: 
  ldrb  r5, [r3, #-1]!  @ load byte from memory[r3-1] into r5. r3 is updated to r3-1
  strb  r5, [r4, #-1]!  @ store r5 byte into memory[r4-1]. r4 is updated to r4-1
  cmp   r1, r3          @ check if we are at the first source address
  bne   copy_bck_single @ if not done, repeat
  b     exit

@ copy forwards
copy_f:                 @ copy from beginning of source and work forwards
  mov   r4, r0          @ copy destination addr into r4
  cmp   r2, #4          @ check if there are 4 or more bytes to copy
  blo   copy_fwd_single @ if not, copy one at a time
  cmp   r2, #16         @ check if there are 16 or more bytes to copy
  blo   quad_f_copy     @ if not, copy 4 bytes at a time
  tst   r5, #3          @ check if dest-source is a multiple of 4
  bne   quad_f_copy     @ if not, copy 4 bytes at a time
  tst   r1, #3          @ check if the source address is 4-byte aligned
  bne   quad_f_copy     @ if not, copy 4 bytes at a time

.balign 8               @ align the loop to an 8 byte boundary, also force encodings for speed/alignment
@ if there are 16 or more bytes to copy and the src and dest are 4-byte aligned, can copy word-wise
quad_word_f_copy:
  sub.w   r2, r2, #16     @ decrement remaining bytes by 16
  cmp.w   r2, #16         @ check if there are 16 or more bytes to copy
  ldr.w  r5, [r1], #4     @ load byte from memory[r1] into r5. r1 is incremented by 4
  str.w  r5, [r4], #4     @ store r5 byte into memory[r4]. r4 is incremented by 4
  ldr.w  r5, [r1], #4     @ repeat 3 more times
  str.w  r5, [r4], #4
  ldr.w  r5, [r1], #4
  str.w  r5, [r4], #4
  ldr.w  r5, [r1], #4
  str.w  r5, [r4], #4
  bhs.n   quad_word_f_copy  @ if there are 16+ bytes left, quad word copy again
  cmp   r2, #4              @ if there are 4 or more bytes to copy
  bhs   quad_f_copy
  cmp   r3, r1            @ check if we are at the final source address
  bne   copy_fwd_single   @ if not, finish with single byte copying
  b     exit              @ otherwise, exit

.balign 4                 @ align the loop to a 4 byte boundary, also force encodings for speed/alignment
                          @ I'm not sure why, but quad_f_copy was the only loop that didn't benefit from 8 byte alignment
quad_f_copy:
  sub.w   r2, r2, #4      @ decrement remaining bytes by 4
  cmp.w   r2, #4          @ check if there are 4 or more bytes to copy
  ldrb.w  r5, [r1], #1    @ load byte from memory[r1] into r5. r1 is incremented by 1
  strb.w  r5, [r4], #1    @ store r5 byte into memory[r4]. r4 is incremented by 1
  ldrb.w  r5, [r1], #1    @ repeat 3 more times
  strb.w  r5, [r4], #1
  ldrb.w  r5, [r1], #1
  strb.w  r5, [r4], #1
  ldrb.w  r5, [r1], #1
  strb.w  r5, [r4], #1
  bhs.n   quad_f_copy     @ if so, quad copy again

  cmp   r3, r1          @ check if we are at the final source address
  beq   exit            @ if so, exit
@ otherwise, there are <4 bytes left to copy forward

copy_fwd_single:
  ldrb  r5, [r1], #1    @ load byte from memory[r1] into r5. r1 is updated to r1+1
  strb  r5, [r4], #1    @ store r5 byte into memory[r4]. r4 is updated to r4+1
  cmp   r3, r1          @ check if we are at the final source address
  bne   copy_fwd_single @ if not done, repeat

exit:
  pop   {r4, r5}        @ restore previous value of r4 & r5
  bx    lr              @ exit function

  .size memmove_, . - memmove_

