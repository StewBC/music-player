;-----------------------------------------------------------------------------
; music.asm
; Interrupt driven music player for Commander X16
;
; Stefan Wessels, 2019
; This is free and unencumbered software released into the public domain.
;
; Use the ca65 assembler and make to build

;-----------------------------------------------------------------------------
; assembler directives
.debuginfo on
.listbytes unlimited

;-----------------------------------------------------------------------------
.segment "CODE"

;-----------------------------------------------------------------------------
jmp main                                        ; This ends up at $080d (sys 2061's target)

;-----------------------------------------------------------------------------
.include "defs.inc"                             ; constants
.include "zpvars.inc"                           ; Zero Page usage (variables)
.include "text.inc"                             ; the toHex function
.include "music.inc"                            ; the music routines
.include "variables.inc"                        ; DATA segment variables

;-----------------------------------------------------------------------------
.segment "CODE"

;-----------------------------------------------------------------------------
.proc main

    jsr musicLoad
    bcc :+
err:jmp err
:
    lda #$ff                                    ; select classes to play - $ff is all classes
    jsr musicPlaySongOfClass                    ; Set the class, cue the song and start the player
    rts                                         ; return to basic

.endproc
