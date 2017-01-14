#pragma once
#include "../utilities/mathlib.h"
#include <qgl.h>

class CModel;

const double EdgeLength_Threshold = 0.1;
const double Area_Threshold = 0.02;
const double AngleThreshold = 1;

class SuppPlane
{
public:
	enum PlaneType
	{
		AABBPlane = 0,
		OBBPlane
	};

	SuppPlane(void);
	~SuppPlane(void);

	SuppPlane(std::vector<MathLib::Vector3> &PointSet);
	SuppPlane(MathLib::Vector3 newCorners[4]);
	SuppPlane(const MathLib::Vector3 &planePt, const MathLib::Vector3 &planeNormal, double);
	SuppPlane(std::vector<MathLib::Vector3> &PointSet, const std::vector<MathLib::Vector3> obbAxis);

	void updateToAABBPlane(std::vector<MathLib::Vector3> &PointSet);
	void updateToOBBPlane(std::vector<MathLib::Vector3> &PointSet, const std::vector<MathLib::Vector3> obbAxis);
	void computeParas();

	void setModel(CModel *m) { m_model = m; };
	void setModelID(int id) { m_modelID = id; };
	int getModelID() { return m_modelID; };
	void setSuppPlaneID(int id) { m_planeID = id; };
	int getSuppPlaneID() { return m_planeID; };

	void BuildAABBPlane(std::vector<MathLib::Vector3> &PointSet);
	void BuildOBBPlane(std::vector<MathLib::Vector3> &PointSet, const std::vector<MathLib::Vector3> obbAxis);

	void Draw(QColor c);
	void draw();
	void setColor(QColor c) { m_color = c; };

	MathLib::Vector3 getNormal() { return m_normal; };
	double getPlaneD() { return m_planeD; };
	MathLib::Vector3* GetCorners() {return m_corners;};
	MathLib::Vector3 GetCenter() {return m_center;};
	MathLib::Vector3 GetCorner(int i) {return m_corners[i];};
	double getYRange() { return m_width;};
	double getXRange() {return m_length;};
	double GetZ() {return m_center[2];};	
	double GetArea() { return m_width*m_length; };
	std::vector<MathLib::Vector3> GetAxis() { return m_axis; };

	MathLib::Vector2 getRandomSamplePos();
	bool isTooSmall(double sceneMetric);
	bool isCoverPos(double x, double y);


	std::vector<double> convertToAABBPlane();
	void transformPlane(const MathLib::Matrix4d &transMat);

	void initGrid();
	void updateGrid(std::vector<MathLib::Vector3> &obbCorners, int gridValue);
	void recoverGrid(std::vector<std::vector<int>> gridPos);

private:
	double m_length;
	double m_width;
	MathLib::Vector3 m_center;
	MathLib::Vector3 m_normal;
	double m_planeD;

	int m_planeType;

	std::vector<MathLib::Vector3> m_axis;

	MathLib::Vector3 m_corners[4];
	
	std::vector<MathLib::Vector3> m_SuppPointSet;

	CModel *m_model;
	int m_modelID;
	int m_planeID; //  which plane in the support plane list

	QColor m_color;

	std::vector<std::vector<int>> m_planeGrid;

};

