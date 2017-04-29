#pragma once
#include "UDGraph.h"
#include "../utilities/utility.h"

class CScene;

class RelationGraph : public CUDGraph
{
public:
	typedef enum {
		CT_VERT_SUPPORT = 0,
		CT_HORIZON_SUPPORT,
		CT_CONTACT,
		CT_CONTAIN,
		CT_PROXIMITY,
		CT_SYMMETRY,
		CT_WEAK,
		CT_SUP_SYM,
		CT_CONTACT_SYM,
		CT_PROX_SYM,
		CT_WEAK_SYM
	} ConnType;

	RelationGraph();
	RelationGraph(CScene *s);
	~RelationGraph();

	void buildGraph();
	void updateGraph(int modelID); // update graph after add a new model
	void updateGraph(int modelID, int suppModelID);  // update graph with known support model

	void collectSupportParentForModels();
	std::vector<std::vector<int>> getSupportParentListForModels() { return m_supportParentListForModels; };

	void drawGraph();

	int readGraph(const QString &filename);
	int saveGraph(const QString &filename);

	double getSuppTh() { return m_SuppThresh; };


private:
	int buildSuppEdgesFromFile(); // for StanfordSceneDatabase
	int extractSupportRel(); //
	void correctSupportEdgeDir(); // v1: parent, v2:child
	int pruneSupportRel();
	int updateSupportRel(int modelID); // update support relationship after insert a new model into the scene

private:
	CScene *m_scene;
	int m_nodeNum;

	std::vector<std::vector<int>>				m_supportParentListForModels;			// on top list
	//CDistMat<double>					m_modelDistMat;			// model center distance matrix
	//CDistMat<double>					m_ConnStrenMat;			// connection strength matrix

	//std::vector<std::vector<int>>	m_SymGrp;
	//std::vector<int>				m_SymMap;

	double m_SuppThresh;
	double m_sceneMetric;
};

