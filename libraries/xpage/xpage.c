// Set the _XPAGE symbol so SDCC can properly initialize the XDATA variables
// with initializers.
// sdcc -c xpage.c --model-medium
__sfr __at (0x93) _XPAGE;
