#include "CModel.h"
#include "CMesh.h"
#include "OBBEstimator.h"
#include "TriTriIntersect.h"
#include "SuppPlaneManager.h"
#include "SuppPlane.h"
#include "../utilities/utility.h"
#include "qgl.h"
#include <QFile>

CModel::CModel()
{
	m_catName = QString("unknown");
	m_mesh = NULL;

	m_suppPlaneManager = new SuppPlaneManager(this);
	m_hasOBB = false;

	m_fullTransMat.setidentity();
	m_lastTransMat.setidentity();
	m_initFrontDir = m_currFrontDir = MathLib::Vector3(0, -1, 0);
	m_initUpDir = m_currUpDir = MathLib::Vector3(0, 0, 1);

	m_sceneMetric = 1.0;
	m_modelMetric = 1.0;

	suppParentID = -1;
	parentSuppPlaneID = -1;
	supportLevel = -10;

	m_status = 0;

	m_hasSuppPlane = false;
	m_showDiffColor = true;
	m_showFaceClusters = false;

	m_readyForInterTest = false;

	m_isVisible = true;

	m_outputStatus = 0;

	m_isBusy = false;


}

CModel::~CModel()
{
	if (m_mesh != NULL)
	{
		delete m_mesh;
		m_mesh = NULL;
	}

	if (m_suppPlaneManager!=NULL)
	{
		delete m_suppPlaneManager;
		m_suppPlaneManager = NULL;
	}
}

bool CModel::loadModel(QString filename, double metric, int metaDataOnly, int obbOnly, int meshAndOBB, QString sceneDbType)
{
	m_modelMetric = metric;

	int cutPos = filename.lastIndexOf("/");
	m_filePath = filename.left(cutPos);
	m_fileName = filename.right(filename.size() - cutPos -1);   // contain .obj

	cutPos = m_fileName.lastIndexOf("."); 
	m_fileName = m_fileName.left(cutPos); // get rig of .obj

	m_nameStr = m_fileName;;


	if (m_fileName.contains("_"))
	{
		QStringList names = m_fileName.split("_");
		m_nameStr = names[names.size() - 1];
	}


	if (m_nameStr == "1ac1d0986508974bf1783a44a88d6275")
	{
		m_nameStr = "1ac1d0986508974bf1783a44a88d6274";      // this night stand has a naming problem in 3ds max
	}

	m_mesh = new CMesh(filename, m_nameStr);

	// still try to load obb, because we want to save the center of the model
	loadOBB();

	//  try to load support plane
	if (m_suppPlaneManager->loadSuppPlane())
	{
		if (m_suppPlaneManager->hasSuppPlane())
		{
			m_hasSuppPlane = true;
		}
	}
 
	
	if (metaDataOnly)
	{
		// return before loading obj data
		return true;
	}

	if (obbOnly)
	{
		bool flag = loadOBB();

		if (flag == -1)
		{
			std::cout << "\t OBB does not exist, please compute OBB first\n";
			return false;
		}
	}
	else
	{
		// load mesh data first
		std::cout << "\t \t loading mesh for " << m_nameStr.toStdString() << "\n";

		bool isLoaded;
		isLoaded = m_mesh->readObjFile(qPrintable(filename), metric, sceneDbType);

		if (!isLoaded)
		{
			return false;
		}

		computeAABB();
		m_initAABB = m_AABB;

		buildDisplayList(0, 0);

		if (meshAndOBB)  // load both obb and mesh, if obb not exist, compute obb
		{
			if (!m_hasOBB && loadOBB() == -1)
			{
				if (m_sceneUpVec == MathLib::Vector3(0, 0, 1))
				{
					computeOBB(2); // fix Z
				}
				else if (m_sceneUpVec == MathLib::Vector3(0, 1, 0))
				{
					computeOBB(1); // fix Y
				}

				m_hasOBB = true;

				saveOBB();
			}
		}
	}

	return true;
}

void CModel::saveModel(QString filename)
{
	m_mesh->saveObjFile(filename.toStdString());
}

void CModel::computeAABB()
{ 
	MathLib::Vector3 minVert = m_mesh->getMinVert();
	MathLib::Vector3 maxVert = m_mesh->getMaxVert();

	m_AABB.SetDataM(minVert, maxVert);
}

