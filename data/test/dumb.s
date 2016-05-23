    // Store the number of pixels on the screen
    lda #0
    sta 0   // screenL
    lda #$02
    sta 1   // screenH
    lda #$D2
    sta 2   // screenH max

    ldy #0
    lda #2
draw:
    sta (0),y
    adc #1
    cmp #16
    bcc noreset
    lda #0  // reset color
noreset:
    iny
    bne draw

end:
    nop
    jmp end