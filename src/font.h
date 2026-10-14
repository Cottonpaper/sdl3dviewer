#ifndef FONT_H
#define FONT_H

/* Tiny built-in 5x7 bitmap font (uppercase letters, digits, some punctuation).
   Used by the editor menu overlay; no external font files or libraries. */

#define FONT_GLYPH_W 5
#define FONT_GLYPH_H 7
#define FONT_ADVANCE 6 /* glyph width + 1 px spacing */

/* Returns FONT_GLYPH_W bytes, one per column, bit 0 = top row.
   Unsupported characters return a blank glyph. */
const unsigned char *font_glyph(char c);

#endif
