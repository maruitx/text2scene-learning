#pragma once

#include "../common/utilities/utility.h"
#include "SemanticGraph.h"

class CScene;
class RelationGraph;
class ModelDatabase;
class DBMetaModel;
class RelationExtractor;
class CModel;

const int NodeTypeNum = 5;
const int SingleAttriNum = 7;
const int PairRelNum = 10;
const int GroupRelNum = 5;
const int GroupAttriNum = 8;

const QString SSGNodeType[] = { "object", "per_obj_attribute", "pairwise_relationship", "group_relationship", "group_attribute" };
const QString SingleAttriStrings[] = {"round", "rectangular", "office", "dining", "kitchen", "floor", "wall"};

const QString GroupAttrStings[] = {"messy", "clean", "organized", "disorganized", "formal", "casual", "spacious", "crowded"};

struct GroupAnnotation 
{
	QString name;
	int refModelId;
	std::vector<int> actModelIds;
};

class SceneSemGraph : public SemanticGraph
{
public:

	SceneSemGraph(CScene *s, ModelDatabase *db, RelationExtractor *relationExtractor);
	~SceneSemGraph();

	void loadGraph(const QString &filename);
	void saveGraph();

	void generateGraph();	
	void addModelDBAnnotation();  // low-level attribute node and edges to object node
	
	void addRelationsFromRelationGraph();  // low-level/high-level relation node and edges to object node
	
	void addSpatialSideRel(); // only extract the side info for objs with support level 0
	void addSpatialSideRelForModelPair(int refModelId, int testModelId);

	void addGroupAttributeFromAnnotation(); // high-level attribute node and edges to object node

	void saveNodeStringToLabelIDMap(const QString &filename);
	void saveGMTNodeAttributeFile(const QString &filename);

	QString getCatName(int modelId);

private:
	CScene *m_scene;
	QString m_sceneFormat;
	RelationGraph *m_relGraph;

	ModelDatabase *m_modelDB;
	RelationExtractor *m_relationExtractor;  // pointer to the singleton
	
	//std::vector<int> m_nonObjNodeIds;  // ids for relation/attribute node
	std::vector<DBMetaModel*> m_metaModelList;
	int m_modelNum;

	std::map<QString, int> m_catStringToLabelIDMap;
};

