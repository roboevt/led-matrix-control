#define COLS 5
#define ROWS 7

int matrix_init(void);
int matrix_free(void);
// char (*matrix_get(void))[COLS];

int matrix_check_col(int col);
int matrix_check_row(int row);

void matrix_set_clear(void);
void matrix_set_row(int row, int val);
void matrix_set_col(int col, int val);

void matrix_display_clear(void);
void matrix_display_row(int row);
void matrix_display_col(int col);