    // Store the number of pixels on the screen
    lda #0
    sta 0
    lda #$D2
    sta 1  // Now addresses 0 and 1 contain 00 D2  -> 0xD200

    ldx #0
    lda #0
draw:
    sta $0200,x
    adc #1
    cmp #16
    bcc noreset
    lda #0
noreset:
    inx
    cpx 0  // TODO: variables
    bcc draw

end:
    nop
    jmp end