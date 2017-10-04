include($$[STARLAB])
include( ../common.pri )
include( ../scene_lab.pri )


StarlabTemplate(plugin)


FORMS += \
	text_scene_widget.ui 

HEADERS += \
	text2scene_mode.h \
	text_scene_widget.h \
	SemanticGraph.h \
	SceneSemGraph.h \
	ScenePatch.h \
	SynScene.h

	
SOURCES += \
	text2scene_mode.cpp \
	text_scene_widget.cpp \
	SemanticGraph.cpp \
	SceneSemGraph.cpp \
	ScenePatch.cpp \
	SynScene.cpp
	
RESOURCES += \
	text2scene.qrc
	
{# Prevent rebuild and Enable debuging in release mode
	QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_LFLAGS_RELEASE += /DEBUG
}
