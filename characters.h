#include "matrix.h"

// returns a pointer to a 2D array of "pixels" for a given character, or a block for an unknown character
const char (*character_get_array(char character))[ROWS][COLS];
