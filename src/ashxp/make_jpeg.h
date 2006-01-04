#ifndef MAKE_JPEG_H
#define MAKE_JPEG_H
#include "color.h"
void shift_line(int, int, int, JSAMPLE*);
void get_indices(int, int, int, int, int*, int);
int make_jpeg(struct bg_img*);
float* set_range(char*);
void frame(struct Ash*, struct bg_img*);
void scale_shading(int*, struct bg_img*);
void add_gridlines(struct bg_img*);
int bg_index(struct bg_img*, const float, const float);
void patch_image(struct bg_img*);
#endif /*MAKE_JPEG_H */
