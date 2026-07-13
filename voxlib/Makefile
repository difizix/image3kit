
mains := voxelImageProcess.cpp  voxelGrad.cpp performance.cpp
srcs := voxelImage.cpp  vxlPro.cpp  vxlPro1.cpp  vxlPro2.cpp
tsts := test.py
all: msCApp1
#all: $(mains)
$(info mains: $(mains),   tsts: $(tsts) )

msCFLAGS += -I../optim  -I. -I../primitives/  -O3   -DGCC_COMPILER
#ifneq ("${OPT}", ".exe")
  msCFLAGS += -DLPNG
  msLFLAGS += -lpng
#endif
msSrc ?= $(abspath ../../src)
USE_ZLIB=1
USE_TIFF=1
USE_SVG=1
USE_OMP=1
USE_CPP17=1
USE_SINGLECPP=1
USE_msTEST=1
include  ${msSrc}/script/Makefile.in


#sudo apt install libomp-dev
#sudo apt install zlib1g-dev
#sudo apt install libtiff-dev
# TODO add to cmake USE_msTEST=1
msCApp1:
	cmake -S${CURDIR} -B${msBilDir}/voxlibCmake
	cmake --build   ${msBilDir}/voxlibCmake  -j # --verbose
	cmake --install ${msBilDir}/voxlibCmake --prefix ${msInst}
