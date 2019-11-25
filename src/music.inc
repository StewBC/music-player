;-----------------------------------------------------------------------------
; music.inc
;
; Stefan Wessels, 2019
; This is free and unencumbered software released into the public domain.

;-----------------------------------------------------------------------------
.segment "CODE"
;-----------------------------------------------------------------------------
.proc musicLoad

    partNumber = tempBlock + 0                  ; counter of part numbers
    partBank   = tempBlock + 1                  ; counter of banks to use in load
    fileStart  = tempBlock + 2                  ; index to start of file name
    fileHex    = tempBlock + 3                  ; index past hex portion of file name

    lda #0
    sta songBanks                               ; banks used by sound starts at $00
    sta partBank                                ; internal - where a part goes
    sta musicNumSongs                           ; number of songs loaded
    sta fileStart                               ; offset to song name
    sta musicRepeat                             ; turn off auto-repeat
    sta musicPlaying                            ; init to the 1st song
    tax 

loop:
    lda songTitles, x                           ; load 1st character of song name
    bne load                                    ; if <> 0 then there's a song
    clc                                         ; carry clear on success
    rts 

load:
    lda #1
    sta partNumber                              ; reset the part number for the file
:
    inx                                         ; keep looking for a $0
    lda songTitles, x
    bne :-
    inx                                         ; step 2x for the 2 hex digits
    inx 
    stx fileHex                                 ; this is past the file name
    ldx musicNumSongs                           ; the song being loaded
    lda partBank                                ; get what bank is up next
    sta songBanks, x                            ; and mark that as the song start

part:
    lda partBank                                ; get the bank number for this part
    sta VIA1P1                                  ; and set up the bank
    lda partNumber                              ; get the part number
    jsr toHex                                   ; make it hex
    ldx fileHex                                 ; copy szHex into file name
    ldy #1
:
    dex 
    lda szHex, y 
    sta songTitles, x 
    dey 
    bpl :-

    lda fileHex                                 ; calculate the file name length
    sec 
    sbc fileStart                               ; name portion
    pha                                         ; remember for SETNAM
    clc 
    lda fileStart
    adc #<songTitles                            ; start of name lo
    tax                                         ; in x for setnam
    lda #0
    adc #>songTitles                            ; start of name hi
    tay                                         ; in y for SETNAM
    pla                                         ; get the length back in a for SETNAM
    clc 
    jsr SETNAM
    lda #$00                                    ; logical number
    ldx #$01                                    ; device 1
    ldy #$01                                    ; secondary address
    jsr SETLFS 
    ldx #<BANKED_RAM                            ; address of where to load in x/y
    ldy #>BANKED_RAM
    lda #$00                                    ; $00 is load ($01 is verify I think)
    jsr LOAD
    bcs fail                                    ; carry set on error
    inc partBank 
    inc partNumber                              ; finished this part, up to next part number
    ldx musicNumSongs                           ; song being loaded
    dec songsParts, x                           ; how many parts left?
    bne part                                    ; more - so go load the next part
    inc musicNumSongs
    ldx fileHex
; inx                                                                         ; there is a null terminator (SQW maybe not neccesary)
    stx fileStart
    jmp loop                                    ; JuMP back to load next song

fail:
    rts 

.endproc

;-----------------------------------------------------------------------------
.proc musicStartPlayer

    jsr musicClearChip                          ; kill all in the chip

    lda musicPlaying                            ; if the player is alreay installed
    beq :+                                      ; ignore this request to install it again
    rts 

:
    sei                                         ; install an IRQ handler that will chain onto the old handler

    lda IRQ_VECTOR                              ; save the old handler
    sta old_irq
    lda IRQ_VECTOR + 1
    sta old_irq + 1

    lda #<musicIRQHandler                       ; install the new handler
    sta IRQ_VECTOR
    lda #>musicIRQHandler
    sta IRQ_VECTOR + 1

    lda #1                                      ; mark the player as active
    sta musicPlaying

    cli 

    rts 

.endProc 

;-----------------------------------------------------------------------------
.proc musicStopPlayer

    lda musicPlaying                            ; check player status
    bne :+                                      ; if not playing then return else remove the player
    rts 

:
    sei                                         ; remove the IRQ handler by restoring the old vector

    lda old_irq
    sta IRQ_VECTOR
    lda old_irq + 1
    sta IRQ_VECTOR + 1

    lda #0                                      ; mark the player as inactive
    sta musicPlaying

    cli 
    
    jsr musicClearChip                          ; and kill all in the chip

    rts 

.endProc 

