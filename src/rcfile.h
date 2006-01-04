#ifndef PUFF_RCFILE_H
#define PUFF_RCFILE_H
#define PUFF_MAX_NUM_MASKS 100
#include <string>

class PuffRC {
  private:
    enum RCType {PUFFRC_MODEL, PUFFRC_DEM};
    std::string modelName, Tmask, umask, vmask, zmask, dataPath;
    std::string demName, demPath;
    std::string cleanString (std::string str);
    std::string varTname, varUname, varVname, varZname;
    char *fileName;
    bool fourDdata;
    std::string getMask(char *var);
       
  public:
    PuffRC();
    ~PuffRC();
    void PuffRC::badMask(const std::string *mask);
    int PuffRC::findNonLiteral(const std::string *mask, const char *c);
    bool PuffRC::init(char *rcfileArg = (char*)NULL);
    int PuffRC::loadResources(char* modelArg, const char *type);
    const char* PuffRC::demType();
    const char* PuffRC::getDemPath();
    std::string PuffRC::getString(char *p);
    std::string PuffRC::getString(const std::string* p);
    const std::string PuffRC::mostRecentFile(const char *edate, char *var, double runHours = 0.0);
    };
    
#endif
