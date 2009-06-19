#ifndef PUFF_RCFILE_H
#define PUFF_RCFILE_H
#define PUFF_MAX_NUM_MASKS 100
#include <string>
#include <cstring>

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
		void envReplace(std::string *s);
       
  public:
    PuffRC();
    ~PuffRC();
    void badMask(const std::string *mask);
    int findNonLiteral(const std::string *mask, const char *c);
    bool init(char *rcfileArg = (char*)NULL);
    int loadResources(char* modelArg, const char *type);
    const char* demType();
    const char* getDemPath();
    std::string getString(char *p);
    std::string getString(const std::string* p);
    const std::string mostRecentFile(const char *edate, char *var, double runHours = 0.0);
    };
    
#endif