void CModel::draw(bool showModel, bool showOBB, bool showSuppPlane, bool showFrontDir, bool showSuppChildOBB)
{
	if (!m_isVisible)
	{
		return;
	}

	if (m_hasOBB && showOBB)
	{
		if (showSuppChildOBB && supportLevel == 0)
		{
			m_OBB.DrawBox(0, 0, 0, showFrontDir, 1);
		}
		else
		{
			m_OBB.DrawBox(0, 0, 0, showFrontDir, 0);
		}
	}

	if (showFrontDir)
	{
		showOBB = 1;
		drawFrontDir();
	}



	if (showModel)
	{
		glCallList(m_displayListID);
	}

	if (showSuppPlane && m_hasSuppPlane)
	{
		m_suppPlaneManager->draw(m_sceneMetric);
	}

	//m_AABB.DrawBox(0, 1, 0, 0, 0);
}

void CModel::buildDisplayList(int showDiffColor /*= 1*/, int showFaceCluster /*= 0*/)
{
	if (glIsList(m_displayListID))
	{
		glDeleteLists(m_displayListID, 1);
	}

	m_showDiffColor = showDiffColor;
	m_showFaceClusters = showFaceCluster;

	m_displayListID = glGenLists(1);

	QColor c;

	// draw model with same color
	if (showFaceCluster && !m_faceIndicators.empty())
	{
		glNewList(m_displayListID, GL_COMPILE);
		m_mesh->draw(m_faceIndicators);
		glEndList();
	}
	else
	{
		if (!showDiffColor)
		{
			//if (m_status == 1)
			//{
			//	c = GetColorFromSet(1);
			//}
			//else if (m_status == 2)  // re-arranged
			//{
			//	c = GetColorFromSet(3);
			//}
			//else if (m_status == 3) // inserted
			//{
			//	c = GetColorFromSet(5);
			//}
			//else
			//{
			//	c = QColor(180, 180, 180, 255);
			//}

			c = QColor(150, 150, 150, 255);
		}
		else
			c = GetColorFromSet(m_id);

		glNewList(m_displayListID, GL_COMPILE);
		m_mesh->draw(c);
		glEndList();
	}
}

void CModel::transformModel(const MathLib::Matrix4d &transMat)
{
	m_mesh->transformMesh(transMat);

	computeAABB(); // update
	
	if (m_suppPlaneManager != NULL && m_suppPlaneManager->hasSuppPlane())
	{
		m_suppPlaneManager->transformSuppPlanes(transMat);
	}

	// transform obb
	if (m_hasOBB)
	{
		m_OBB.Transform(transMat);
		m_currOBBPos = transMat.transform(m_initOBBPos);
	}

	// transform front dir
	m_currFrontDir = transMat.transformVec(m_currFrontDir);
	m_currFrontDir.normalize();
	m_currFrontDir = m_currFrontDir / m_sceneMetric;

	buildDisplayList();

	if (m_readyForInterTest)
	{
		updateForIntersect();
	}
	else
	{
		prepareForIntersect();
	}

	m_lastTransMat = transMat;
	m_fullTransMat = m_lastTransMat*m_fullTransMat;
}

void CModel::revertLastTransform()
{
	//MathLib::Matrix4d inverseMat = m_lastTransMat.invert();
	Eigen::Matrix4d eigenMat = convertToEigenMat(m_lastTransMat);
	Eigen::Matrix4d eigenInverseMat;
	if (eigenMat.determinant())
	{
		eigenInverseMat = eigenMat.inverse();
	}
	else
	{
		Simple_Message_Box("Transformation invertible");
	}
	eigenInverseMat = convertToEigenMat(m_lastTransMat).inverse();
	MathLib::Matrix4d inverseMat = convertToMatrix4d(eigenInverseMat);

	transformModel(inverseMat);

	m_lastTransMat.setidentity();
}

void CModel::transformModel(double tarOBBDiagLen, const MathLib::Vector3 &tarOBBPos, const MathLib::Vector3 &tarFrontDir)
{
	computeTransMat(tarOBBDiagLen, tarOBBPos, tarFrontDir);
	transformModel(m_lastTransMat);
}