;-----------------------------------------------------------------------------
; Sets the song indexed by the X index register to active
.proc musicSetSongByIndex

    stx musicCurrSong                           ; mark which song is to be active

    jsr musicClearChip                          ; turn off everything on the chip

    lda #<BANKED_RAM                            ; set zMusicPtr to BANKED_RAM
    sta zMusicPtr
    lda #>BANKED_RAM
    sta zMusicPtr + 1

    lda #0
    sta musicDelay                              ; set the delay countdown to 0

    lda songBanks, x                            ; get the active song's start bank
    sta musicBank
    sta VIA1P1                                  ; and set that bank active

    rts 

.endproc 

;-----------------------------------------------------------------------------
; Sets first song as active that has a class match with musicClass
.proc musicSetSongByClass

    ldx #0                                      ; start at the 1st song
:
    lda songClasses, x                          ; get its class
    bit musicClass                              ; does it match the desired class
    beq next                                    ; not a match, try next

    jmp musicSetSongByIndex                     ; activate the first found song in the class

next:
    inx                                         ; next song
    cpx musicNumSongs                           ; check all loaded songs
    bcc :-
    lda musicPlaying                            ; no match - is the music player actve
    beq :+                                      ; no, just exit
    jsr musicStopPlayer                         ; yes - stop the musuic player
:
    rts 

.endproc

;-----------------------------------------------------------------------------
; Shorthand to set and start a song
.proc musicPlaySongOfClass

    sta musicClass                              ; set as the type that should play now
    jsr musicSetSongByClass                     ; find the song
    jmp musicStartPlayer                        ; and play the active song
    
.endproc 

;-----------------------------------------------------------------------------
; Finds the next song to play.  Must match class.  If repeat is true,
; or no songs match class, will re-play the song that was playing
.proc musicNextSong

    lda musicRepeat                             ; if repeat <> 0 then just play same song again
    bne repeat

    ldx musicCurrSong                           ; get the index of the current song

loop:
    inx                                         ; go to next song
:
    cpx musicCurrSong                           ; if the index is back to the current song, it will play again
    beq repeat

    cpx musicNumSongs                           ; don't walk past the last song
    bcs wrap

    lda songClasses, x                          ; get the class of the song
    bit musicClass                              ; test it against the desired play classes
    beq loop                                    ; no match, loop to next song

repeat:
    jmp musicSetSongByIndex                     ; match or repeat, so play song pointed at by x reg

wrap:
    ldx #0                                      ; past last song, wrap back to 1st song
    beq :-                                      ; JuMP

.endproc

;-----------------------------------------------------------------------------
; Write 0 to every register
.proc musicClearChip

    lda #0
    tay 
:
    sty YM2151                                  ; write 0-255 the the register
    sta YM2151 + 1                              ; and write 0 to the value
    iny 
    bne :-
    
    rts 

.endproc

;-----------------------------------------------------------------------------
; The actual player invoked by the system IRQ (through $0314/$0315)
.proc musicIRQHandler

    lda musicDelay 
    beq start                                   ; if delay is 0, play something
    dec musicDelay
    jmp (old_irq)                               ; if there was a delay, chain the old irq

start:
    ldy #0                                      ; index y to 0

loop:
    lda (zMusicPtr), y                          ; get a register
    iny 

    cmp #REG_DELAY                              ; test if this is a control register
    beq delay 
    cmp #REG_YIELD
    beq leave
    cmp #REG_END
    beq done
    cmp #REG_BUFFER
    beq buffer

    sta YM2151                                  ; activate the register
    lda (zMusicPtr), y                          ; and load the value for the register

    sta YM2151 + 1                              ; set the value
    iny                                         ; y step's in 2s so may reach 0

    bne loop                                    ; y <> 0 keep looping
    inc zMusicPtr + 1                           ; when y is 0, just update the hi byte
    bne loop                                    ; and keep looping

delay:
    lda (zMusicPtr), y                          ; load the n for the delay
    iny 
    sta musicDelay                              ; save n to delay

leave:
    tya                                         ; y is how many bytes moved, so put in a
    clc 
    adc zMusicPtr                               ; and move the ptr by that much
    sta zMusicPtr
    bcc :+
    inc zMusicPtr + 1
:
    jmp (old_irq)                               ; and chain the old IRQ

done:
    jsr musicNextSong                           ; song done, play what's up next
    jmp (old_irq)                               ; ignore y since this is a reset, so chain the old irq

buffer:
    inc musicBank                               ; go to next bank
    lda musicBank
    sta VIA1P1                                  ; and set the bank active
    ldy #<BANKED_RAM                            ; reset the MuscPtr
    sty zMusicPtr                               ; assumes the buffer is 0x100 aligned
    lda #>BANKED_RAM
    sta zMusicPtr + 1
    bne loop                                    ; JuMP to keep playing

.endproc 
