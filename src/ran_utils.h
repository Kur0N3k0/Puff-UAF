#ifndef RAN_UTILS_H_
#define RAN_UTILS_H_

// PROTOTYPES:
void init_seed(int& iseed, int set);
float ran1(int &idum);
float gasdev(int &idum);
float expdev(int &idum);
float poidev(float xm, int &idum);
float gammln(float xx);
float factorial(float f);
float poi_dist(float xm, int &idum);

// RAN1 DEFS:
#define M1 259200
#define IA1 7141
#define IC1 54773
#define RM1 (1.0/M1)
#define M2 134456
#define IA2 8121
#define IC2 28411
#define RM2 (1.0/M2)
#define M3 243000
#define IA3 4561
#define IC3 51349
#ifndef PI
    #define PI 3.141592654
#endif

#endif