void CModel::computeTransMat(double tarOBBDiagLen, const MathLib::Vector3 &tarOBBPos, const MathLib::Vector3 &tarFrontDir)
{
	MathLib::Matrix4d scaleMat, translateMat, rotMat;

	double scaleFactor = tarOBBDiagLen / m_initOBBDiagLen;
	scaleMat.setscale(scaleFactor, scaleFactor, scaleFactor);
	rotMat = GetRotMat(m_currFrontDir, tarFrontDir);

	m_lastTransMat = rotMat*scaleMat;

	MathLib::Vector3 translateVec = tarOBBPos - m_lastTransMat.transform(m_initOBBPos);
	m_lastTransMat.M[12] = translateVec[0]; m_lastTransMat.M[13] = translateVec[1]; m_lastTransMat.M[14] = translateVec[2];
}

void CModel::setStatus(int i)
{
	m_status = i;
}

bool CModel::isSegIntersectMesh(const MathLib::Vector3 &startPt, const MathLib::Vector3 &endPt, double radius, MathLib::Vector3 &intersectPoint /*= MathLib::Vector3(0, 0, 0)*/)
{
	return m_mesh->isSegIntersect(startPt, endPt, radius, intersectPoint);
}

void CModel::prepareForIntersect()
{
	m_mesh->buildOpcodeModel();
	m_readyForInterTest = true;
}

void CModel::buildSuppPlane()
{
	std::cout << "SuppPlaneManager: start computing support plane ...\n";
	m_faceIndicators = m_suppPlaneManager->clusteringMeshFacesSuppPlane();
		//m_suppPlaneManager->pruneSuppPlanes();  // DEBUG: just keep the largest supplane

	m_suppPlaneManager->saveSuppPlane();

	m_showFaceClusters = true;

	if (m_suppPlaneManager->hasSuppPlane())
	{
		m_hasSuppPlane = true;
	}
}

void CModel::builSuppPlaneUsingBBTop()
{
	// support plane should have consistent vertex order, w.r.t to the model front
	std::vector<MathLib::Vector3> corners(4); // need to delete after use to release memory
	SuppPlane *p;

	//if (m_hasOBB)
	//{
	//	corners[0] = m_OBB.vp[0];
	//	corners[1] = m_OBB.vp[4];
	//	corners[2] = m_OBB.vp[5];
	//	corners[3] = m_OBB.vp[1];

	//	SuppPlane *p = new SuppPlane(corners, );
	//}
	//else
	{
		std::cout << "SuppPlaneManager: start extracting support plane from AABB top for "<< m_nameStr.toStdString() <<"\n";

		corners[0] = m_AABB.vp[0];
		corners[1] = m_AABB.vp[4];
		corners[2] = m_AABB.vp[5];
		corners[3] = m_AABB.vp[1];
	}

	// adjust corner order based on the model front orientation
	std::vector<MathLib::Vector3> orderedCorners(4);
	MathLib::Vector3 modelFront = m_currFrontDir;

	modelFront.normalize();

	MathLib::Vector3 modelRight = getRightDir();

	MathLib::Vector3 modelCenter = m_AABB.cent;
	MathLib::Vector3 cornerToCenterVec;

	m_suppPlaneManager->clearSupportPlanes();


	for (int i = 0; i < 4; i++)
	{
		cornerToCenterVec = corners[i] - modelCenter;
		cornerToCenterVec.normalize();

		double dotBetweenFront = cornerToCenterVec.dot(modelFront);
		double dotBetweenRight = cornerToCenterVec.dot(modelRight);

		if (dotBetweenFront > 0 && dotBetweenRight > 0)
		{
			orderedCorners[0] = corners[i];
		}

		if (dotBetweenFront > 0 && dotBetweenRight < 0)
		{
			orderedCorners[1] = corners[i];
		}

		if (dotBetweenFront < 0 && dotBetweenRight < 0)
		{
			orderedCorners[2] = corners[i];
		}

		if (dotBetweenFront < 0 && dotBetweenRight > 0)
		{
			orderedCorners[3] = corners[i];
		}
	}

	p = new SuppPlane(orderedCorners, 1);
	p->setColor(GetColorFromSet(0));
	p->setModel(this);
	p->setModelID(m_id);
	p->setSuppPlaneID(0);

	m_suppPlaneManager->addSupportPlane(p);
	m_suppPlaneManager->saveSuppPlane();

	m_showFaceClusters = true;

	if (m_suppPlaneManager->hasSuppPlane())
	{
		m_hasSuppPlane = true;
	}
}

