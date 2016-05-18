    lda #01
    sta $0200
    lda #02
    sta $0202,x
    lda #03
    sta $0204
    lda #04
    sta $0206

    sta ($02,x)

    jmp $d400
    jmp ($d400)

    bmi some_label

    inx

end:
    nop
    jmp end