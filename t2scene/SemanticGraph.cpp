#include "SemanticGraph.h"


SemanticGraph::SemanticGraph()
	: m_nodeNum(0), m_edgeNum(0)
{
}


SemanticGraph::~SemanticGraph()
{
}

void SemanticGraph::addNode(const QString &nType, const QString &nName)
{
	SemNode newNode = SemNode(nType, nName, m_nodeNum);
	m_nodes.push_back(newNode);

	m_nodeNum++;
}

void SemanticGraph::addEdge(int s, int t)
{
	SemEdge newEdge = SemEdge(s, t, m_edgeNum);

	m_edges.push_back(newEdge);

	// update node edge list
	m_nodes[s].outEdgeNodeList.push_back(t);
	m_nodes[t].inEdgeNodeList.push_back(s);

	m_edgeNum++;
}

void SemanticGraph::parseNodeNeighbors()
{
	for (int i = 0; i < m_nodeNum; i++)
	{
		SemNode& sgNode = m_nodes[i];

		if (sgNode.nodeType == "object")
		{
			sgNode.nodeLabels.clear();

			for (int ai = 0; ai < sgNode.inEdgeNodeList.size(); ai++)
			{
				int attNodeId = sgNode.inEdgeNodeList[ai];  // id of attribute node in tsg
				SemNode &attNode = this->m_nodes[attNodeId];

				if (attNode.nodeType.contains("attribute"))
				{
					sgNode.nodeLabels.push_back(attNodeId);
				}
			}
		}

		if (sgNode.nodeType.contains("relation") || sgNode.nodeType.contains("group_attribute"))
		{
			sgNode.activeNodeList.clear();
			sgNode.anchorNodeList.clear();

			// edge dir: (active, relation), (relation, reference)

			for (int ai = 0; ai < sgNode.inEdgeNodeList.size(); ai++)
			{
				int inNodeId = sgNode.inEdgeNodeList[ai];
				sgNode.activeNodeList.push_back(inNodeId);
			}

			for (int ai = 0; ai < sgNode.outEdgeNodeList.size(); ai++)
			{
				int outNodeId = sgNode.outEdgeNodeList[ai];
				sgNode.anchorNodeList.push_back(outNodeId);
			}
		}
	}
}
