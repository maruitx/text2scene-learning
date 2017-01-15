#pragma once
#include "../utilities/mathlib.h"
#include "../utilities/utility.h"

class CModel;
class CMesh;
class SuppPlane;

class SuppPlaneManager
{
public:
	enum BuildMethod{ AABB_PLANE, OBB_PLANE, CONVEX_HULL_PLANE };
	SuppPlaneManager();
	~SuppPlaneManager();

	SuppPlaneManager(CModel *m);

	//void build(int method);
	//void collectSuppPtsSet();
	//void seperateSuppSoupByZlevel(const std::vector<Surface_mesh::Point> &pts);

	std::vector<int> clusteringMeshFacesSuppPlane();
	SuppPlane* fitPlaneToFaces(const std::vector<int> &faceIds);
	void pruneSuppPlanes();

	void addSupportPlane(SuppPlane *p) { m_suppPlanes.push_back(p); };
	void clearSupportPlanes();

	void sortFaceIdsByHeight();

	void draw(double sceneMetric = 1.0);

	std::vector<SuppPlane*> getAllSuppPlanes() { return m_suppPlanes; };
	SuppPlane* getLargestAreaSuppPlane();
	SuppPlane* getHighestSuppPlane();
	SuppPlane* getSuppPlane(int id) { return m_suppPlanes[id]; };

	bool hasSuppPlane() { return !m_suppPlanes.empty(); };
	int getSuppPlaneNum() { return m_suppPlanes.size(); };

	// transformation
	void transformSuppPlanes(const MathLib::Matrix4d &transMat);

	bool loadSuppPlane();
	void saveSuppPlane();

private:
	CModel *m_model;
	CMesh *m_mesh;
	int m_method;

	double m_sceneMetric;

	// mesh info
	std::vector<double> m_faceAreas;
	std::vector<MathLib::Vector3> m_faceBarycenters;
	std::vector<MathLib::Vector3> m_verts;
	std::vector<std::vector<int>> m_faces;


	// each inner vector corresponds to a support plane
	std::vector<std::vector<MathLib::Vector3>> m_SuppPtsSet;
	std::vector<double> m_zLevelVals;

	std::vector<SuppPlane*> m_suppPlanes;
	MathLib::Vector3 m_upRightVec;

	std::vector<int> m_faceIdsByHeight;
};

