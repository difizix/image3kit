
msSrc ?= $(abspath ../../../src)

all: msApp

USE_msMAKE=1
USE_ZLIB=1
USE_TIFF=1
USE_VOX_IMG=1
USE_POCKETPY=1
USE_OMP=1

srcs = voxelSurface.cpp voxelSurface_write.cpp smoothGauss.cpp voxelToSurfMesh.cpp 
msCFLAGS += -I../common -I../voxlib -I../src  -I../pkgs/stb -I../pkgs/zlib
include ${msSrc}/script/Makefile.in

test:; @echo "vxlToSurf tests skipped"
