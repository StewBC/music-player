;-----------------------------------------------------------------------------
; defs.inc
;
; Stefan Wessels, 2019
; This is free and unencumbered software released into the public domain.

;-----------------------------------------------------------------------------
; System locations
; These are for r34 - find them in rom.sym in the emulator folder
IRQ_VECTOR     = $0314                          ; System IRQ jump vector
VIA1P1         = $9F61                          ; Memory bank select memory location
YM2151         = $9FE0                          ; Yamaha 2151 register address
BANKED_RAM     = $A000                          ; Location of memory mapped banked RAM
SETLFS         = $FFBA                          ; Set file/device IDs
SETNAM         = $FFBD                          ; Set file name
LOAD           = $FFD5                          ; LOAD Kernal location
SAVE           = $FFD8                          ; SAVE kernal location

;-----------------------------------------------------------------------------
; Music stream control registers
REG_DELAY      = $02                            ; Followed by n, no of IRQs to delay for
REG_YIELD      = $03                            ; Same as REG_DELAY 0
REG_END        = $04                            ; End of music data (end of song)
REG_BUFFER     = $05                            ; Switch to the next buffer
