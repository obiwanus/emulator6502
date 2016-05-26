// The simplest pong game

define  draw_cursor     $00
define  draw_cursor_h   $01

define  up    1
define  down  2
define  left  4
define  right 8
define  ball_x      $02
define  ball_y      $03
define  ball_dir_x  $04
define  ball_dir_y  $05
define  ball_display    $06
define  ball_display_h  $07
define  screen_border   191


  jsr init

mainloop:
  jsr get_input
  jsr update_paddles
  jsr update_ball
  // jsr draw_separator
  jsr draw_paddles
  jsr draw_ball
  jsr sleep
  jsr sleep
  jsr sleep
  jsr sleep
  jmp mainloop


init:
  // Init ball
  lda #24
  sta ball_x
  lda #10
  sta ball_y
  lda #left
  sta ball_dir_x
  lda #up
  sta ball_dir_y
  rts


get_input:
  rts


update_paddles:
  rts


update_ball:
  jsr update_ball_drawing_position
  lda #0
  ldx #0
  sta (ball_display, x)  // erase
  lda ball_dir_x
  cmp #left
  beq move_ball_left
  cmp #right
  beq move_ball_right
move_ball_left:
  dec ball_x
  beq bounce_right
  jmp move_ball_vert
bounce_right:
  lda #right
  sta ball_dir_x
  jmp move_ball_vert
move_ball_right:
  inc ball_x
  lda ball_x
  cmp #screen_border
  beq bounce_left
  jmp move_ball_vert
bounce_left:
  lda #left
  sta ball_dir_x
  jmp move_ball_vert

move_ball_vert:
  lda ball_dir_y
  cmp #up
  beq move_ball_up
  cmp #down
  beq move_ball_down
move_ball_up:
  dec ball_y
  beq bounce_down
  jmp end_update
bounce_down:
  lda #down
  sta ball_dir_y
  jmp end_update
move_ball_down:
  inc ball_y
  lda ball_y
  cmp #screen_border
  beq bounce_up
  jmp end_update
bounce_up:
  lda #up
  sta ball_dir_y
  jmp end_update
end_update:
  rts


draw_separator:
  // Init draw cursor
  lda #0
  sta draw_cursor
  lda #2
  sta draw_cursor_h

  lda #$0f
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


draw_paddles:
  rts


draw_ball:
  jsr update_ball_drawing_position
  lda #1
  ldx #0
  sta (ball_display, x)
  rts


update_ball_drawing_position:
  lda #2
  sta ball_display_h
  lda #0
  sta ball_display
  clc
  adc ball_x
  sta ball_display
  ldx ball_y
  beq update_ball_drawing_end
adjust_y_loop:
  lda ball_display
  clc
  adc #$20
  sta ball_display
  lda #0
  adc ball_display_h
  sta ball_display_h
  dex
  bne adjust_y_loop
update_ball_drawing_end:
  rts


sleep:
  ldx #0
sleep_loop:
  dex
  bne sleep_loop
  rts


game_over:
  // the end