//
//double CModel::getLargestSuppPlaneHeight()
//{
//	SuppPlane *p = m_suppPlaneManager->getLargestAreaSuppPlane();
//	MathLib::Vector3 center = p->GetCenter();
//	return center[2];
//}
//
//SuppPlane* CModel::getLargestSuppPlane()
//{
//	return m_suppPlaneManager->getLargestAreaSuppPlane();
//}
//
//bool CModel::hasSuppPlane()
//{
//	return m_suppPlaneManager->hasSuppPlane();
//}

std::vector<double> CModel::getAABBXYRange()
{
	std::vector<double> rangeVals(4);   // (xmin, xmax, ymin, ymax)

	MathLib::Vector3 minV = m_mesh->getMinVert();
	MathLib::Vector3 maxV = m_mesh->getMaxVert();

	rangeVals[0] = minV[0];
	rangeVals[1] = maxV[0];

	rangeVals[2] = minV[1];
	rangeVals[3] = maxV[1];

	return rangeVals;
}

void CModel::computeOBB(int fixAxis /*= -1*/)
{	
	std::vector<MathLib::Vector3> verts = m_mesh->getVertices();

	COBBEstimator OBBE(&verts, &m_OBB);

	OBBE.ComputeOBB_Min(fixAxis);

	m_hasOBB = true;
}

int CModel::loadOBB(const QString &sPathName /*= QString()*/)
{
	QString sFilename;

	if (sPathName.isEmpty()) {
		sFilename = m_filePath + "/" + m_fileName + ".obb";
	}
	else {
		sFilename = sPathName;
	}

	std::ifstream ifs(sFilename.toStdString());
	if (!ifs.is_open()) {
		return -1;
	}

	while (!ifs.eof()) {
		m_OBB.ReadData(ifs);
	}

	m_hasOBB = true;

	// save the init info from file
	m_initOBBDiagLen = m_OBB.GetDiagLength();
	m_initOBBPos = getModelPosOBB();
	return 0;
}

int CModel::saveOBB(const QString &sPathName /*= QString()*/)
{
	QString sFilename;

	if (sPathName.isEmpty()) {
		sFilename = m_filePath + "/" + m_fileName + ".obb";
	}
	else {
		sFilename = sPathName;
	}

	std::ofstream ofs(sFilename.toStdString());
	if (!ofs.is_open()) {
		return -1;
	}

	m_OBB.WriteData(ofs);

	return 0;
}

bool CModel::IsSupport(CModel *pOther, bool roughOBB, double dDistT, const MathLib::Vector3 &Upright)
{
	if (pOther == NULL) {
		return false;
	}
	double dAngleT = 5.0;
	if (!m_AABB.IsIntersect(pOther->m_AABB, dDistT*2.0)) {	// if too distant
		return false;
	}

	if (!m_OBB.IsCoverCenter(pOther->m_OBB) && !pOther->m_OBB.IsCoverCenter(m_OBB))
	{
		return false;
	}

	if (roughOBB && m_OBB.IsCoverCenter(pOther->m_OBB))
	{
		return true;
	}

	if (m_OBB.IsSupport(pOther->m_OBB, dAngleT, dDistT, Upright)) {
		return true;
	}

	std::vector<MathLib::Vector3>& verts = m_mesh->getVertices();
	std::vector<std::vector<int>>& faces = m_mesh->getFaces();
	std::vector<MathLib::Vector3>& faceNormals = m_mesh->getfaceNormals();

	CMesh *pMeshOther = pOther->getMesh();
	std::vector<MathLib::Vector3>& vertsOther = pMeshOther->getVertices();
	std::vector<std::vector<int>>& facesOther = pMeshOther->getFaces();
	std::vector<MathLib::Vector3>& faceNormalsOther = pMeshOther->getfaceNormals();

	for (unsigned int fi = 0; fi < faces.size(); fi++)
	{
		std::vector<int> &FI = faces[fi];
		const MathLib::Vector3 &FNI = faceNormals[fi];
		if (MathLib::Acos(MathLib::Abs(FNI.dot(Upright))) > dAngleT) {
			continue;
		}

		for (unsigned int fj = 0; fj < facesOther.size(); fj++)
		{
			std::vector<int> &FJ = facesOther[fj];
			const MathLib::Vector3 &FNJ = faceNormalsOther[fj];

			if (MathLib::Acos(MathLib::Abs(FNJ.dot(Upright))) > dAngleT) {
				continue;
			}

			if (ContactTriTri(verts[FI[0]], verts[FI[1]], verts[FI[2]], FNI,
				vertsOther[FJ[0]], vertsOther[FJ[1]], vertsOther[FJ[2]], FNJ, 
				dAngleT, dDistT, true))
			{
				return true;
			}
		}
	}

	return false;
}

