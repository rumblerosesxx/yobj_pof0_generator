# yobj_pof0_generator
Generates the POF0 section of the given YOBJ

The POF0 section at the end of the YOBJ file appears to be a variable-length
encoded set of indices into the main YOBJ data. Probably used by the game
engine to efficiently load (or keep?) the file in memory rather than some sort
of checksum as previously speculated.

The format is fairly simple variable length encoding of relevant indices in
ascending order, with each value being relative in position to the previous.

The encoding itself uses the two low-order bits of the first byte to determine
how many consequent bytes to use for the offset data. These are then rotated
to the high-order position, which allows shifting the value across multiple
bytes when decoding.

* 0x40 means just this byte
* 0x80 means this and the following byte, i.e. half word
* 0xC0 means this and following 3 bytes, i.e. full word

The reason we can get away with using the two low-order bits is because all
offsets are multiples of 4 within the file, so no values will ever use these
bits to store information.

Reverse engineering was made possible by the POF0 decoder written by eatrawmeat391
(found in POF0_decoder_source.7z) as well as rrxx_tools_addon.py written by
anonymous loverslab member, with credit to mariokart64n for the original maxscript.
The former made it possible to identify which offsets the POF0 is referencing while
the latter helped provide clues as to what those offsets represent inside the file.

NOTE: this overall process should work for YOBJ files for games other than RRXX,
such as SVR, etc. but I expect there are endianness issues that will require small
tweaks to this code to make it work with YOBJ from other games/platforms.
