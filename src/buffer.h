#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdlib.h>

typedef struct {
	size_t capacity;
	size_t size;
	char *chars;
} Line;

void line_insert_text_before_cursor(Line *line, const char *text, size_t col);
void line_backspace_before_cursor(Line *line, size_t col);
void line_delete_over_cursor(Line *line, size_t col);


#endif //BUFFER_H_
