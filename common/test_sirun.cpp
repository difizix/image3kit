
#define _InitGlobals
#include "SiRun.h"

using namespace std;
class Plugins : public SiR
{
 public:
  Plugins() : SiR()  {};
};


int main(int argc, char *argv[]) {
  try {
    Plugins plugins; // test
    return MainT(argc, argv, &plugins); // test
  }
  catch( runtime_error & ex ) { cerr << "\nAborted; " << ex.what() <<"\n"<< endl; }
  //catch( exception     & ex ) { cerr << "\nAborting due to "<<typeid(ex).name()<<":"<< ex.what() <<"\n"<< endl; }
  return -1;
}
