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

	void loadParas();

	// load scene list
	void loadSceneFileNamesFromListFile(std::vector<QStringList> &loadedSceneFileNames);
	void LoadSceneList(int metaDataOnly = 0, int obbOnly = 0, int meshAndOBB = 0);

	// categories
	void extractModelCatsFromSceneList();

public slots:
	void LoadScene();

	void updateModelMetaInfoForScene(CScene *s);

	// obb
	void BuildOBBForSceneList();

	// structure graph
	void BuildRelationGraphForCurrentScene();
	void BuildRelationGraphForSceneList();

	// ssg
	void BuildSemGraphForCurrentScene();
	void BuildSemGraphForSceneList();

	// relational model
	void ComputeBBAlignMatForSceneList();
	void ExtractRelPosForSceneList();
	void ExtractSuppProbForSceneList();

	void BuildRelativeRelationModels();
	void BuildPairwiseRelationModels();
	void BuildGroupRelationModels();

	void BatchBuildModelsForList();

	void ExtractMetaFileForSceneList();

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

	QString m_sceneDBType;
	QString m_localSceneDBPath;
	double m_angleTh;
};

#endif // SCENE_LAB_H
