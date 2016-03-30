; The simplest pong game

define  draw_cursor_l   $00
define  draw_cursor_h   $01
define  draw_cursor     $0000


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
  sta draw_cursor_l
  lda #$2
  sta draw_cursor_h

  ; Draw the separator
  lda #$0f
  sta draw_cursor_l ; set draw cursor x to the middle of the screen
  lda #$c
init_draw_separator:
  sta ($0000)
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