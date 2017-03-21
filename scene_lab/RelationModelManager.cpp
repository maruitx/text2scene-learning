#include "RelationModelManager.h"
#include "RelationExtractor.h"
#include "../common/geometry/Scene.h"
#include "../common/geometry/CModel.h"



RelationModelManager::RelationModelManager(RelationExtractor *mExtractor)
	:m_relationExtractor(mExtractor)
{

}


RelationModelManager::~RelationModelManager()
{

}

void RelationModelManager::collectRelativePosInCurrScene()
{
	int modelNum = m_currScene->getModelNum();

	if (!m_currScene->loadModelBBAlignMat())
	{
		qDebug() << "RelationModelManager: compute model align mat first for "<< m_currScene->getSceneName();
	}

	for (int i = 0; i < modelNum; i++)
	{
		CModel *anchorModel = m_currScene->getModel(i);
		for (int j = 0; j < modelNum; j++)
		{
			if (i==j) continue;
			
			CModel *actModel = m_currScene->getModel(j);
			QString conditionName = m_relationExtractor->getRelationConditionType(anchorModel, actModel); 

			if (conditionName == "none") continue;

			RelativePos relPos;
			relPos.conditionName = conditionName;

			m_relationExtractor->extractRelativePosForModelPair(anchorModel, actModel, relPos);

			m_currScene->m_relPositions.push_back(relPos);
		}
	}

	m_currScene->saveRelPositions();
}

void RelationModelManager::updateCurrScene(CScene *s)
{
	m_currScene = s;
	m_relationExtractor->updateCurrScene(m_currScene);
}

void RelationModelManager::addRelativePosForCurrScene()
{
	QString filename = m_currScene->getFilePath() + "/" + m_currScene->getSceneName() + ".relPos";

	QFile inFile(filename);
	QTextStream ifs(&inFile);

	if (!inFile.open(QIODevice::ReadOnly))
	{
		qDebug() << "RelationModelManager: cannot load relative position for scene " << m_currScene->getSceneName();
		return;
	}

	while (!ifs.atEnd())
	{
		QString currLine = ifs.readLine();
		std::vector<std::string> parts = PartitionString(currLine.toStdString(), ",");
		RelativePos relPos;
		relPos.anchorObjName = toQString(parts[0]);
		relPos.actObjName = toQString(parts[1]);
		relPos.conditionName = toQString(parts[2]);

		currLine = ifs.readLine();
		parts = PartitionString(currLine.toStdString(), ",");
		std::vector<float> pos = StringToFloatList(parts[0], "");
		relPos.pos = MathLib::Vector3(pos[0], pos[1], pos[2]);
		relPos.theta = pos[3]; 

		std::vector<float> transformVec = StringToFloatList(currLine.toStdString(), "");
		MathLib::Matrix4d transMat(transformVec);
		relPos.actAlignMat = transMat;

		m_relativePostions.push_back(relPos);
	}

	inFile.close();

	qDebug() << "RelationModelManager:loaded relative position for scene " << m_currScene->getSceneName();
}
