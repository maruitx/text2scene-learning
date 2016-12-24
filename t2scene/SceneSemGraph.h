#pragma once

#include "../common/utilities/utility.h"
#include "SemanticGraph.h"

class CScene;
class RelationGraph;
class ModelDatabase;
class MetaModel;

const QString SSGNodeType[5] = { "object", "per_obj_attribute", "pairwise_relationship", "group_relationship", "group_attribute" };
const QString SSGRelationStrings[15] = {"horizon_support", "vert_support", "contain", "above", "below", "leftside", "rightside", "front", "back", "near",
"around", "aligned", "grouped", "stacked", "scattered"};
const QString SSGAttributeStings[8] = {"messy", "clean", "organized", "disorganized", "formal", "casual", "spacious", "crowded"};

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
	RelationGraph *m_relGraph;

	ModelDatabase *m_modelDB;
	
	//std::vector<int> m_nonObjNodeIds;  // ids for relation/attribute node
	//std::map<QString, int> m_modelStringToNodeIdMap;
	std::vector<MetaModel*> m_metaModelList;
	int m_modelNum;
};

