; The simplest pong game

define  draw_cursor     $00
define  draw_cursor_h   $01

define  up    1
define  down  2
define  left  4
define  right 8
define  ball_l      $02  ; screen location of the ball, low byte
define  ball_h      $03  ; screen location of the ball, high byte
define  ball_dir_x  $04
define  ball_dir_y  $05


  jsr init

mainloop:
  jsr get_input
  jsr update_paddles
  jsr update_ball
  jsr draw_separator
  jsr draw_paddles
  jsr draw_ball
  jsr sleep
  jmp mainloop


init:
  ; Init ball
  lda #$18
  sta ball_l
  lda #$03
  sta ball_h
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
  lda #0
  ldx #0
  sta (ball_l, x)  ; erase ball
  lda ball_dir_x
  cmp #left
  beq move_ball_left
  cmp #right
  beq move_ball_right
move_ball_left:
  dec ball_l
  jmp move_ball_vert
move_ball_right:
  inc ball_l
  jmp move_ball_vert
move_ball_vert:
  lda ball_dir_y
  cmp #up
  beq move_ball_up
  cmp #down
  beq move_ball_down
move_ball_up:
  lda ball_l
  sbc #$20  ; pitch
  sta ball_l
  lda ball_h
  sbc #0
  sta ball_h
  jmp end_update
move_ball_down:
  lda ball_l
  clc
  adc #$20  ; pitch
  sta ball_l
  lda ball_h
  adc #0
  sta ball_h
  jmp end_update
end_update:
  rts


draw_separator:
  ; Init draw cursor
  lda #0
  sta draw_cursor
  lda #$2
  sta draw_cursor_h

  lda #$0f
  sta draw_cursor ; set draw cursor x to the middle of the screen
draw_separator_loop:
  lda #$c  ; grey
  ldx #0
  sta (draw_cursor, x)  ; draw pixel
  lda draw_cursor
  clc
  adc #$40 ; step two pixels down
  sta draw_cursor
  bcc draw_separator_loop ; continue until overflow
  inc draw_cursor_h
  lda draw_cursor_h
  cmp #$6
  bne draw_separator_loop ; continue until less than 6
  rts


draw_paddles:
  rts


draw_ball:
  ldx #0
  lda #1
  sta (ball_l, x)
  rts


sleep:
  ldx #0
sleep_loop:
  dex
  bne sleep_loop
  rts


game_over:
  ; the end