double CModel::getOBBBottomHeight()
{
	return m_OBB.GetBottomHeight(MathLib::Vector3(0,0,1));
}

//SuppPlane* CModel::getSuppPlane(int i)
//{
//	return m_suppPlaneManager->getSuppPlane(i);
//}

bool CModel::isOBBIntersectMesh(const COBB &testOBB)
{
	return m_mesh->isOBBIntersect(testOBB);
}

void CModel::updateForIntersect()
{
	m_mesh->updateOpcodeModel();
}

void CModel::selectOBBFace(const MathLib::Vector3 &origin, const MathLib::Vector3 &dir)
{
	double depth(std::numeric_limits<double>::max());

	m_OBB.PickByRay(origin, dir, depth);
}

MathLib::Vector3 CModel::getOBBFrontFaceCenter()
{
	std::vector<int> selOBBFaceIds = m_OBB.getSelQuadFaceIds();

	if (selOBBFaceIds.size() == 1 || selOBBFaceIds.size() == 2)
	{
		return m_OBB.GetFaceCent(selOBBFaceIds[0]);
	}
	else
		return m_OBB.GetFaceCent(3);  // (0,-1,0)
}

void CModel::drawFrontDir()
{
	MathLib::Vector3 startPt, endPt;

	startPt = m_OBB.cent;
	endPt = startPt + m_currFrontDir/m_sceneMetric;

	MathLib::Vector3 rightDir = getRightDir();
	MathLib::Vector3 endRight = startPt + rightDir / m_sceneMetric;

	MathLib::Vector3 endUp = startPt + m_currUpDir / m_sceneMetric;

	QColor red(0, 255, 0);
	QColor green(255,0,0);
	QColor blue(0,0,255);

	glDisable(GL_LIGHTING);

	// draw front dir as green
	glColor4d(red.redF(), red.greenF(), red.blueF(), red.alphaF());
	glLineWidth(5.0);

	glBegin(GL_LINES);
	glVertex3d(startPt[0], startPt[1], startPt[2]);
	glVertex3d(endPt[0], endPt[1], endPt[2]);
	glEnd();

	// draw front point
	glPointSize(10);
	glColor4d(green.redF(), green.greenF(), green.blueF(), green.alphaF());
	glBegin(GL_POINTS);
	glVertex3d(endPt[0], endPt[1], endPt[2]);
	glEnd();


	// draw right dir as red
	glColor4d(green.redF(), green.greenF(), green.blueF(), green.alphaF());
	glLineWidth(5.0);

	glBegin(GL_LINES);
	glVertex3d(startPt[0], startPt[1], startPt[2]);
	glVertex3d(endRight[0], endRight[1], endRight[2]);
	glEnd();

	// draw right point
	glPointSize(10);
	glColor4d(red.redF(), red.greenF(), red.blueF(), red.alphaF());
	glBegin(GL_POINTS);
	glVertex3d(endRight[0], endRight[1], endRight[2]);
	glEnd();

	// draw up dir as blue
	glColor4d(blue.redF(), blue.greenF(), blue.blueF(), blue.alphaF());
	glLineWidth(5.0);

	glBegin(GL_LINES);
	glVertex3d(startPt[0], startPt[1], startPt[2]);
	glVertex3d(endUp[0], endUp[1], endUp[2]);
	glEnd();

	// draw right point
	glPointSize(10);
	glColor4d(red.redF(), red.greenF(), red.blueF(), red.alphaF());
	glBegin(GL_POINTS);
	glVertex3d(endUp[0], endUp[1], endUp[2]);
	glEnd();


	glEnable(GL_LIGHTING);
}

