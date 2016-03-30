; The simplest pong game

define  draw_cursor   $00
define  draw_cursor_h   $01


  jsr init

mainloop:
  jsr get_input
  jsr check_collisions
  jsr update_paddles
  jsr update_ball
  jsr draw_paddles
  jsr draw_ball
  jmp mainloop


init:
  ; Init draw cursor
  lda #0
  sta draw_cursor
  lda #$2
  sta draw_cursor_h

  ; Draw the separator
  lda #$0f
  sta draw_cursor ; set draw cursor x to the middle of the screen

draw_separator:
  lda #$c  ; grey
  ldx #0
  sta (draw_cursor, x)  ; draw pixel
  lda draw_cursor
  adc #$40 ; step two pixels down
  sta draw_cursor
  bcc draw_separator ; continue until overflow
  inc draw_cursor_h
  lda draw_cursor_h
  cmp #$6
  bne draw_separator ; continue until less than 6
  rts



get_input:
  rts


check_collisions:
  rts


update_paddles:
  rts


update_ball:
  rts


draw_paddles:
  rts


draw_ball:
  rts


game_over:
  ; the end