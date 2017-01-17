#include "SuppPlane.h"
#include "CModel.h"

const double GridSize = 0.05;   // 5cm, NEED TO CONSIDER SCENE METRIC!!


SuppPlane::SuppPlane(void)
{
	m_corners.resize(4);
	m_axis.resize(2);
	m_sceneMetric = 1.0;
}

SuppPlane::~SuppPlane(void)
{

}


// build plane based on ax+by+cz+d=0;
// should be infinite area plane, here set with to be 0.5; will update later
SuppPlane::SuppPlane(const MathLib::Vector3 &planePt, const MathLib::Vector3 &planeNormal, double planeD)
{
	m_corners.resize(4);
	m_axis.resize(2);

	m_normal = planeNormal;
	m_center = planePt;
	m_planeD = planeD;

	m_axis[0] = MathLib::Vector3(1, 0, 0);
	m_axis[1] = MathLib::Vector3(0, 1, 0);

	m_width = 0.5;

	m_corners[0] = m_center - m_axis[0] * m_width - m_axis[1] * m_width;
	m_corners[1] = m_center + m_axis[0] * m_width - m_axis[1] * m_width;
	m_corners[2] = m_center + m_axis[0] * m_width + m_axis[1] * m_width;
	m_corners[3] = m_center - m_axis[0] * m_width + m_axis[1] * m_width;

	m_sceneMetric = 1.0;

	m_planeType = AABBPlane;
}

SuppPlane::SuppPlane(const std::vector<MathLib::Vector3> &PointSet, bool isOrderedCorner)
{
	m_corners.resize(4);
	m_axis.resize(2);
	m_sceneMetric = 1.0;

	if (!isOrderedCorner)
	{
		BuildAABBPlane(PointSet);
	}
	else
	{
		m_corners[0] = PointSet[0];
		m_corners[1] = PointSet[1];
		m_corners[2] = PointSet[2];
		m_corners[3] = PointSet[3];

		computeParas();
		m_planeType = AABBPlane;
	}
}

SuppPlane::SuppPlane(const std::vector<MathLib::Vector3> &PointSet, const std::vector<MathLib::Vector3> obbAxis)
{
	m_corners.resize(4);
	m_axis.resize(2);
	m_sceneMetric = 1.0;

	BuildOBBPlane(PointSet, obbAxis);
}

void SuppPlane::BuildAABBPlane(const std::vector<MathLib::Vector3> &PointSet)
{
	m_SuppPointSet = PointSet;

	// axis aligned plane
	double min_x = 1e6;
	double min_y = 1e6;
	double max_x = -1e6;
	double max_y = -1e6;
	double center_z = 0;

	for (unsigned int i = 0; i<PointSet.size(); i++)
	{
		MathLib::Vector3 currPoint = PointSet[i];
		if (currPoint[0] > max_x)
		{
			max_x = currPoint[0];
		}
		if (currPoint[0] < min_x)
		{
			min_x = currPoint[0];
		}

		if (currPoint[1] > max_y)
		{
			max_y = currPoint[1];
		}
		if (currPoint[1] < min_y)
		{
			min_y = currPoint[1];
		}

		center_z += PointSet[i][2];
	}

	center_z = center_z/PointSet.size();
	m_corners[0] = MathLib::Vector3(min_x, min_y, center_z);
	m_corners[1] = MathLib::Vector3(max_x, min_y, center_z);
	m_corners[2] = MathLib::Vector3(max_x, max_y, center_z);
	m_corners[3] = MathLib::Vector3(min_x, max_y, center_z);

	computeParas();
	m_planeType = AABBPlane;
}

