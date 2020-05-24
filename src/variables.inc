;-----------------------------------------------------------------------------
; variables.inc
;
; Stefan Wessels, 2019
; This is free and unencumbered software released into the public domain.

;-----------------------------------------------------------------------------
.segment "DATA"

;-----------------------------------------------------------------------------
; Music player variables
musicDelay:         .res 1                      ; Internal - how many frames to wait
musicBank:          .res 1                      ; Internal - Which RAM bank is active
musicClass:         .res 1                      ; What class(es) of music can be played
musicCurrSong:      .res 1                      ; Index of the song currently active
musicNumSongs:      .res 1                      ; Number of songs loaded
musicPlaying:       .res 1                      ; true - IRQ installed, fals - NOT installed
musicRepeat:        .res 1                      ; true - repeat current song endlessly
old_irq:            .res 2                      ; $0314 original address
irq_bank:           .res 1                      ; active bank when IRQ is called

tempBlock:          .res 19                     ; used by different routines that need local storage

;-----------------------------------------------------------------------------
songTitles:                                     ; Song name and 2 bytes for the hex part number
    .byte "0210"      , $00, $00
    .byte "02final"   , $00, $00
    .byte "03red"     , $00, $00
    .byte "04super"   , $00, $00
    .byte "05after"   , $00, $00
    .byte "06city"    , $00, $00
    .byte "10cruising", $00, $00
    .byte "08final"   , $00, $00
    .byte "11camino"  , $00, $00
    .byte $00                                   ; null terminate the list of files

songsParts:                                     ; how many parts the song has
    .byte $05   ; 0210
    .byte $0b   ; 02final     
    .byte $05   ; 03red       
    .byte $05   ; 04super     
    .byte $0c   ; 05after     
    .byte $01   ; 06city      
    .byte $07   ; 10cruising  
    .byte $10   ; 08final     
    .byte $02   ; 11camino                      ; SHould be $08 but that requires more than the default 512 KB ram
                                                ; run emulator with -ram 2048 to set to $08

songBanks:                                      ; Filled in by loader
    .byte $00   ; 0210
    .byte $00   ; 02final
    .byte $00   ; 03red
    .byte $00   ; 04super
    .byte $00   ; 05after
    .byte $00   ; 06city
    .byte $00   ; 10cruising
    .byte $00   ; 08final
    .byte $00   ; 11camino

songClasses:                                    ; user defined bit mask of what classes a song belongs to
    .byte $01   ; 0210
    .byte $02   ; 02final
    .byte $04   ; 03red
    .byte $08   ; 04super
    .byte $10   ; 05after
    .byte $20   ; 06city
    .byte $40   ; 10cruising
    .byte $81   ; 08final
    .byte $83   ; 11camino
