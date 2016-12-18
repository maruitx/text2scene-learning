#pragma once

class CScene;
class SceneGraph;
class ModelDatabase;
class MetaModel;

class SemNode{
public:
	SemNode(const QString &t, const QString &n, int id) { nodeType = t; nodeName = n; nodeId = id; };
	~SemNode();

	// node types: object, p_attribute, p_relation, g_relation, g_attribute, 
	QString nodeType;
	
	// model category name, relationship name or attribute name, e.g. chair, support, messy
	QString nodeName;

	int nodeId;
	
	// node id of the other end of in edge
	// for object node: saves the non-object node (relations, attributes) id to the object node, e.g. table <-- wood,  chair <-- support
	// for relation/attribute node: saves passive object id to relation/attribute node, e.g. support <-- table
	std::vector<int> inEdgeNodeList;  
	
	// node id of the other end of out edge
	// for relation/attribute node: from relation/attribute node to object, e.g. messy --> book, messy --> utensils
	// for object: from passive object to relation/attribute node, e.g. table --> support
	std::vector<int> outEdgeNodeList; 

};

class SemEdge{
public:
	SemEdge(int s, int t, int id) { sourceNodeId = s, targetNodeId = t; edgeId = id; };
	~SemEdge();

	int sourceNodeId, targetNodeId;
	int edgeId;
};

class SceneSemGraph
{
public:
	SceneSemGraph(CScene *s, ModelDatabase *db);
	~SceneSemGraph();

	void addNode(const QString &nType, const QString &nName);
	void addEdge(int s, int t);

	void generateGraph();
	
	void buildFromModelDBAnnotation();  // low-level attribute node and edges to object node
	void extractRelationsFromRelationGraph();  // low-level/high-level relation node and edges to object node
	void loadAttributeNodeFromAnnotation(); // high-level attribute node and edges to object node
	void saveGraph();

private:
	CScene *m_scene;
	QString m_sceneFormat;
	SceneGraph *m_relGraph;

	ModelDatabase *m_modelDB;
	
	int m_nodeNum, m_edgeNum;
	std::vector<SemNode> m_nodes;
	std::vector<SemEdge> m_edges;

	std::vector<int> m_nonObjNodeIds;  // ids for relation/attribute node
	//std::map<QString, int> m_modelStringToNodeIdMap;
	std::vector<MetaModel*> m_metaModelList;
};

