 //Basic unit test for InputFile, typses and Timing files

// Developed by:
// - Ali Q Raeini <a.q.raeini@gmail.com>  (2020-2022)

#ifdef _GeanyHighlighTricKeys
class vector {};  class for_ {};   class for_i_ {}; class for_0 {}; class for_ih_ {}; class _s {}; class array {};
class iterator {}; class fphaz {}; class Elem{}; class dbl {}; class string {};
class map {}; class T {}; class endl {}; class cout {}; class cerr {}; class min {}; class max {};
namespace std {}; class string {}; class vector {}; class ostringstream {}; class istringstream {}
#endif


#include <iostream>
#include <vector>
#include <array>

#include <cassert>
#include "globals.h"
#include "InputFile.h"
#include "profilers.h"
//#include "SiRun.h"
#include "typses.h"

using namespace std;

int main() {
 int nErrs=0; // number of errors, if any
 #define TEST  nErrs+=check
 try {

  const double small=1e-64;
  Timing tim;
  tim("Testing InputFile");
  {  cout<<"Testing InputFile inp: { ";
    InputFile inp;
    inp.add("one", "1");
    int oneInInputFile=0;
    TEST( inp.lookup("one", oneInInputFile),  "checking `addKeyword` data exist" );
    TEST( oneInInputFile == 1,                 "getting data into an an already allocated integer" );
    TEST( inp.getOr("one",2) == 1,          "looking up an integer" );
    TEST( inp.getOr("notthere",2) == 2,     "setting default value for non-existing data" );
    inp.add("dbl3_235", " 2 3 5." );
    TEST( (mag(inp.getOr("dbl3_235",dbl3()))-30) < small,  "getting data as dbl3" );
    TEST( inp.getOr("dbl3_235",string()) == "2",    "getting the first number as string" );
    TEST( inp["dbl3_235"] == " 2 3 5.",       "getting raw string data using operator [], not-recommended" );
  } cout<<" }"<<endl;

  tim("Testing vars<T>" );
  {  cout<<"Testing vars<T> (dbls, ints, dbl3s): {";
    array<int,6> oneto6= {1,2,3,4,5,6};
    TEST( piece<int>(&oneto6[0],6).sum() == 3*7,             "piece<int> construction and summation" );
    TEST( abs(dbls({1.,2.,3.,4.,5.,6.}).sum()-3*7) < small,  "dbl3 construction and summation" );
    TEST( mag(dbl3s(5,dbl3(1,1,1)).sum()-dbl3(5,5,5))<small, "dbl3s construction and summation" );
  } cout<<" }"<<endl;

  tim("Testing Timing" );
  TEST( tim.times.size()==2,  "`Timing` data collection" ); // carefull when moving around, this can fail, last one will be added later

 }
 catch (std::exception &exc) {  std::cerr << "\n\n Exception on processing: \n" << exc.what() << " Aborting! \n" << endl;  return 1; }
 catch (...)                 {  std::cerr << "\n\n Unknown exception! \n Aborting! \n" << endl;  return 1; }

 if(nErrs) cout<<nErrs<<" fails."<<endl;  else cout<<"All tests passed."<<endl;
 return nErrs;

}
