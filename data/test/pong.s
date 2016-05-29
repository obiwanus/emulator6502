define  draw_cursor     $00
define  draw_cursor_h   $01

define  up    1
define  left  2
define  ball_x      $02   // 2 bytes
define  ball_x_h    $03
define  ball_y      $04   // 1 byte
define  ball_color  $0f   // white
define  ball_direction  $05
define  ball_display    $06
define  ball_display_h  $07
define  max_y   192
define  max_x_l 23    // = 279 - 256
define  screen_half_x   140


  jsr init
  jsr draw_separator
  jmp game_over

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
  jsr init_draw_cursor
  rts


init_draw_cursor:
  lda #$00
  sta draw_cursor
  lda #$02
  sta draw_cursor_h
  rts


update_ball:
  lda ball_direction
  and up
  bne move_up
  inc ball_y  // move down
  lda #max_y
  cmp ball_y
  beq bounce
  jmp move_horiz
move_up:
  dec ball_y  // move up
  bne move_horiz  // else bounce
bounce:
  lda ball_direction
  eor up
  sta ball_direction

move_horiz:
  lda ball_direction
  and left
  beq move_left
  // move_right
  jmp end_update_ball
move_left:
  // move_left

end_update_ball:
  rts


draw_separator:
  define separator_color 10   // light grey
  jsr init_draw_cursor
  lda #screen_half_x
  sta draw_cursor // set draw cursor x to the middle of the screen
  ldy #96

draw_separator_loop:
  lda #separator_color
  ldx #0
  sta (draw_cursor, x)  // draw pixel
  lda draw_cursor
  clc
  adc #48
  sta draw_cursor
  lda draw_cursor_h
  adc #2
  sta draw_cursor_h
  dey
  bne draw_separator_loop // continue Y times
  brk
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