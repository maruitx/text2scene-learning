#include "SuppPlaneManager.h"
#include "CModel.h"
#include "CMesh.h"
#include <algorithm>
//#include "SimplePointCloud.h"
#include "SuppPlane.h"
#include "../third_party/clustering/Kmeans.h"
#include <QFile>
#include <set>
#include <algorithm>

const double DistTh = 0.1;
const double AreaRatio = 0.2;
const double AreaTh = 0.4;


SuppPlaneManager::SuppPlaneManager()
{
}

SuppPlaneManager::SuppPlaneManager(CModel *m)
{
	m_model = m;
	m_upRightVec = MathLib::Vector3(0, 0, 1);

	m_sceneMetric = m_model->getSceneMetric();
}


SuppPlaneManager::~SuppPlaneManager()
{

}

//void SuppPlaneManager::build(int method)
//{
//	m_method = method;
//	//collectSuppPtsSet();
//
//	for (unsigned int i = 0; i < m_SuppPtsSet.size(); i++)
//	{
//		if (m_SuppPtsSet[i].size() > 0)
//		{
//			//m_suppPlanes.push_back(new SuppPlane(m_SuppPtsSet[i], m_model->getOBBAxis()));
//
//
//			SuppPlane* currPlane = m_suppPlanes[m_suppPlanes.size() - 1];
//			currPlane->setModelID(m_model->getID());
//
//			if (currPlane->isTooSmall())
//			{
//				m_suppPlanes.pop_back();
//				delete currPlane;
//			}
//		}
//	}
//
//	// plane id w.r.t current supplane list
//	for (unsigned int i = 0; i < m_suppPlanes.size(); i++)
//	{
//		m_suppPlanes[i]->setSuppPlaneID(i);
//	}
//}

//// need to think: didn't consider model transform
//void SuppPlaneManager::collectSuppPtsSet()
//{
//	SimplePointCloud& pts = m_model->getPointCloud();
//	std::vector<Surface_mesh::Point> SupportPointSoup;
//
//	if (pts.isEmpty())
//	{
//		m_model->extractToPointCloud();
//		pts = m_model->getPointCloud();
//	}
//
//	Surface_mesh::Vertex_property<Surface_mesh::Vector3> vpoints = m_model->meshData()->vertex_property<Surface_mesh::Vector3>("v:point");
//	Surface_mesh::Vertex_property<Surface_mesh::Vector3> vnormals = m_model->meshData()->vertex_property<Surface_mesh::Vector3>("v:normal");
//	Surface_mesh::Vertex_iterator vit, vend = m_model->meshData()->vertices_end();
//
//	
//	int id = 0;
//	for (vit = m_model->meshData()->vertices_begin(); vit != vend; vit++)
//	{
//		//// need to fix: perhaps use face normals
//		//MathLib::Vector3 vn = MathLib::Vector3(vnormals[vit][0], vnormals[vit][1], vnormals[vit][2]);
//		//if (MathLib::Acos(vn.dot(m_upRightVec)) < AngleThreshold)
//		//{
//		//	SupportPointSoup.push_back(pts[id]);
//		//}
//
//		if (m_model->isVertexUpRight(vit, AngleThreshold))
//		{
//			SupportPointSoup.push_back(pts[id]);
//		}
//
//		id++;
//	}
//
//	if (!SupportPointSoup.empty())
//	{
//		seperateSuppSoupByZlevel(SupportPointSoup);
//	}
//}
//
//void SuppPlaneManager::seperateSuppSoupByZlevel(const std::vector<Surface_mesh::Point> &pts)
//{
//	// similar to nearest neighbor classifier
//	std::vector<double> allZvals;
//	double Gap_Threshold = 0.1;
//
//	for (int i = 0; i < pts.size(); i++)
//	{
//		allZvals.push_back(pts[i][2]);
//	}
//
//	double zMax = *std::max_element(allZvals.begin(), allZvals.end());
//	double zMin = *std::min_element(allZvals.begin(), allZvals.end());
//
//	double zRange = zMax - zMin;
//	//double zGap = Gap_Threshold*zRange;
//
//	if (zRange != 0)
//	{
//		std::vector<double> tempZLevelVals(zRange / Gap_Threshold + 1, 0);
//		std::vector<std::vector<MathLib::Vector3>> tempSuppPtsSet(tempZLevelVals.size());
//
//		for (int i = 0; i < allZvals.size(); i++)
//		{
//			int zID = (allZvals[i] - zMin) / Gap_Threshold;
//
//			tempZLevelVals[zID] = zMin + zID*Gap_Threshold;
//			tempSuppPtsSet[zID].push_back(MathLib::Vector3(pts[i][0], pts[i][1], pts[i][2]));
//		}
//
//		for (int i = 0; i < tempZLevelVals.size(); i++)
//		{
//			if (tempZLevelVals[i] != 0)
//			{
//				m_zLevelVals.push_back(tempZLevelVals[i]);
//			}
//
//			if (tempSuppPtsSet[i].size() != 0)
//			{
//				m_SuppPtsSet.push_back(tempSuppPtsSet[i]);
//			}
//		}
//	}
//	else
//	{
//		std::vector<MathLib::Vector3> tempSuppPts;
//
//		for (int i = 0; i < pts.size(); i++)
//		{
//			tempSuppPts.push_back(MathLib::Vector3(pts[i][0], pts[i][1], pts[i][2]));
//		}
//
//		m_SuppPtsSet.push_back(tempSuppPts);
//	}
//}

