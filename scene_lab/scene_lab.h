#ifndef SCENE_LAB_H
#define SCENE_LAB_H

#include <QObject>
#include "StarlabDrawArea.h"


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

	void setDrawArea(Starlab::DrawArea *drawArea);

	CScene *getScene() { return m_currScene; };

	void loadParas();

	// load scene list
	void loadSceneFileNamesFromListFile(std::vector<QStringList> &loadedSceneFileNames);
	void LoadSceneList(int metaDataOnly = 0, int obbOnly = 0, int meshAndOBB = 0, int updateModelCat = 1);

	void loadModelCatsMap();
	void updateModelCatForScene(CScene *s);  // update model category for tsinghua scenes

public slots:
	void LoadScene();

	void updateModelMetaInfoForScene(CScene *s);  // update model meta info for stanford or scenenn scenes

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

	// categories
	void ExtractModelCatsFromSceneList();
	void ExtractMetaFileForSceneList();

	void updateSceneRenderingOptions();

	void create_modelDBViewer_widget();
	void destory_modelDBViewer_widget();

	void testMatlab();

	// screen shots
	void ScreenShotForCurrScene();
	void ScreenShotForSceneList();

signals:
	void sceneLoaded();
	void sceneRenderingUpdated();

private:
	scene_lab_widget *m_widget;
	Starlab::DrawArea *m_drawArea;
	
	std::vector<CScene*> m_sceneList;
	std::vector<QStringList> m_loadedSceneFileNames;
	CScene *m_currScene;

	SceneSemGraph *m_currSceneSemGraph;

	RelationModelManager *m_relationModelManager;  // singleton
	RelationExtractor *m_relationExtractor;  // singleton

	ModelDatabase *m_modelDB;
	ModelDBViewer_widget *m_modelDBViewer_widget;
	std::map<QString, QString> m_modelCatMap; // model category mapping from tsinghua to stanford

	QString m_sceneDBType;
	QString m_localSceneDBPath;
	double m_angleTh;
};

#endif // SCENE_LAB_H
