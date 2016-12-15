INCLUDEPATH *= $$PWD/common
Debug: LIBS += -L$$PWD/common/lib/debug/ -lcommon
Release: LIBS += -L$$PWD/common/lib/release/ -lcommon

win32 {# Prevent rebuild and Enable debuging in release mode
    QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_LFLAGS_RELEASE += /DEBUG
}

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