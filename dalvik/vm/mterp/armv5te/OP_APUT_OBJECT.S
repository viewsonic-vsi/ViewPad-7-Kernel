%verify "executed"
    /*
     * Store an object into an array.  vBB[vCC] <- vAA.
     *
     * Note: using the usual FETCH/and/shift stuff, this fits in exactly 17
     * instructions.  We use a pair of FETCH_Bs instead.
     */
    /* op vAA, vBB, vCC */
    FETCH(r0, 1)                        @ r0<- CCBB
    mov     r9, rINST, lsr #8           @ r9<- AA
    and     r2, r0, #255                @ r2<- BB
    mov     r3, r0, lsr #8              @ r3<- CC
    GET_VREG(r1, r2)                    @ r1<- vBB (array object)
    GET_VREG(r0, r3)                    @ r0<- vCC (requested index)
    cmp     r1, #0                      @ null array object?
    GET_VREG(r9, r9)                    @ r9<- vAA
    beq     common_errNullObject        @ yes, bail
    ldr     r3, [r1, #offArrayObject_length]    @ r3<- arrayObj->length
    add     r10, r1, r0, lsl #2         @ r10<- arrayObj + index*width
    cmp     r0, r3                      @ compare unsigned index, length
    bcc     .L${opcode}_finish          @ we're okay, continue on
    b       common_errArrayIndex        @ index >= length, bail

%break
    /*
     * On entry:
     *  r1 = vBB (arrayObj)
     *  r9 = vAA (obj)
     *  r10 = offset into array (vBB + vCC * width)
     */
.L${opcode}_finish:
    cmp     r9, #0                      @ storing null reference?
    beq     .L${opcode}_skip_check      @ yes, skip type checks
    ldr     r0, [r9, #offObject_clazz]  @ r0<- obj->clazz
    ldr     r1, [r1, #offObject_clazz]  @ r1<- arrayObj->clazz
    bl      dvmCanPutArrayElement       @ test object type vs. array type
    cmp     r0, #0                      @ okay?
    beq     common_errArrayStore        @ no
.L${opcode}_skip_check:
    FETCH_ADVANCE_INST(2)               @ advance rPC, load rINST
    GET_INST_OPCODE(ip)                 @ extract opcode from rINST
    str     r9, [r10, #offArrayObject_contents] @ vBB[vCC]<- vAA
    GOTO_OPCODE(ip)                     @ jump to next instruction

