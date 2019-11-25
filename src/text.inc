;-----------------------------------------------------------------------------
; text.inc
;
; Stefan Wessels, 2019
; This is free and unencumbered software released into the public domain.

;-----------------------------------------------------------------------------
.segment "DATA"

szHex:            .asciiz "  "

;-----------------------------------------------------------------------------
.segment "CODE"

;-----------------------------------------------------------------------------
; Prints the accumulator to the szHez text as 2 hex bytes
.proc toHex

    ldx #0                                      ; offset into the szHex buffer
    pha                                         ; save a for lower 4 bits
    lsr                                         ; move upper 4 bits to lo
    lsr 
    lsr 
    lsr 
    jsr :+                                      ; process upper 4 bits
    pla                                         ; restore a to process lower 4 bits
:
    and #$0f                                    ; mask to only the 4 bits now in lo
    cmp #10                                     ; see if it's > 9
    bcc :+
    adc #6                                      ; move from 10+ to a+
   :
    adc #$30                                    ; and move to '0' and beyond
    sta szHex, x                                ; save this in the buffer
    inx                                         ; go to next buffer location
    rts                                         ; and return to self or caller

.endproc
