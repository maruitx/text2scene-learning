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
	geometry/IO_3DS.h \
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
	geometry/IO_3DS.cpp \
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
		LIBS += -L$$PWD/third_party/Lib3DS/lib/win32 -llib3ds-2_0

    } else {
		Debug: LIBS += -L$$PWD/third_party/opcode/lib/x64 -lOPCODE_D
		Release: LIBS += -L$$PWD/third_party/opcode/lib/x64 -lOPCODE
		LIBS += -L$$PWD/third_party/Lib3DS/lib/x64 -llib3ds-2_0
    }
}

# Copy dll for lib3DS
CONFIG(debug, debug|release){
	EXTRA_BINFILES += $$PWD/third_party/Lib3DS/lib/x64/lib3ds-2_0.dll
} else {
	EXTRA_BINFILES += $$PWD/third_party/Lib3DS/lib/x64/lib3ds-2_0.dll
}		
        
		EXTRA_BINFILES_WIN = $${EXTRA_BINFILES}
        EXTRA_BINFILES_WIN ~= s,/,\\,g
        DESTDIR_WIN = $${EXECUTABLEPATH}
        DESTDIR_WIN ~= s,/,\\,g
        for(FILE,EXTRA_BINFILES_WIN){
            # message("Will copy file" $$FILE "to" $$DESTDIR_WIN)
            QMAKE_POST_LINK += $$quote(cmd /c copy /y $${FILE} $${DESTDIR_WIN}$$escape_expand(\n\t))
        }





{# Prevent rebuild and Enable debuging in release mode
	QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_LFLAGS_RELEASE += /DEBUG
}