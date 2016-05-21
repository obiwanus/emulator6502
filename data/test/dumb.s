    lda #01
    sta $0200
    lda #02
    sta $0202
    lda #03
    sta $0204
    lda #04
    sta $0206
    lda #05
    sta $0208
    lda #06
    sta $020A
    lda #07
    sta $020C

    // sta ($02,x)

    // jmp $d400
    // jmp ($d400)

    // bmi end

    // inx

end:
    nop
    jmp end