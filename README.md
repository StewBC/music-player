# music-player
 Commander X16 IRQ based Music Player (R37)

1. INTRODUCTION

I wanted to add audio to the Penetrator game I made for the Commander X16 so I
made a player that will slot in easily into an existing game and is no harder
to use than selecting a song, or type/class of song and calling play.

There's a YouTube video at: https://youtu.be/9qYxqbePETI

2. CURRENT STATUS

The player works quite well.  There are timing issues which may be because I
know next to nothing about audio, but also because the emulator for the
Commander X16 doesn't run at 100% nor, I believe, is the audio support in it at
100% accurate.

As things stand, I think this player is in good shape.  There are no "nice"
features such as volume or fade.  There's basically just play, skip, repeat, on
and off, with song classes.

3. TECHNICAL DETAILS

The "music" is nothing more than register/value pairs, with 4 of the unused
YM2151 registers used for control.  The player loads music "chunks" into banked
memory at $A000, filling the banks sequentially.  

For playback, when a song is selected, the songs' memory bank is activated and
data from that bank is fed to the YM2151.  At the end of the bank will be a
control register which cuases the player to activate the next bank.  At the end
of the song, another control register will cause the player to switch to the
next matching song.

Songs are given a bitmask "type".  The player matches songs with the desired
bits set when looking for a song to play.  This way, songs can be tagged as,
for example Front End, In Game, High Score, etc.  WIth 8 bits, there are up to
8 classes.  Songs can belong to multiple classes by simply having the
appropriate bit set.

The "api" for the player loads songs, and plays songs based on a matching
"class" or plays a specific song.  There's a next song call and there's a
repeat variable that will keep playing the same song over and over as long as
repeat is true.

The music player uses 2 bytes of zero page, but even that can be avoided if
needed.

Important notes for using the player:

1) The music must be in the same folder the emulator is started from.  As is
   checked in, the Makefile takes care of this.

2) File names must be all lowe-case or the load will fail.  

3) After loading, if carry is set, the load failed.  As it is here in GitHub
   that means the player will simpy hang and not return to the READY prompt.

The code is written for ca65, the assembler that's part of the cc65 toolchain.
The code is 6502 only.

5. THE FILES

The player is almost self-contained in the music.inc file.  The text.inc has a
hex to string function that is needed for file loading.  There are a few
defines in defs.inc.  Lastly, variables.inc defines the needed player variables
and contains the information anout the songs.  The files have extended
information for what I used in Penetrator, hence this setup.

    * defs.inc          - System defines
    * music.asm         - Calls the loaded and starts the music
    * music.cfg         - ca65 configuration file
    * music.inc         - Mujsic Player code 
    * text.inc          - hex to string routine
    * variables.inc     - Player variables and songs catalogue
    * zpvars.inc        - The 2 zero-page bytes the player uses

    * tools/vgmclean.c  - Turns Vgm files into a music-player resources

6. CREDITS

    * Frank Buss for his work on Audio and pointing me at the Commander X16
    * Everyone involved in the Commander X16 project
    * Everyone involved in making the cc65 tools, it's very good
    * Oliver Schmidt & Patryk "Silver Dream !" Łogiewa for a great Makefile

7.  RESOURCES

VGM Spec v1.70
http://www.smspower.org/uploads/Music/vgmspec170.txt?sid=aeb415184dd74afd08de71924e7f6124

Yamaha FM-Sound Synthesizer Unit documentation Ver:1.0 by NYYRIKKI
http://www.cx5m.net/fmunit.htm

7. CONTACT

Feel free to contact me at swessels@email.com if you have thoughts or
suggestions.

Thank you
Stefan Wessels
25 November 2019 - Initial Revision
