#include "RelationModelManager.h"
#include "RelationExtractor.h"
#include "../common/geometry/Scene.h"
#include "../common/geometry/CModel.h"

#include "engine.h"
extern Engine *matlabEngine;

RelationModelManager::RelationModelManager(RelationExtractor *mExtractor)
	:m_relationExtractor(mExtractor)
{

}

RelationModelManager::~RelationModelManager()
{
	for (int i=0; i < m_relativePostions.size(); i++)
	{
		delete m_relativePostions[i];
	}
}

void RelationModelManager::collectRelativePosInCurrScene()
{
	int modelNum = m_currScene->getModelNum();

	if (!m_currScene->loadModelBBAlignMat())
	{
		qDebug() << "RelationModelManager: compute model align mat first for "<< m_currScene->getSceneName();
		return;
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

			RelativePos *relPos = new RelativePos();
			relPos->m_conditionName = conditionName;

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

void RelationModelManager::addRelativePosFromCurrScene()
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
		RelativePos *relPos = new RelativePos();
		relPos->m_anchorObjName = toQString(parts[0]);
		relPos->m_actObjName = toQString(parts[1]);
		relPos->m_conditionName = toQString(parts[2]);

		currLine = ifs.readLine();
		parts = PartitionString(currLine.toStdString(), ",");

		std::vector<float> pos = StringToFloatList(parts[0], "");
		relPos->pos = MathLib::Vector3(pos[0], pos[1], pos[2]);
		relPos->theta = pos[3];

		std::vector<float> transformVec = StringToFloatList(parts[1], "");
		relPos->anchorAlignMat = MathLib::Matrix4d(transformVec);

		transformVec = StringToFloatList(parts[2], "");
		relPos->actAlignMat = MathLib::Matrix4d(transformVec);

		m_relativePostions.push_back(relPos);
	}

	inFile.close();

	qDebug() << "RelationModelManager: loaded relative position for scene " << m_currScene->getSceneName();
}

void RelationModelManager::buildRelationModels()
{
	// collect instance ids for relative models
	for (int i = 0; i < m_relativePostions.size(); i++)
	{
		RelativePos *relPos = m_relativePostions[i];
		QString mapKey = relPos->m_anchorObjName + relPos->m_actObjName + relPos->m_conditionName;

		if (!m_relativeModels.count(mapKey))
		{
			PairwiseRelationModel *relativeModel = new PairwiseRelationModel(relPos->m_anchorObjName, relPos->m_actObjName, relPos->m_conditionName);
			
			relativeModel->m_instances.push_back(relPos);
			m_relativeModels[mapKey] = relativeModel;
		}
		else
			m_relativeModels[mapKey]->m_instances.push_back(relPos);

		m_relativeModels[mapKey]->m_numInstance = m_relativeModels[mapKey]->m_instances.size();
	}

	// 1. Open MATLAB engine
	matlabEngine = engOpen(NULL);

	// fit GMM model
	for (auto iter = m_relativeModels.begin(); iter != m_relativeModels.end(); iter++)
	{
		PairwiseRelationModel *relModel = iter->second;
		relModel->fitGMM();
	}

	// 3. Close MATLAB engine
	engClose(matlabEngine);
}

void RelationModelManager::saveRelationModels(const QString &filePath)
{
	QString filename = filePath + "/Relative.model";

	QFile outFile(filename);
	QTextStream ofs(&outFile);

	if (outFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
	{
		// save relative relations
		for (auto iter = m_relativeModels.begin(); iter != m_relativeModels.end(); iter++)
		{
			PairwiseRelationModel *relModel = iter->second;
			ofs << relModel->m_anchorObjName << "," << relModel->m_actObjName << "," << relModel->m_conditionName << "\n";
			ofs << relModel->m_numGauss<< " "<< relModel->m_numInstance <<"\n";

			if (relModel->m_numGauss > 0 )
			{
				for (int i = 0; i < relModel->m_numGauss; i++)
				{
					GaussianModel & currGauss = relModel->m_gaussians[i];
					ofs << currGauss.weight << ",";
					ofs << currGauss.mean(0) << " " << currGauss.mean(1) << " " << currGauss.mean(2) << " " << currGauss.mean(3) << ",";
					ofs << GetTransformationString(currGauss.covarMat) << "\n";
				}
			}

			else
			{
				for (int i=0; i < relModel->m_numInstance; i++)
				{
					RelativePos *relPos = relModel->m_instances[i];
					if (i < relModel->m_numInstance -1)
					{
						ofs << relPos->pos.x << " " << relPos->pos.y << " " << relPos->pos.z << " " << relPos->theta << ",";
					}
					else
						ofs << relPos->pos.x << " " << relPos->pos.y << " " << relPos->pos.z << " " << relPos->theta << "\n";					
				}
			}
		}

		outFile.close();
	}
}
