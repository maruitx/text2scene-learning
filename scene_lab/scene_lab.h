#ifndef SCENE_LAB_H
#define SCENE_LAB_H

#include <QObject>

class scene_lab_widget;
class CScene;
class SceneSemGraph;
class ModelDatabase;
class ModelDBViewer_widget;
class RelationModelManager;
class RelationExtractor;

class scene_lab : public QObject
{
	Q_OBJECT

public:
	scene_lab(QObject *parent = 0);
	~scene_lab();

	void create_widget();
	void destroy_widget();
	CScene *getScene() { return m_currScene; };

public slots:
	void loadScene();
	void loadSceneList(int metaDataOnly = 0, int obbOnly = 0, int meshAndOBB = 0);

	void updateModelMetaInfoForScene(CScene *s);

	// structure graph
	void buildRelationGraphForCurrentScene();
	void buildRelationGraphForSceneList();

	// ssg
	void buildSemGraphForCurrentScene(int batchLoading = 0);
	void buildSemGraphForSceneList();

	// relational model
	void computeBBAlignMatForSceneList();
	void extractRelPosForSceneList();
	void buildRelationModels();

	void collectModelInfoForSceneList();

	void updateSceneRenderingOptions();

	void create_modelDBViewer_widget();
	void destory_modelDBViewer_widget();

	void testMatlab();

signals:
	void sceneLoaded();
	void sceneRenderingUpdated();

private:
	scene_lab_widget *m_widget;
	
	std::vector<CScene*> m_sceneList;
	CScene *m_currScene;

	SceneSemGraph *m_currSceneSemGraph;

	RelationModelManager *m_relationModelManager;  // singleton
	RelationExtractor *m_relationExtractor;  // singleton

	ModelDatabase *m_modelDB;
	ModelDBViewer_widget *m_modelDBViewer_widget;
};

#endif // SCENE_LAB_H
