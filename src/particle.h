#ifndef PARTICLE_H_
#define PARTICLE_H_

class Particle {
    
public:
    double   x, y, z;  // 3-dimensional location
    double   size;  // radius in meters
    double   startTime;
    double   mass_fraction;
    bool     grounded; // is ash on the ground?
    bool     exists;   // has ash left the boundary?
    int      order;    // sorted order
    
    Particle();
    Particle(double xx, double yy, double zz);
    ~Particle();
    
    Particle & operator=(const Particle &pnt);
    Particle & operator+(Particle &pnt); 
    Particle & operator-(Particle &pnt); 
    

};

#endif 
