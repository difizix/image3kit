#include <iostream>
#include <string>

#include "globals.h"
#include "voxelImageI.h"

namespace VxlPy { void execScript(const std::string& script, VoxelImagesBase* imgPtr, const std::string& nam); }

using namespace std;
int usage() {
  std::cout << "Usage:\n voxelImageProcPy imageFile.mhd  ['python commands']  [outputFile.mhd]\n";
  return 1;
}

//! Reads imageFile.mhd (like sindo's VxlRead keyword: processing any VxlPy
//! script embedded in its own header), optionally runs a second python
//! script given on the command line against the loaded image (bound as
//! `img`), and, if a third argument is given, writes the (processed) image
//! to that path.
int main(int argc, char** argv) {

  if (argc < 2) return usage();

  int code = 0;
  try {
    std::unique_ptr<VoxelImagesBase> img = readImage(argv[1], 1);
    ensure(img, "can not read image "+string(argv[1]), -1);


    if (argc > 2 && std::string(argv[2]) != "-o")
      VxlPy::execScript(argv[2], img.get(), argv[1]);

    if (argc > 4 && std::string(argv[2]) != "UChar") {
        VoxelImage img8(img->size3(), 0, img->dx(), img->X0());
    		resetFromImageT<unsigned char,short,unsigned short,int,unsigned int,float,double, char>(img8,img.get());
        img->write(argv[3]);
    }
    else if (argc > 3)
      img->write(argv[3]);
  }
  catch (std::exception& e) {
    cout << "Error in py: " << e.what()<<endl;
    code = 1;
  }
  catch (...) {
    cout << "Error in py: "<<endl;
    code = 1;
  }

  return code;
}