void SuppPlane::BuildOBBPlane(const std::vector<MathLib::Vector3> &PointSet, const std::vector<MathLib::Vector3> obbAxis)
{
	m_axis[0] = obbAxis[0];
	m_axis[1] = obbAxis[1];

	m_axis[0].normalize();
	m_axis[1].normalize();

	MathLib::Vector3 setCenter(0, 0, 0);

	for (unsigned int i = 0; i < PointSet.size(); i++)
	{
		setCenter += PointSet[i];
	}

	setCenter /= PointSet.size();

	std::vector<MathLib::Vector3> centerToPointVec(PointSet.size());

	for (unsigned int i = 0; i < PointSet.size(); i++)
	{
		centerToPointVec[i] = PointSet[i] - setCenter;
	}

	double max_a0_x = -1e6;
	double min_a0_x = 1e6;
	double max_a1_y = -1e6;
	double min_a1_y = 1e6;

	for (unsigned int i = 0; i < PointSet.size(); i++)
	{
		double pt_a0_comp = centerToPointVec[i].dot(m_axis[0]);

		if (pt_a0_comp > max_a0_x)
		{
			max_a0_x = pt_a0_comp;
		}

		if (pt_a0_comp < min_a0_x)
		{
			min_a0_x = pt_a0_comp;
		}

		double pt_a1_comp = centerToPointVec[i].dot(m_axis[1]);

		if (pt_a1_comp > max_a1_y)
		{
			max_a1_y = pt_a1_comp;
		}

		if (pt_a1_comp < min_a1_y)
		{
			min_a1_y = pt_a1_comp;
		}
	}

	// min_x, max_y is negative
	m_corners[0] = setCenter + m_axis[0] * min_a0_x + m_axis[1] * min_a1_y;
	m_corners[1] = setCenter + m_axis[0] * max_a0_x + m_axis[1] * min_a1_y;
	m_corners[2] = setCenter + m_axis[0] * max_a0_x + m_axis[1] * max_a1_y;
	m_corners[3] = setCenter + m_axis[0] * min_a0_x + m_axis[1] * max_a1_y;

	computeParas();
	m_planeType = OBBPlane;
}

void SuppPlane::Draw(QColor c)
{
	double zD = 0.01;

	glDisable(GL_LIGHTING);
	glColor4d(c.redF(), c.greenF(), c.blueF(), c.alphaF());
	glBegin(GL_QUADS);
	glVertex3f(m_corners[0][0], m_corners[0][1], m_corners[0][2] + zD);
	glVertex3f(m_corners[1][0], m_corners[1][1], m_corners[1][2] + zD);
	glVertex3f(m_corners[2][0], m_corners[2][1], m_corners[2][2] + zD);
	glVertex3f(m_corners[3][0], m_corners[3][1], m_corners[3][2] + zD);
	glEnd();
	glEnable(GL_LIGHTING);
}

void SuppPlane::draw(double sceneMetric)
{
	double zD = 0.01/sceneMetric;
	
	// draw plane
	glDisable(GL_LIGHTING);
	glColor4d(m_color.redF(), m_color.greenF(), m_color.blueF(), m_color.alphaF());
	glBegin(GL_QUADS);
	glVertex3f(m_corners[0][0], m_corners[0][1], m_corners[0][2] + zD);
	glVertex3f(m_corners[1][0], m_corners[1][1], m_corners[1][2] + zD);
	glVertex3f(m_corners[2][0], m_corners[2][1], m_corners[2][2] + zD);
	glVertex3f(m_corners[3][0], m_corners[3][1], m_corners[3][2] + zD);
	glEnd();

	// draw U
	glColor4d(255,0,0,255);
	glLineWidth(5.0);

	glBegin(GL_LINES);
	glVertex3d(m_corners[0][0], m_corners[0][1], m_corners[0][2]);
	glVertex3d(m_corners[1][0], m_corners[1][1], m_corners[1][2]);
	glEnd();

	// draw right point
	glPointSize(10);
	glColor4d(255, 0, 0, 255);
	glBegin(GL_POINTS);
	glVertex3d(m_corners[1][0], m_corners[1][1], m_corners[1][2]);
	glEnd();

	// draw V
	glColor4d(0, 255, 0, 255);
	glLineWidth(5.0);

	glBegin(GL_LINES);
	glVertex3d(m_corners[0][0], m_corners[0][1], m_corners[0][2]);
	glVertex3d(m_corners[3][0], m_corners[3][1], m_corners[3][2]);
	glEnd();

	// draw right point
	glPointSize(10);
	glColor4d(0, 255, 0, 255);
	glBegin(GL_POINTS);
	glVertex3d(m_corners[3][0], m_corners[3][1], m_corners[3][2]);
	glEnd();

	glEnable(GL_LIGHTING);
}

bool SuppPlane::isTooSmall(double sceneMetric)
{
	if (m_width < EdgeLength_Threshold / sceneMetric || m_length < EdgeLength_Threshold / sceneMetric || m_width*m_length < Area_Threshold / sceneMetric)
	{
		return true;
	}

	else
	{
		return false;
	}
}

void SuppPlane::computeParas()
{
	m_center = (m_corners[0] + m_corners[1] + m_corners[2] + m_corners[3])*0.25;
	m_length = (m_corners[1] - m_corners[0]).magnitude();
	m_width = (m_corners[2] - m_corners[1]).magnitude();

	m_axis[0] = (m_corners[1] - m_corners[0]);
	m_axis[1] = (m_corners[3] - m_corners[0]);

	m_axis[0].normalize();
	m_axis[1].normalize();

	m_normal = m_axis[0].cross(m_axis[1]);

	//initGrid();
}

