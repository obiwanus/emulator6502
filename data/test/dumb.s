
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
    cmx #53760
    bcc draw

end:
    nop
    jmp end