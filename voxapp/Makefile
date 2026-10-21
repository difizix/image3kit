msSrc ?= $(abspath ../../../src)

mains = voxelImageProcPy

all: $(mains);

MS_VERSION=2
USE_SINGLECPP=1
USE_ZLIB=1
USE_TIFF=1
USE_VOX_IMG=1
USE_VOX_FULL=1
USE_POCKETPY=1
USE_OMP=1

# the "full" voxel image bindings need the sources of the other voxel tools (without their main functions)
srcs += ../voxlib/VxlDifImg.cpp ../voxlib/VxlStrips.cpp
srcs += ../vxlToFoam/voxelToFoam.cpp ../vxlToFoam/voxelToFoamPar.cpp ../vxlToFoam/voxelToFoamPar_seq.cpp
srcs += ../vxlToSurf/smoothGauss.cpp ../vxlToSurf/voxelSurface.cpp ../vxlToSurf/voxelSurface_write.cpp ../vxlToSurf/voxelToSurfMesh.cpp
msCFLAGS += -DNO_MAIN -I../common -I../voxlib -I../src -I../pkgs/stb -I../pkgs/zlib -I../vxlToFoam -I../vxlToSurf
include ${msSrc}/script/Makefile.in

test:; @echo "voxapp tests skipped"
