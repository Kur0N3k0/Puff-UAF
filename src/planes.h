#ifndef PLANES_H
#define PLANES_H
#include <ctime> // typedef time_t
#include <string>
#include <vector>
#include <fstream>
#include <map>

// each object of class Planes has a vector of 'Flight', one per flight number
// with the same origin and destination.  Each 'Flight' has a vector of 
// 'Location', which is the lat, lon, height, and time.  

//float, string multimap
typedef std::multimap<float, std::string> fs_mmap;

struct Location 
{
  float lat, lon, level;
  int speed;
  time_t time;
};

struct flData
{
  char *fltnum, *origin, *dest, *make;
  struct Location loc;
  bool empty;
};

struct Flight
{
  std::string orig, dest, fltnum, make;
  time_t start_time, end_time;
  std::vector<Location> location;
};

struct CCloud
{
  public:
  float *abs_air_conc_avg, *rel_air_conc_avg, *abs_fo_conc_avg, *rel_fo_conc_avg;
  float *xValues, *yValues, *zValues;
  long int *tValues;
  int xSize, ySize, zSize, tSize;
  int d2size, d3size;
  
};

class Planes 
{
  private:
    std::vector<Flight> flight;
    fs_mmap exposure;
//    int num_flights;
  // private member functions
  void Planes::newFlight(const struct flData* data);
  void Planes::clearData(flData *data);
  void Planes::readPlanesFile(std::ifstream *file);
  void Planes::setEndTimes();
        
  public:
    // public member functions
    Planes::Planes(std::vector<std::string> file);
    int Planes::size() const { return flight.size(); }
    int Planes::size(int idx) { return flight[idx].location.size(); }
    void Planes::calculateExposure(CCloud *cc);
    float Planes::abs_conc(CCloud *cc, const Location *loc);
};


// utility functions not particular to this class
flData parseLine(char* line);
int monthAbrToNum(char* s);

#endif // PLANES_H
