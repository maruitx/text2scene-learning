#pragma once

#include "../common/utilities/utility.h"
#include "SemanticGraph.h"

class CScene;
class SceneGraph;
class ModelDatabase;
class MetaModel;


class SceneSemGraph : public SemanticGraph
{
public:
	SceneSemGraph(CScene *s, ModelDatabase *db);
	~SceneSemGraph();

	void loadGraph(const QString &filename);
	void saveGraph();

	void generateGraph();	
	void buildFromModelDBAnnotation();  // low-level attribute node and edges to object node
	void extractRelationsFromRelationGraph();  // low-level/high-level relation node and edges to object node
	void loadAttributeNodeFromAnnotation(); // high-level attribute node and edges to object node

private:
	CScene *m_scene;
	QString m_sceneFormat;
	SceneGraph *m_relGraph;

	ModelDatabase *m_modelDB;
	
	//std::vector<int> m_nonObjNodeIds;  // ids for relation/attribute node
	//std::map<QString, int> m_modelStringToNodeIdMap;
	std::vector<MetaModel*> m_metaModelList;
	int m_modelNum;
};

