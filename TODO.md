TODO
====
- [ ] keyboard commands
  - zoom
  - mask movement, arrow keys
- [x] fit to terminal size
- [ ] resize with window
- [x] flag for mirroring image
- [ ] build peggyFrame (header + 13x13 bytes, 2 pixels per byte)
- [ ] i2c output of peggyFrame
- [ ] GetOpt / Boost.Program_options
- [ ] check about inline utf8
- [ ] learn c++
- [ ] document beaglebone to peggy circuit -- i2c setup w/ 5.6k pullup resistors on P9_19 and P9_20, BiDi level converter.
- [ ] v4l2-ctl examples

More unicode / ASCII / ANSI fun
===============================
- [x] ncurses
- [x] 256 color support, 8-bit tty.
- [ ] Quantization for 3-bit color too, for ANSI terms?
- [ ] command line params - size, color, fullscreen
- [x] check if background-color with spaces is better than \u2588 FULL BLOCK (avoid utf8 requirement)
- [ ] unicode dingbats character ramp for utf8 without 256 color? :-P
