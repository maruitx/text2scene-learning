#pragma once

#include "../common/utilities/utility.h"

class SemNode{
public:
	SemNode(const QString &t, const QString &n, int id) { nodeType = t; nodeName = n; nodeId = id; };
	~SemNode() {};

	// node types: e.g., object, per_obj_attribute, pairwise_relation, group_relation, group_relation_anno, 
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

	// parsed from in and out edge node list
	std::vector<int> nodeLabels;  // idx of per-object attribute node, valid for "object" node
	std::vector<int> activeNodeList; // idx of active object node, valid for "relation" node
	std::vector<int> anchorNodeList; // idx of anchor object node, valid for "relation" node

};

// directed edge, from source to target
class SemEdge{
public:
	SemEdge(int s, int t, int id) { sourceNodeId = s, targetNodeId = t; edgeId = id; };
	~SemEdge() {};

	int sourceNodeId, targetNodeId;
	int edgeId;
};

class SemanticGraph
{
public:
	SemanticGraph();
	~SemanticGraph();

	void addNode(const QString &nType, const QString &nName);
	void addEdge(int s, int t); // after adding edge, parseNodeNeighbors must be called to update the neighbor list
	void parseNodeNeighbors();  // parse in and out node list to node labels, active and anchor node list

public:
	int m_nodeNum, m_edgeNum;
	std::vector<SemNode> m_nodes;
	std::vector<SemEdge> m_edges;
};

