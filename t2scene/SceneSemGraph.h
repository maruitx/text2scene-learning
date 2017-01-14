#pragma once

#include "../common/utilities/utility.h"
#include "SemanticGraph.h"

class CScene;
class RelationGraph;
class ModelDatabase;
class DBMetaModel;

const int NodeTypeNum = 5;
const int SingleAttriNum = 7;
const int PairRelNum = 10;
const int GroupRelNum = 5;
const int GroupAttriNum = 8;

const QString SSGNodeType[] = { "object", "per_obj_attribute", "pairwise_relationship", "group_relationship", "group_attribute" };
const QString SSGSingleAttriStrings[] = {"round", "rectangular", "office", "dining", "kitchen", "floor", "wall"};
const QString SSGPairRelStrings[] = {"vert_support", "horizon_support", "contain", "above", "below", "leftside", "rightside", "front", "back", "near"};
const QString SSGGroupRelStrings[] = {"around", "aligned", "grouped", "stacked", "scattered"};
const QString SSGGroupAttrStings[] = {"messy", "clean", "organized", "disorganized", "formal", "casual", "spacious", "crowded"};

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
	
	void extractSpatialSideRel(); // only extract the side info for objs with support level 0
	void extractSpatialSideRelForModelPair(int refModelId, int testModelId);
	std::vector<QString> computeSpatialSideRelForModelPair(int refModelId, int testModelId);

	void loadAttributeNodeFromAnnotation(); // high-level attribute node and edges to object node

	void saveNodeStringToLabelIDMap(const QString &filename);
	void saveGMTNodeAttributeFile(const QString &filename);

private:
	CScene *m_scene;
	QString m_sceneFormat;
	RelationGraph *m_relGraph;

	ModelDatabase *m_modelDB;
	
	//std::vector<int> m_nonObjNodeIds;  // ids for relation/attribute node
	std::vector<DBMetaModel*> m_metaModelList;
	int m_modelNum;

	std::map<QString, int> m_catStringToLabelIDMap;
};

