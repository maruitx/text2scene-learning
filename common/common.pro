include($$[STARLAB])

QT*=xml opengl widgets
win32:LIBS += -lopengl32 -lglu32

# LOADS EIGEN
INCLUDEPATH *= $$[EIGENPATH]
DEFINES *= EIGEN

TEMPLATE = lib
CONFIG += staticlib

# Build flag
CONFIG(debug, debug|release) {CFG = debug} else {CFG = release}

# Library name and destination
TARGET = common
DESTDIR = $$PWD/lib/$$CFG

DEPENDPATH += geometry 
DEPENDPATH += utilities 
				
INCLUDEPATH += geometry
INCLUDEPATH += utilities

INCLUDEPATH *= "C:/Program Files/MATLAB/R2016b/extern/include"
LIBS += -L"C:/Program Files/MATLAB/R2016b/extern/lib/win64/microsoft" -llibmx -llibeng -libmex

HEADERS += \
	geometry/Scene.h \
	geometry/CModel.h \
	geometry/CMesh.h \
	geometry/AABB.h \
	geometry/OBB.h \
	geometry/OBBEstimator.h \
	geometry/BestFit.h \
	geometry/TriTriIntersect.h \
	geometry/RelationGraph.h \
	geometry/UDGraph.h \
	geometry/SuppPlane.h \
	geometry/SuppPlaneManager.h \
	third_party/clustering/Kmeans.h \
	utilities/utility.h \
	utilities/mathlib.h \
	utilities/Eigen3x3.h \
	utilities/eigenMat.h

	
SOURCES += \
	geometry/Scene.cpp \
	geometry/CModel.cpp \
	geometry/CMesh.cpp \
	geometry/AABB.cpp \
	geometry/OBB.cpp \
	geometry/OBBEstimator.cpp \
	geometry/BestFit.cpp \
	geometry/TriTriIntersect.cpp \
	geometry/RelationGraph.cpp \
	geometry/UDGraph.cpp \
	geometry/SuppPlane.cpp	\
	geometry/SuppPlaneManager.cpp \
	third_party/clustering/Kmeans.cpp \
	utilities/mathlib.cpp 
	
# Opcode lib
INCLUDEPATH *= $$PWD/third_party/opcode/include


win32 {
    !contains(QMAKE_TARGET.arch, x86_64) {
		Debug: LIBS += -L$$PWD/third_party/opcode/lib/win32 -lOPCODE_D
		Release: LIBS += -L$$PWD/third_party/opcode/lib/win32 -lOPCODE

    } else {
		Debug: LIBS += -L$$PWD/third_party/opcode/lib/x64 -lOPCODE_D
		Release: LIBS += -L$$PWD/third_party/opcode/lib/x64 -lOPCODE
    }
}

{# Prevent rebuild and Enable debuging in release mode
	QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_LFLAGS_RELEASE += /DEBUG
}