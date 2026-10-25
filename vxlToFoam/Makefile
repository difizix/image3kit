
msSrc ?= $(abspath $(if $(wildcard ../../script/Makefile.in),../..,../../../src)) # flat porescale, or <root>/src

mains = voxelToFoam voxelToFoamPar

all: $(mains);

USE_SINGLECPP=1
USE_ZLIB=1
USE_TIFF=1
USE_VOX_IMG=1
USE_POCKETPY=1
USE_OMP=1

msCFLAGS += -I../common -I../voxlib -I../src  -I../pkgs/stb -I../pkgs/zlib
include ${msSrc}/script/Makefile.in

test:; @echo "vxlToFoam tests skipped"
