#ifndef ASHXP_COLOR_H
#define ASHXP_COLOR_H
void add_colorbar(struct bg_img*);
void add_date_time(struct bg_img *, char *); 
void add_labels(struct bg_img*);
void color1(JSAMPLE*);
void print_number(int, int, struct bg_img*);
void put_bitmap(struct bg_img*, struct bg_img*, int);
void wgt_color(JSAMPLE*, float*);
#endif
