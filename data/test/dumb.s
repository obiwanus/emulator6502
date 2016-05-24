    // Store the number of pixels on the screen
    lda #0
    sta 0   // screenL
    lda #$02
    sta 1   // screenH

    ldy #0
    lda #0
draw:
    sta (0),y
    adc #1
    cmp #16
    bcc noreset
    lda #0  // reset color
noreset:
    iny
    bne draw
    inc 1
    ldx #$d3
    cpx 1
    bcs draw

    end