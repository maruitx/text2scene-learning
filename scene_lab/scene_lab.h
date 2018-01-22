#ifndef SCENE_LAB_H
#define SCENE_LAB_H

#include <QObject>
#include "StarlabDrawArea.h"
#include "../common/geometry/CMesh.h"



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

	void loadSceneWithName(const QString &sceneFullName, int metaDataOnly = 0, int obbOnly = 0, int reComputeOBB = 0, int updateModelCat = 1);

	void loadSceneListNamesFromDBListFile();
	void loadSceneFileNamesFromSceneListFile(const QString &sceneDBName, const QString &sceneListFileName, std::map<QString, QStringList> &loadedSceneFileNames);

	void LoadWholeSceneList(int metaDataOnly = 0, int obbOnly = 0, int reComputeOBB = 0, int updateModelCat = 1);

	void InitModelDBs();
	void initShapeNetDB();
	void initTsinghuaDB();
	void initSunCGDB();


	void loadModelCatsMapTsinghua();
	void updateModelCatForTsinghuaScene(CScene *s);  // update model category for tsinghua scenes

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
	QStringList m_sceneDBList;
	std::map<QString, QStringList> m_loadedSceneFileNames;
	CScene *m_currScene;

	SceneSemGraph *m_currSceneSemGraph;

	RelationModelManager *m_relationModelManager;  // singleton
	RelationExtractor *m_relationExtractor;  // singleton

	ModelDatabase *m_shapeNetModelDB;
	ModelDBViewer_widget *m_modelDBViewer_widget;

	ModelDatabase *m_sunCGModelDB;

	std::map<QString, CMesh> m_meshDatabase;  // database for saving loaded meshes; to speed up mesh loading time

	std::map<QString, QString> m_modelCatMapTsinghua; // model category mapping from tsinghua to stanford

	QString m_projectPath;
	QString m_sceneDBType;
	QString m_localSceneDBPath;
	double m_angleTh;
};

#endif // SCENE_LAB_H