std::vector<double> SuppPlane::convertToAABBPlane()
{
	std::vector<double> aabbcorners(4);

	double xmin, ymin, xmax, ymax;
	xmin = ymin = 1e6; // x, y min
	xmax = ymax = -1e6; // x, y max

	for (int i = 0; i < 4; i++)
	{
		if (m_corners[i][0] < xmin)
		{
			xmin = m_corners[i][0];
		}

		if (m_corners[i][0] > xmax)
		{
			xmax = m_corners[i][0];
		}

		if (m_corners[i][1] < ymin)
		{
			ymin = m_corners[i][1];
		}

		if (m_corners[i][1] > ymax)
		{
			ymax = m_corners[i][1];
		}
	}

	aabbcorners[0] = xmin;
	aabbcorners[1] = xmax;
	aabbcorners[2] = ymin;
	aabbcorners[3] = ymax;

	return aabbcorners;
}

void SuppPlane::updateToAABBPlane(std::vector<MathLib::Vector3> &PointSet)
{
	BuildAABBPlane(PointSet);
}

void SuppPlane::updateToOBBPlane(std::vector<MathLib::Vector3> &PointSet, const std::vector<MathLib::Vector3> obbAxis)
{
	BuildOBBPlane(PointSet, obbAxis);
}

void SuppPlane::transformPlane(const MathLib::Matrix4d &transMat)
{
	for (int i = 0; i < 4; i++)
	{
		m_corners[i] = transMat.transform(m_corners[i]);
	}

	computeParas();
}

MathLib::Vector2 SuppPlane::getRandomSamplePos()
{
	//double xRatio = std::rand() % 100 / 100.0;
	//double yRatio = std::rand() % 100 / 100.0;

	double xRatio, yRatio;

	GenTwoRandomDouble(0.25, 0.8, xRatio, yRatio);

	if (m_planeType == AABBPlane)
	{
		return MathLib::Vector2(m_corners[0][0] + xRatio*m_length, m_corners[0][1] + yRatio*m_width);
	}
	else
	{
		MathLib::Vector3 randPt;
		randPt = m_corners[0] + m_axis[0] * m_length*xRatio + m_axis[1] * m_width*yRatio;
		return MathLib::Vector2(randPt[0], randPt[1]);
	}

}

bool SuppPlane::isCoverPos(double x, double y)
{
	MathLib::Vector2 testPos(x, y);
	double boundWidth = 0.08/m_sceneMetric;

	double d;
	MathLib::Vector2 dv = testPos - MathLib::Vector2(m_corners[0][0], m_corners[0][1]);
	d = m_axis[0].dot(dv);
	if (d<boundWidth || d>m_length - boundWidth) {
		return false;
	}
	d = m_axis[1].dot(dv);
	if (d<boundWidth || d>m_width - boundWidth) {
		return false;
	}

	return true;
}

void SuppPlane::initGrid()
{
	m_planeGrid.clear();

	int xGridNum, yGridNum;

	xGridNum = (int)(m_length / (GridSize/m_sceneMetric));
	yGridNum = (int)(m_width / (GridSize/m_sceneMetric));

	m_planeGrid.resize(yGridNum, std::vector<int>(xGridNum, 0));
}

void SuppPlane::updateGrid(std::vector<MathLib::Vector3> &obbCorners, int gridValue)
{
	// update grid value using aabb box

}

void SuppPlane::recoverGrid(std::vector<std::vector<int>> gridPos)
{
	for (int i = 0; i < gridPos.size(); i++)
	{
		for (int j = 0; j < gridPos.size(); j++)
		{
			m_planeGrid[i][j] = 0;
		}
	}
}

MathLib::Vector3 SuppPlane::GetPlaneOrigin()
{
	return m_corners[0];
}

std::vector<double> SuppPlane::GetUVForPoint(const MathLib::Vector3 &pt)
{
	MathLib::Vector3 projectPt;

	MathLib::Vector3 planeOrigin = m_corners[0];
	MathLib::Vector3 toOrigninVec = pt - planeOrigin;

	// compute projected x and y to get uv
	double u = toOrigninVec.dot(m_axis[0]) / m_length;
	double v = toOrigninVec.dot(m_axis[1]) / m_width;

	if (u <= 0 || u >= 1 || v <= 0 || v >= 1)
	{
		qDebug() << "UV coordinate wrong: " << u << " " << v << "\n";
	}

	std::vector<double> uv;
	uv.push_back(u);
	uv.push_back(v);

	return uv;
}

double SuppPlane::GetPointToPlaneDist(const MathLib::Vector3 &pt)
{
	MathLib::Vector3 ptToCenterVec = pt - m_center;

	double d = ptToCenterVec.dot(m_normal);

	return d;
}
