#ifndef SCENE_LAB_H
#define SCENE_LAB_H

#include <QObject>

class scene_lab_widget;
class CScene;
class SceneSemGraph;
class ModelDatabase;
class ModelDBViewer_widget;
class RelationModelManager;

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
	void loadSceneList();

	void loadModelMetaInfoForScene();

	void buildRelationGraphForCurrentScene();
	void buildRelationGraphForSceneList();

	void buildSemGraphForCurrentScene(int batchLoading = 0);
	void buildSemGraphForSceneList();

	void buildRelationModels();

	void collectModelInfoForSceneList();

	void updateSceneRenderingOptions();

	void create_modelDBViewer_widget();
	void destory_modelDBViewer_widget();

signals:
	void sceneLoaded();
	void sceneRenderingUpdated();

private:
	scene_lab_widget *m_widget;
	
	std::vector<CScene*> m_sceneList;
	CScene *m_currScene;

	SceneSemGraph *m_currSceneSemGraph;

	RelationModelManager *m_relationModelManager;

	ModelDatabase *m_modelDB;
	ModelDBViewer_widget *m_modelDBViewer_widget;
};

#endif // SCENE_LAB_H
