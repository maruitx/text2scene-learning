INCLUDEPATH *= $$PWD/common
Debug: LIBS += -L$$PWD/common/lib/debug/ -lcommon
Release: LIBS += -L$$PWD/common/lib/release/ -lcommon

# Matlab
INCLUDEPATH *= "C:/Program Files/MATLAB/R2016b/extern/include"
LIBS += -L"C:/Program Files/MATLAB/R2016b/extern/lib/win64/microsoft" -llibmx -llibeng -llibmex

# Opcode lib
INCLUDEPATH *= $$PWD/common/third_party/opcode/include
win32 {
    !contains(QMAKE_TARGET.arch, x86_64) {
		Debug: LIBS += -L$$PWD/common/third_party/opcode/lib/win32 -lOPCODE_D
		Release: LIBS += -L$$PWD/common/third_party/opcode/lib/win32 -lOPCODE
		LIBS += -L$$PWD/common/third_party/Lib3DS/lib/win32 -llib3ds-2_0

		
    } else {
		Debug: LIBS += -L$$PWD/common/third_party/opcode/lib/x64 -lOPCODE_D
		Release: LIBS += -L$$PWD/common/third_party/opcode/lib/x64 -lOPCODE
		LIBS += -L$$PWD/common/third_party/Lib3DS/lib/x64 -llib3ds-2_0		
    }
}


