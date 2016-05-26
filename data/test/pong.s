define  draw_cursor     $00
define  draw_cursor_h   $01

define  up    1
define  down  2
define  left  4
define  right 8
define  ball_x      $02   // 2 bytes
define  ball_y      $04   // 1 byte
define  ball_direction  $05
define  ball_display    $06
define  ball_display_h  $07


  jsr init

mainloop:
  jsr update_ball
  jsr draw_separator
  jsr draw_ball
  jsr sleep   // TODO: sleep on "vblank"
  jmp mainloop


init:
  // Init ball
  lda #24
  sta ball_x
  lda #10
  sta ball_y
  lda #left
  ora #up
  sta ball_direction
  rts


update_ball:
  rts


draw_separator:
  // Init draw cursor
  lda #0
  sta draw_cursor
  lda #2
  sta draw_cursor_h

  lda #140
  sta draw_cursor // set draw cursor x to the middle of the screen
draw_separator_loop:
  lda #$c  // grey
  ldx #0
  sta (draw_cursor, x)  // draw pixel
  lda draw_cursor
  clc
  adc #$40 // step two pixels down
  sta draw_cursor
  bcc draw_separator_loop // continue until overflow
  inc draw_cursor_h
  lda draw_cursor_h
  cmp #$6
  bne draw_separator_loop // continue until less than 6
  rts


draw_ball:
  rts


sleep:
  ldx #0
sleep_loop:
  dex
  bne sleep_loop
  rts


game_over:
  end