double CModel::getOBBDiagLength()
{
	return m_OBB.GetDiagLength();
}

MathLib::Vector3 CModel::getFaceCenter(int fid)
{
	return m_mesh->getFaceCenter(fid);
}

MathLib::Vector3 CModel::getFaceNormal(int fid)
{
	return m_mesh->getFaceNormal(fid);
}

MathLib::Vector3 CModel::getModelPosOBB()
{  // get model bottom center

	MathLib::Vector3 bottomCenter;

	for (int i = 0; i < 3; i++)
	{
		if ((m_OBB.axis[i].dot(MathLib::Vector3(0, 0, 1))) > 0.99)
		{
			bottomCenter = m_OBB.cent - m_OBB.axis[i] * m_OBB.hsize[i];
			return bottomCenter;
		}		
		else if ((m_OBB.axis[i].dot(MathLib::Vector3(0, 0, 1))) < -0.99)
		{
			bottomCenter = m_OBB.cent + m_OBB.axis[i] * m_OBB.hsize[i];
			return bottomCenter;
		}
	}

	return m_OBB.GetFaceCent(4);
}

MathLib::Vector3 CModel::getModelTopCenter()
{
	MathLib::Vector3 topCenter;

	for (int i = 0; i < 3; i++)
	{
		if ((m_OBB.axis[i].dot(MathLib::Vector3(0, 0, 1))) > 0.99)
		{
			topCenter = m_OBB.cent + m_OBB.axis[i] * m_OBB.hsize[i];
			return topCenter;
		}
		else if ((m_OBB.axis[i].dot(MathLib::Vector3(0, 0, 1))) < -0.99)
		{
			topCenter = m_OBB.cent - m_OBB.axis[i] * m_OBB.hsize[i];
			return topCenter;
		}
	}

	return m_OBB.GetFaceCent(2);
}

MathLib::Vector3 CModel::getModelAlongDirOBBFaceCenter()
{
	MathLib::Vector3 center;

	MathLib::Vector3 frontDir = getFrontDir();
	for (int i = 0; i < 3; i++)
	{
		if (std::abs(m_OBB.axis[i].dot(frontDir)) < 0.1 && m_OBB.axis[i].cross(frontDir).dot(MathLib::Vector3(0, 0, 1)) < 0.9)
		{
			center = m_OBB.cent + m_OBB.axis[i] * m_OBB.hsize[i];
		}

		else
		{
			center = m_OBB.cent - m_OBB.axis[i] * m_OBB.hsize[i];
		}

		return center;
	}

	return center;
}

MathLib::Vector3 CModel::getModelNormalDirOBBFaceCenter()
{
	MathLib::Vector3 center;

	return center;
}

MathLib::Vector3 CModel::getModelRightCenter()
{
	return m_OBB.cent + m_OBB.axis[0] * m_OBB.hsize[0];
}

MathLib::Vector3 CModel::getModeLeftCenter()
{
	return m_OBB.cent - m_OBB.axis[0] * m_OBB.hsize[0];
}

double CModel::getOBBHeight()
{
	return m_OBB.GetHeight();
}

double CModel::getHorizonShortRange()
{
	int zIdx = -1;
	for (int i = 0; i < 3; i++)
	{
		if (std::abs(m_OBB.axis[i].dot(MathLib::Vector3(0, 0, 1)) > 0.9))
		{
			zIdx = i;
		}
	}

	double rangeVal = 1e6;
	for (int i=0; i <3; i++)
	{
		if (zIdx!=i)
		{
			if (m_OBB.size[i] < rangeVal)
			{
				rangeVal = m_OBB.size[i];
			}
		}
	}

	return rangeVal;
}

double CModel::getHorizonLongRange()
{
	int zIdx = -1;
	for (int i = 0; i < 3; i++)
	{
		if (std::abs(m_OBB.axis[i].dot(MathLib::Vector3(0, 0, 1)) > 0.95))
		{
			zIdx = i;
		}
	}

	double rangeVal = 0;
	for (int i = 0; i < 3; i++)
	{
		if (zIdx != i)
		{
			if (m_OBB.size[i] > rangeVal)
			{
				rangeVal = m_OBB.size[i];
			}
		}
	}

	return rangeVal;
}

