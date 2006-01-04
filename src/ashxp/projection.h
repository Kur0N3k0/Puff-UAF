#ifndef ASHXP_PROJECTION_H
#define ASHXP_PROJECTION_H

#endif
void make_projection(struct bg_img *bg);
void idx2xy(struct bg_img *bg, int idx, int *x, int *y);
void idx2ll(struct bg_img *bg, int idx, double *lat, double *lon, int *rgb_flag);
int xy2idx(struct bg_img *bg, int x, int y);
void get_image_size(struct bg_img *bg, struct bg_img *bg_p);
