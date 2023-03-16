# Fonts and symbols

## Sources for fonts and symbols
The fonts used in this projects have been downloaded from https://fonts.google.com/specimen/Antonio and then customized using FontForge (see chapter below for instructions):
- [Antonio-Semibold](Antonio-SemiBold.ttf) is used for the time visualization. The glyphs have been modified to have a constant width
- [Antonio-Regular](Antonio-Regular.ttf) is used for the "secondary" times: alarm, snooze and bed times. Also the symbols have been embedded into this font and the glyphs have been modified to have a constant width as well
- [Antonio-Light](Antonio-Light.ttf) is used only for the "PRESS WPS" message and has not been customized or modified at all


The [symbols](symbols) are material design icons from https://pictogrammers.com/library/mdi/ which have been imported as glyphs in the fonts

## Tooling

The main tool used for the fonts and symbols customization is [FontForge](https://fontforge.org/en-US/). With this tool it is possible (amongst many other things!) to:
- Remove unnecessary glyphs
- Change the glyph width to allow for consistent spacing and layout
- Import icons and symbols, in our case in SVG format, to be able to print symbols as if they were characters
- Modify these glyphs: scaling, rotating, etc. and create new symbols this way
- Generate a new TTF file for further processing

When the customized fonts are finished, the required characters can be converted to a header file which can be included directly in the code, using the [fotoconvert tool from Adafruit](https://github.com/adafruit/Adafruit-GFX-Library/tree/master/fontconvert). First build the program with `make` and then execute these scripts to generate the required header files:
```
./fontconvert Antonio-SemiBold.ttf 75 48 58 > Antonio_SemiBold75pt7b.h
./fontconvert Antonio-Regular.ttf 26 32 70 > Antonio_Regular26pt7b.h
./fontconvert Antonio-Light.ttf 16 69 87 > Antonio_Light16pt7b.h
```
Please note that it is necessary to specify a range of characters. For big fonts it is necessary to pay attention to the flash size of the generated fonts. The fotoconvert tool does not support multiple ranges, which could help minimize the size of the files. 