MathLib::Vector3 CModel::getAlongDirOBBAxis()
{
	MathLib::Vector3 frontDir = getFrontDir();
	MathLib::Vector3 alongAxis;

	for (int i = 0; i < 3; i++)
	{
		if (std::abs(m_OBB.axis[i].dot(frontDir)) < 0.1 && m_OBB.axis[i].cross(frontDir).dot(MathLib::Vector3(0, 0, 1)) < -0.9)
		{
			alongAxis = m_OBB.axis[i];
			return alongAxis;
		}

		else if (std::abs(m_OBB.axis[i].dot(frontDir)) < 0.1 && m_OBB.axis[i].cross(frontDir).dot(MathLib::Vector3(0, 0, 1)) > 0.9)
		{
			alongAxis = - m_OBB.axis[i];
			return alongAxis;
		}
	}

	return alongAxis;
}

bool CModel::isSupportChild(int id)
{
	if (std::find(suppChindrenList.begin(), suppChindrenList.end(), id) != suppChindrenList.end())
	{
		return true;
	}

	else
	{
		return false;
	}
}

void CModel::updateFrontDir(const MathLib::Vector3 &loadedDir)
{
	m_initFrontDir = loadedDir;

	MathLib::Vector3 transedDir = m_fullTransMat.transformVec(loadedDir);
	transedDir.normalize();
	m_currFrontDir = transedDir;
}

void CModel::updateUpDir(const MathLib::Vector3 &loadedDir)
{
	m_initUpDir = loadedDir;

	MathLib::Vector3 transedDir = m_fullTransMat.transformVec(loadedDir);
	transedDir.normalize();
	m_currUpDir = transedDir;
}

MathLib::Vector3 CModel::getHorizonFrontDir()
{
	MathLib::Vector3 refHorizonFront;

	if (m_currFrontDir.dot(m_sceneUpVec) < 0.1)
	{
		refHorizonFront = m_currFrontDir;
	}
	else if (m_currFrontDir.dot(m_sceneUpVec) > 0.9)
	{
		refHorizonFront = -m_currUpDir; // suppose for bed
	}

	refHorizonFront.normalize();

	return refHorizonFront;
}

MathLib::Vector3 CModel::getVertUpDir()
{
	MathLib::Vector3 refVertUpDir;

	if (m_currUpDir.dot(m_sceneUpVec) > 0.9)
	{
		refVertUpDir = m_currUpDir;
	}
	else if (m_currUpDir.dot(m_sceneUpVec) < 0.1)
	{
		refVertUpDir = m_currFrontDir; // suppose for bed
	}

	refVertUpDir.normalize();
	return refVertUpDir;
}

MathLib::Vector3 CModel::getRightDir()
{
	MathLib::Vector3 rightDir = m_currFrontDir.cross(m_currUpDir);

	return rightDir;
}

SuppPlane* CModel::getSuppPlane(int i)
{
	return m_suppPlaneManager->getSuppPlane(i);
}

void CModel::computeBBAlignMat()
{
	MathLib::Vector3 transVec = -m_initAABB.cent;

	MathLib::Vector3 maxVert = m_initAABB.GetMaxV();
	MathLib::Vector3 minVert = m_initAABB.GetMinV();

	double scaleX = maxVert.x - minVert.x;
	double scaleY = maxVert.y - minVert.y;
	double scaleZ = maxVert.z - minVert.z;

	MathLib::Matrix4d translateMat;
	MathLib::Matrix4d rotMat;
	MathLib::Matrix4d scaleMat;

	translateMat.settranslate(transVec);
	scaleMat.setscale(1 / scaleX, 1 / scaleY, 1 / scaleZ);

	// rotate init front dir to word Y direction
	rotMat = GetRotMat(m_initFrontDir, MathLib::Vector3(0,1,0));

	// first bring model back to init frame, then transform init frame to unit box
	m_WorldBBToUnitBoxMat = rotMat*scaleMat*translateMat*m_initTransMat.invert();
}


//int CModel::getSuppPlaneNum()
//{
//	return m_suppPlaneManager->getSuppPlaneNum();
//}
//








