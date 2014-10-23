#ifndef FIXED_POINT_H
#define FIXED_POINT_H

typedef int fixed_point;

fixed_point to_fixed_point(int n);
int to_int(fixed_point x);
int to_int_nearest(fixed_point x);
fixed_point add(fixed_point x, fixed_point y);
fixed_point subtract(fixed_point x, fixed_point y);
fixed_point add_int(fixed_point x, int n);
fixed_point sub_int(fixed_point x, int n);
fixed_point multi(fixed_point x, fixed_point y);
fixed_point multi_int(fixed_point x, int n);
fixed_point div(fixed_point x, fixed_point y);
fixed_point div_int(fixed_point x, int n);


#endif