void SuppPlaneManager::draw()
{
	for (unsigned int i = 0; i < m_suppPlanes.size(); i++)
	{
		//QColor c = GetColorFromSet(m_model->getID());
		//m_suppPlanes[i]->Draw(c);

		m_suppPlanes[i]->draw();
	}
}

SuppPlane* SuppPlaneManager::getLargestAreaSuppPlane()
{
	double maxArea = -1e6;
	int maxPlaneID = -1;

	if (!m_suppPlanes.empty())
	{
		for (unsigned int i = 0; i < m_suppPlanes.size(); i++)
		{
			double currArea = m_suppPlanes[i]->GetArea();
			if (currArea >= maxArea)
			{
				double currZ = m_suppPlanes[i]->GetZ();
				maxArea = currArea;
				maxPlaneID = i;
			}
		}

		return m_suppPlanes[maxPlaneID];
	}

	else
		return NULL;
}

SuppPlane* SuppPlaneManager::getHighestSuppPlane()
{
	int maxZPlaneID = -1;
	double maxZ = -1e6;

	for (unsigned int i = 0; i < m_suppPlanes.size(); i++)
	{
		double currZ = m_suppPlanes[i]->GetZ();
		if (currZ > maxZ)
		{
			maxZ = currZ;
			maxZPlaneID = i;
		}
	}

	return m_suppPlanes[maxZPlaneID];
}

