    define screenL 0
    define screenH 1
    define screenHmax $d3

    lda #0
    sta screenL
    lda #$02
    sta screenH

    ldy #0
    lda #0
draw:
    sta (screenL),y
    adc #1
    cmp #16
    bcc noreset
    lda #0  // reset color
noreset:
    iny
    bne draw
    inc screenH
    pha
    lda #screenHmax
    cmp screenH
    pla
    bcs draw

    end


