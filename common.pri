INCLUDEPATH *= $$PWD/common
Debug: LIBS += -L$$PWD/common/lib/debug/ -lcommon
Release: LIBS += -L$$PWD/common/lib/release/ -lcommon

INCLUDEPATH *= "C:/Program Files/MATLAB/R2016b/extern/include"
LIBS += -L"C:/Program Files/MATLAB/R2016b/extern/lib/win64/microsoft" -llibmx -llibeng


# mlpack
#INCLUDEPATH *= $$PWD/common/third_party/mlpack/include
#LIBS += -L$$PWD/common/third_party/mlpack/lib -lmlpack -lmlpack_gmm_train -lmlpack_gmm_probability -lmlpack_gmm_generate

#INCLUDEPATH *= $$PWD/common/third_party/boost_1_63_0/include
#LIBS += -L$$PWD/common/third_party/boost_1_63_0/lib64-msvc-14.0

#INCLUDEPATH *= $$PWD/common/third_party/armadillo-7.800.2/include

#LIBS += -L$$PWD/common/third_party/OpenBLAS.0.2.14.1/lib/x64 libopenblas.dll.a
#LIBS += -L$$PWD/common/third_party/lapack -llapack_win64_MT

# Opcode lib
INCLUDEPATH *= $$PWD/common/third_party/opcode/include
win32 {
    !contains(QMAKE_TARGET.arch, x86_64) {
		Debug: LIBS += -L$$PWD/common/third_party/opcode/lib/win32 -lOPCODE_D
		Release: LIBS += -L$$PWD/common/third_party/opcode/lib/win32 -lOPCODE		
    } else {
		Debug: LIBS += -L$$PWD/common/third_party/opcode/lib/x64 -lOPCODE_D
		Release: LIBS += -L$$PWD/common/third_party/opcode/lib/x64 -lOPCODE	
    }
}