std::vector<int> SuppPlaneManager::clusteringMeshFacesSuppPlane()
{
	m_mesh = m_model->getMesh();

	// clustering mesh faces by face normal
	// compute data matrix
	std::vector<MathLib::Vector3> faceNormals = m_mesh->getfaceNormals();
	int faceNum = faceNormals.size();
	std::vector<int> clusterIndicator(faceNum, -1);
	
	// clustering face normal
	//Eigen::MatrixXd faceNormalMat(faceNum, 3);
	//for (int i = 0; i < faceNum; i++)
	//{
	//	faceNormalMat(i, 0) = faceNormals[i][0];
	//	faceNormalMat(i, 1) = faceNormals[i][1];
	//	faceNormalMat(i, 2) = faceNormals[i][2];
	//}

	//int K = 6;
	//std::vector<std::vector<int>> clusters = Kmeans::cluster(faceNormalMat, K);

	//for (int i = 0; i < clusters.size(); i++)
	//{
	//	if (clusters[i].size() > 0)
	//	{
	//		for (int j = 0; j < clusters[i].size(); j++)
	//		{
	//			int faceId = clusters[i][j];
	//			clusterIndicator[faceId] = i;
	//		}
	//	}
	//}

	// collect upright faces
	std::vector<int> upFaceIds;
	for (int i = 0; i < faceNum; i++)
	{
		if (faceNormals[i].dot(m_upRightVec) > 0.9)
		{
			upFaceIds.push_back(i);
		}
	}
	
	// clustering faces by z value
	Eigen::MatrixXd zValueMat(upFaceIds.size(), 1);

	m_verts = m_mesh->getVertices();
	m_faces = m_mesh->getFaces();
	m_faceAreas.resize(m_faces.size(), 0);
	m_faceBarycenters.resize(m_faces.size());

	// compute face barycenter and use z value of barycenter
	// compute face areas
	for (int i = 0; i < upFaceIds.size(); i++)
	{
		int f_id = upFaceIds[i];
		m_faceBarycenters[f_id] = (m_verts[m_faces[f_id][0]] + m_verts[m_faces[f_id][1]] + m_verts[m_faces[f_id][2]]) / 3;
		zValueMat(i, 0) = m_faceBarycenters[f_id][2];

		MathLib::Vector3 v0 = m_verts[m_faces[f_id][0]];
		MathLib::Vector3 v1 = m_verts[m_faces[f_id][1]];
		MathLib::Vector3 v2 = m_verts[m_faces[f_id][2]];

		m_faceAreas[f_id] = 0.5*(v1 - v0).cross(v2 - v0).magnitude(); // tri area
	}

	int K = std::min((int)upFaceIds.size(), 15);
	std::vector<std::vector<int>> clusters = Kmeans::cluster(zValueMat, K);

	// refine cluster
	//for (int c = 0; c < clusters.size(); c++)
	//{
	//	double clusterZmax(-1e6), clusterZmin(1e6);

	//	for (int i = 0; i < clusters[c].size(); i++)
	//	{
	//		if (zValueMat(clusters[c][i], 0) > clusterZmax)
	//		{
	//			clusterZmax = zValueMat(clusters[c][i], 0);
	//		}

	//		if (zValueMat(clusters[c][i], 0) < clusterZmin)
	//		{
	//			clusterZmin = zValueMat(clusters[c][i], 0);
	//		}

	//		if (clusterZmax - clusterZmin > 0.2)
	//		{
	//		}
	//	}
	//}


	std::vector<double> xyRange = m_model->getAABBXYRange();
	double xyArea = (xyRange[1] - xyRange[0])*(xyRange[3] - xyRange[2]);

	std::vector<std::vector<int>> clusteredFaceIds;

	for (int i = 0; i < clusters.size(); i++)
	{
		double clusterFaceArea = 0;

		if (clusters[i].size() > 0)
		{

			for (int j = 0; j < clusters[i].size(); j++)			
			{
				int f_id = upFaceIds[clusters[i][j]];

				clusterFaceArea += m_faceAreas[f_id];

				clusterIndicator[f_id] = i;  // to show the up faces
			}

			// filter out faces with small area
			if (clusterFaceArea < AreaTh / m_sceneMetric)
			{
				continue;
			}
			else
			{
				std::vector<int> currFaceIds;
				for (int j = 0; j < clusters[i].size(); j++)
				{
					int f_id = upFaceIds[clusters[i][j]];
					currFaceIds.push_back(f_id);
				}
				clusteredFaceIds.push_back(currFaceIds);
			}
		}
	}

	for (int i = 0; i < clusteredFaceIds.size(); i++)
	{
		SuppPlane * p = fitPlaneToFaces(clusteredFaceIds[i]);

		if (p== NULL)
		{
			continue;
		}

		//find inlier tris
		std::set<int> inlierVertIds;
		std::vector<MathLib::Vector3> inlierVerts;
		for (int fi = 0; fi < clusteredFaceIds[i].size(); fi++)
		{
			int f_id = clusteredFaceIds[i][fi];

			double distToPlane = m_faceBarycenters[f_id].dot(p->getNormal()) + p->getPlaneD();

			if (std::abs(distToPlane) < DistTh / m_sceneMetric)
			{
				//clusterIndicator[f_id] = i;   // only show faces that are fitted with plane
				inlierVertIds.insert(m_faces[f_id][0]);
				inlierVertIds.insert(m_faces[f_id][1]);
				inlierVertIds.insert(m_faces[f_id][2]);
			}
		}

		std::set<int>::iterator vit;
		for (vit = inlierVertIds.begin(); vit != inlierVertIds.end(); vit++)
		{
			inlierVerts.push_back(m_verts[*vit]);
		}

		p->updateToOBBPlane(inlierVerts, m_model->getOBBAxis());


		if (p->isTooSmall(m_sceneMetric))
		{
			delete p;
			continue;
		}

		p->setModelID(m_model->getID());
		p->setColor(GetColorFromSet(i));

		m_suppPlanes.push_back(p);
		p->setSuppPlaneID(m_suppPlanes.size() - 1);
	}
	
	sortFaceIdsByHeight();

	return clusterIndicator;
}

SuppPlane* SuppPlaneManager::fitPlaneToFaces(const std::vector<int> &faceIds)
{
	// collect face verts
	std::set<int> faceVertIds;

	for (int j = 0; j < faceIds.size(); j++)
	{
		faceVertIds.insert(m_faces[faceIds[j]][0]);
		faceVertIds.insert(m_faces[faceIds[j]][0]);
		faceVertIds.insert(m_faces[faceIds[j]][0]);
	}

	std::vector<int> uniqueFaceVertIds(faceVertIds.begin(), faceVertIds.end());

	// random sample 3 verts
	int vertNum = uniqueFaceVertIds.size();
	int numIter = 100;

	std::vector<double> votedAreas(numIter);
	std::vector<MathLib::Vector3> sampledNormals(numIter);
	std::vector<MathLib::Vector3> sampledPlanePts(numIter);
	std::vector<double> sampledDs(numIter);

	for (int iIter = 0; iIter < numIter; iIter++)
	{
		std::vector<int> sampleVertIds;

		for (int vi = 0; vi < 3; vi++)
		{
			bool flag;
			do
			{
				flag = false;

				int randVertId = std::rand() % vertNum;

				for (int si = 0; si < sampleVertIds.size(); si++)
				{
					if (randVertId == sampleVertIds[si])
					{
						flag = true;
						break;
					}
				}

				if (!flag)
				{
					sampleVertIds.push_back(uniqueFaceVertIds[randVertId]);
				}

			} while (flag);
		}

		// fit a plane to sampled verts
		MathLib::Vector3 e0 = m_verts[sampleVertIds[1]] - m_verts[sampleVertIds[0]];
		MathLib::Vector3 e1 = m_verts[sampleVertIds[2]] - m_verts[sampleVertIds[0]];
		MathLib::Vector3 planeNormal = e0.cross(e1);
		planeNormal.normalize();

		double pD = -m_verts[sampleVertIds[0]].dot(planeNormal);

		// vote for this plane by summing up the inner tri areas
		double fittedArea = 0;
		for (int fi = 0; fi < faceIds.size(); fi++)
		{
			int f_id = faceIds[fi];

			double distToPlane = m_faceBarycenters[f_id].dot(planeNormal) + pD;

			if (std::abs(distToPlane) < DistTh / m_sceneMetric)
			{
				fittedArea += m_faceAreas[f_id];
			}
		}

		sampledNormals[iIter] = planeNormal;
		sampledPlanePts[iIter] = m_verts[sampleVertIds[0]];
		sampledDs[iIter] = pD;
		votedAreas[iIter] = fittedArea;
	}

	std::vector<double>::iterator result;
	result = std::max_element(votedAreas.begin(), votedAreas.end());
	int maxAreaPlaneId = std::distance(votedAreas.begin(), result);

	// fit with a infinite plane
	SuppPlane* p = new SuppPlane(sampledPlanePts[maxAreaPlaneId], sampledNormals[maxAreaPlaneId], sampledDs[maxAreaPlaneId]);

	return p;

	//std::vector<int> candidatePlaneIds;
	//for (int i = 0; i < votedAreas.size(); i++)
	//{
	//	if (votedAreas[i] > 0.8*votedAreas[maxAreaPlaneId])
	//	{
	//		candidatePlaneIds.push_back(i);
	//	}
	//}

	//std::vector<SuppPlane*> candidatePlanes;
	//std::vector<double> planeHeights;

	//for (int i = 0; i < candidatePlaneIds.size(); i++)
	//{
	//	int id = candidatePlaneIds[i];
	//	SuppPlane* p = new SuppPlane(sampledPlanePts[id], sampledNormals[id], sampledDs[id]);
	//	planeHeights.push_back(p->GetZ());
	//	candidatePlanes.push_back(p);
	//}
	//std::vector<double>::iterator maxHeightIter;
	//maxHeightIter = std::max_element(planeHeights.begin(), planeHeights.end());
	//int maxHeightPlaneId = std::distance(planeHeights.begin(), maxHeightIter);

	//for (int i = 0; i < candidatePlaneIds.size(); i++)
	//{
	//	if (i!=maxHeightPlaneId)
	//	{
	//		delete candidatePlanes[i];
	//	}
	//}
	//return candidatePlanes[maxHeightPlaneId];
}

void SuppPlaneManager::transformSuppPlanes(const MathLib::Matrix4d &transMat)
{
	for (int i = 0; i < m_suppPlanes.size(); i++)
	{
		m_suppPlanes[i]->transformPlane(transMat);
	}
}

// TO DO: fix
void SuppPlaneManager::pruneSuppPlanes()
{
	QString catName = m_model->getCatName();

	if (!m_suppPlanes.empty() && catName!="room")
	{
		// keep the largest and highest plane
		SuppPlane* p = this->getLargestAreaSuppPlane();
		double maxArea = p->GetArea();
		SuppPlane *q;

		for (int i = 0; i < m_suppPlanes.size(); i++)
		{
			int pid = m_faceIdsByHeight[i];
			
			if (m_suppPlanes[pid]->GetArea() > 0.5*maxArea)
			{
				q = m_suppPlanes[pid];
				break;
			}
		}

		m_suppPlanes.clear();
		q->setSuppPlaneID(0);
		m_suppPlanes.push_back(q);
	}

	if (catName.contains("room") || catName == "unknown")
	{
		SuppPlane *floor = NULL;

		foreach(SuppPlane *p, m_suppPlanes)
		{
			if (abs(p->GetZ()) < 0.2)
			{
				floor = p;
			}
		}
		
		if (floor!=NULL)
		{
			floor->setSuppPlaneID(0);
			m_suppPlanes.push_back(floor);
		}

		m_suppPlanes.clear();
	}
}

void SuppPlaneManager::sortFaceIdsByHeight()
{
	std::vector<std::pair<double, int>> faceHs;

	for (int i = 0; i < m_suppPlanes.size(); i++)
	{
		faceHs.push_back(std::make_pair(m_suppPlanes[i]->GetZ(), i));
	}

	std::sort(faceHs.begin(), faceHs.end());
	std::reverse(faceHs.begin(), faceHs.end());

	m_faceIdsByHeight.resize(m_suppPlanes.size());

	for (int i = 0; i < m_suppPlanes.size(); i++)
	{
		m_faceIdsByHeight[i] = faceHs[i].second;
	}
}

void SuppPlaneManager::saveSuppPlane()
{
	QString suppPlaneFilename;
	QString modelFilePath = m_model->getModelFilePath();
	QString modelNameStr = m_model->getNameStr();

	suppPlaneFilename = modelFilePath + "/" + modelNameStr + ".supp";

	QFile suppFile(suppPlaneFilename);

	QTextStream ofs(&suppFile);

	if (!suppFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
	{
		std::cout << "\t cannot save to file " << suppPlaneFilename.toStdString() << "\n";
		return;
	};

	for (int i = 0; i < m_suppPlanes.size(); i++)
	{
		MathLib::Vector3 *corners = m_suppPlanes[i]->GetCorners();

		for (int c = 0; c < 4; c++)
		{
			ofs << corners[c][0] << " " << corners[c][1] << " " << corners[c][2] <<"\n";
		}
	}

	suppFile.close();

	std::cout << "\t support plane saved\n";
}

bool SuppPlaneManager::loadSuppPlane()
{
	// should load support plane after OBB is loaded

	QString suppPlaneFilename;
	QString modelFilePath = m_model->getModelFilePath();
	QString modelNameStr = m_model->getNameStr();

	suppPlaneFilename = modelFilePath + "/" + modelNameStr + ".supp";

	QFile suppFile(suppPlaneFilename);

	QTextStream ifs(&suppFile);

	if (!suppFile.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

	std::vector<MathLib::Vector3> suppCorners, obbAxis;
	obbAxis = m_model->getOBBAxis();

	int planeID = 0;

	while (!ifs.atEnd())
	{
		//QString currLine = ifs.readLine();
		//QStringList lineparts = currLine.split(" ");

		suppCorners.clear();
		for (int i = 0; i < 4; i++)
		{
			//MathLib::Vector3 corner(lineparts[3 * i].toDouble(), lineparts[3 * i+1].toDouble(), lineparts[3 * i+2].toDouble());
			//suppCorners.push_back(corner);
			MathLib::Vector3 corner;
			QString buff;
			ifs >> buff;
			if (buff[0].isDigit() || buff[0]=='-')
			{
				corner[0] = buff.toDouble();
				ifs >> corner[1] >> corner[2];
				//ifs >> buff;  // read rest of line "\n"
			}
			else
				break;

			suppCorners.push_back(corner);
		}

		if (!suppCorners.empty())
		{
			SuppPlane *p = new SuppPlane(suppCorners, obbAxis);

			p->setModelID(m_model->getID());
			p->setColor(GetColorFromSet(planeID));
			p->setSuppPlaneID(planeID);
			m_suppPlanes.push_back(p);
			planeID++;
		}
		else
		{
			break;
		}

	}

	suppFile.close();

	std::cout << "SuppPlaneManager: support plane loaded\n";
	return true;
}
