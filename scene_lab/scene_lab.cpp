#include "scene_lab.h"
#include "scene_lab_widget.h"
#include "modelDatabase.h"
#include "modelDBViewer_widget.h"
#include "RelationModelManager.h"
#include "RelationExtractor.h"
#include "../common/geometry/Scene.h"
#include "../t2scene/SceneSemGraph.h"
#include <set>
#include "engine.h"
#include <stdio.h>

#include <QResource>

Engine *matlabEngine;

scene_lab::scene_lab(QObject *parent)
	: QObject(parent)
{
	m_widget = NULL;
	m_currScene = NULL;
	m_currSceneSemGraph = NULL;

	m_relationModelManager = NULL;
	m_relationExtractor = NULL;

	m_shapeNetModelDB = NULL; 
	m_modelDBViewer_widget = NULL;

	m_sunCGModelDB = NULL;

	loadParas();
}

scene_lab::~scene_lab()
{
	if (m_widget != NULL)
	{
		delete m_widget;
	}
}

void scene_lab::create_widget()
{
	m_widget = new scene_lab_widget(this);
	m_widget->show();
	m_widget->move(0, 200);

	std::cout << "SceneLab: widget created.\n";
}

void scene_lab::loadParas()
{
	QString fileName(":/paras.txt");
	QFile paraFile(fileName);

	if (!paraFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		std::cout << "Cannot open paras.txt\n";
	}

	QString currLine;
	int pos = 0;

	while (!paraFile.atEnd())
	{
		currLine = paraFile.readLine();

		if (currLine[0] != "#")
		{
			if ((pos = currLine.lastIndexOf("ProjectPath=")) != -1)
			{
				m_projectPath = currLine.replace("ProjectPath=", "");
				m_projectPath = currLine.replace("\n", "");
			}

			if ((pos = currLine.lastIndexOf("SceneDBType=")) != -1)
			{
				m_sceneDBType = currLine.replace("SceneDBType=", "");
				m_sceneDBType = currLine.replace("\n", "");
			}

			//if ((pos = currLine.lastIndexOf("SceneListFileName=")) != -1)
			//{
			//	m_sceneListFileName = currLine.replace("SceneListFileName=", "");
			//	m_sceneListFileName = currLine.replace("\n", "");
			//}

			if ((pos = currLine.lastIndexOf("LocalSceneDBPath=")) != -1)
			{
				m_localSceneDBPath = currLine.replace("LocalSceneDBPath=", "");
				m_localSceneDBPath = currLine.replace("\n", "");
			}

			if ((pos = currLine.lastIndexOf("AngleThreshold=")) != -1)
			{
				m_angleTh = currLine.replace("AngleThreshold=", "").toDouble();
			}
		}
	}
}

void scene_lab::loadSceneWithName(const QString &sceneFullName, int metaDataOnly, int obbOnly, int reComputeOBB, int updateModelCat)
{
	CScene *scene = new CScene();

	QFile sceneFile(sceneFullName);
	QFileInfo sceneFileInfo(sceneFile.fileName());

	QString sceneFormat = sceneFileInfo.suffix();

	if (sceneFormat == "txt")
	{
		// only load scene mesh
		scene->loadStanfordScene(sceneFullName, metaDataOnly, obbOnly, reComputeOBB);
		m_currScene = scene;

		if (m_shapeNetModelDB == NULL)
		{
			initShapeNetDB();
		}
		
		updateModelMetaInfoForScene(m_currScene);
	}
	else if (sceneFormat == "th")
	{
		scene->loadTsinghuaScene(sceneFullName, obbOnly, reComputeOBB);
		m_currScene = scene;

		if (m_modelCatMapTsinghua.empty())
		{
			initTsinghuaDB();
		}

		if (updateModelCat)
			updateModelCatForTsinghuaScene(m_currScene);
	}
	else if (sceneFormat == "json")
	{
		scene->loadJsonScene(sceneFullName, obbOnly, reComputeOBB);
		m_currScene = scene;

		if (m_sunCGModelDB == NULL)
		{
			initSunCGDB();
		}

		updateModelMetaInfoForScene(m_currScene);
	}

	if (m_relationExtractor == NULL)
	{
		m_relationExtractor = new RelationExtractor(m_angleTh);
	}

	emit sceneLoaded();
}

void scene_lab::LoadScene()
{
	if (m_currScene != NULL)
	{
		delete m_currScene;
	}

	QString sceneFullName = m_widget->loadSceneName();
	loadSceneWithName(sceneFullName, 0, 0, 0, 1);
}

void scene_lab::loadSceneDBList()
{
	QString sceneDBListFileName = m_projectPath + "/paras/scene_db_list.txt";

	QFile inFile(sceneDBListFileName);

	if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		std::cout << "SceneLab: cannot open scene db list file.\n";
		return;
	}

	QString currLine;
	m_loadedSceneFileNames.clear();

	while (!inFile.atEnd())
	{
		currLine = inFile.readLine();
		if (currLine[0]!="%")
		{
			QStringList parts = currLine.split(",");

			QString sceneListName = parts[1].replace("\n", "");
			loadSceneFileNamesFromListFile(m_loadedSceneFileNames, parts[0], sceneListName);
		}
	}

	inFile.close();
}

void scene_lab::loadSceneFileNamesFromListFile(std::map<QString, QStringList> &loadedSceneFileNames, const QString &sceneDBName, const QString &sceneListFileName)
{
	QString sceneListFileFullName = m_projectPath + QString("/paras/%1.txt").arg(sceneListFileName);

	QFile inFile(sceneListFileFullName);
	QTextStream ifs(&inFile);

	qDebug() << inFile.fileName();

	if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		std::cout << "SceneLab: cannot open scene list file.\n";
		return;
	}

	std::map<QString, QString> sceneFolder;
	sceneFolder[SceneFormat[DBTypeID::Stanford]] = m_localSceneDBPath + "/StanfordSceneDB/scenes";
	sceneFolder[SceneFormat[DBTypeID::SceneNN]] = m_localSceneDBPath + "/StanfordSceneDB/scenes";
	sceneFolder[SceneFormat[DBTypeID::Tsinghua]] = m_localSceneDBPath + "/TsinghuaSceneDB/scenes";
	sceneFolder[SceneFormat[DBTypeID::SunCG]] = m_localSceneDBPath + "/suncg_data/house/";

	QString currLine = ifs.readLine();

	if (currLine.contains("SceneNum"))
	{
		int sceneNum = StringToIntegerList(currLine.toStdString(), "SceneNum ")[0];

		QStringList currSceneNames;

		for (int i = 0; i < sceneNum; i++)
		{
			QString sceneName = ifs.readLine();
			QString filename;

			if (sceneDBName == SceneFormat[DBTypeID::Stanford] || sceneDBName == SceneFormat[DBTypeID::SceneNN])
			{
				filename = sceneFolder[sceneDBName] + "/" + sceneName + ".txt";
			}

			if (sceneDBName == SceneFormat[DBTypeID::Tsinghua])
			{
				filename = sceneFolder[sceneDBName] + "/" + sceneName + ".th";
			}

			if (sceneDBName == SceneFormat[DBTypeID::SunCG])
			{
				filename = sceneFolder[sceneDBName] + "/" + sceneName + "/house.json";
			}

			currSceneNames.push_back(filename);
		}

		loadedSceneFileNames[sceneDBName] = currSceneNames;
	}
}

// load the whole scene list into memory
void scene_lab::LoadWholeSceneList(int metaDataOnly, int obbOnly, int reComputeOBB, int updateModelCat)
{
	if (m_relationExtractor == NULL)
	{
		m_relationExtractor = new RelationExtractor(m_angleTh);
	}

	if (!m_sceneList.empty())
	{
		for (int i = 0; i < m_sceneList.size(); i++)
		{
			if (m_sceneList[i]!=NULL)
			{
				delete m_sceneList[i];
			}
		}
	}

	m_sceneList.clear();

	InitModelDBs();
	loadSceneDBList();

	for (auto it = m_loadedSceneFileNames.begin(); it!= m_loadedSceneFileNames.end(); it++)
	{
		QStringList& sceneFullNames = it->second;
		foreach(QString sceneName, sceneFullNames)
		{
			loadSceneWithName(sceneName, metaDataOnly, obbOnly, reComputeOBB, updateModelCat);
			m_sceneList.push_back(m_currScene);
		}
	}

	//
	//// load stanford or scenenn scenes
	//for (int i = 0; i < m_loadedSceneFileNames[0].size(); i++)
	//{
	//	CScene *scene = new CScene();
	//	scene->loadStanfordScene(m_loadedSceneFileNames[0][i], metaDataOnly, obbOnly, meshAndOBB);

	//	updateModelMetaInfoForScene(scene);
	//	m_sceneList.push_back(scene);
	//}

	//// load tsinghua scenes
	//for (int i = 0; i < m_loadedSceneFileNames[1].size(); i++)
	//{
	//	CScene *scene = new CScene();
	//	scene->loadTsinghuaScene(m_loadedSceneFileNames[1][i], obbOnly);

	//	if (updateModelCat)
	//	{
	//		updateModelCatForTsinghuaScene(scene);
	//	}

	//	m_sceneList.push_back(scene);
	//}
}

void scene_lab::InitModelDBs()
{
	if (m_sceneDBType == "stanford" || m_sceneDBType == "scenenn")
	{
		if (m_shapeNetModelDB == NULL)
		{
			initShapeNetDB();
		}
	}
	else if (m_sceneDBType == "tsinghua")
	{
		if (m_modelCatMapTsinghua.empty())
		{
			initTsinghuaDB();
		}
	}
	else if (m_sceneDBType == "suncg")
	{
		if (m_sunCGModelDB == NULL)
		{
			initSunCGDB();
		}
	}
	else
	{

	}
}

void scene_lab::initShapeNetDB()
{
	m_shapeNetModelDB = new ModelDatabase(m_projectPath, ModelDBType::ShapeNetDB);

	m_shapeNetModelDB->loadSpecifiedCatMap();
	m_shapeNetModelDB->loadShapeNetSemTxt();
}

void scene_lab::initTsinghuaDB()
{
	loadModelCatsMapTsinghua();
}

void scene_lab::initSunCGDB()
{
	m_sunCGModelDB = new ModelDatabase(m_projectPath, ModelDBType::SunCGDB);

	m_sunCGModelDB->loadSunCGMetaData();

	m_sunCGModelDB->loadSpecifiedCatMap();
	m_sunCGModelDB->loadSunCGModelCatMap();
	m_sunCGModelDB->loadSunCGModelCat();
}

void scene_lab::loadModelCatsMapTsinghua()
{
	//QString currPath = QDir::currentPath();
	//QString modelCatFileName = currPath + "/ModelCategoryMapTsinghua.txt";

	QString modelCatFileName = m_projectPath + "/meta_data/ModelCategoryMapTsinghua.txt";

	QFile modelCatFile(modelCatFileName);

	if (!modelCatFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		std::cout << "Cannot open model category map file\n";
	}

	QTextStream ifs(&modelCatFile);

	while (!ifs.atEnd())
	{
		QString currLine = ifs.readLine();
		QStringList catNames = currLine.split(",");
	
		// save category if it does not exist in the map
		if (m_modelCatMapTsinghua.find(catNames[0]) == m_modelCatMapTsinghua.end())
		{
			m_modelCatMapTsinghua[catNames[0]] = catNames[1];
		}
	}

	modelCatFile.close();
}

void scene_lab::updateModelCatForTsinghuaScene(CScene *s)
{
	for (int i=0; i < s->getModelNum(); i++)
	{
		CModel *currModel = s->getModel(i);
		QString currModelCat = currModel->getCatName();

		if (m_modelCatMapTsinghua.count(currModelCat))
		{
			currModel->setCatName(m_modelCatMapTsinghua[currModelCat]);
		}
	}
}

void scene_lab::ExtractModelCatsFromSceneList()
{
	// legacy code

	loadParas();
	LoadWholeSceneList(1, 1, 0, 0);  // load scenes without updating model cats

	int stanfordSceneNum, tsinghuaSceneNum;
	//int stanfordSceneNum = m_loadedSceneFileNames[0].size();
	//int tsinghuaSceneNum = m_loadedSceneFileNames[1].size();

	// collect model categories for stanford scenes

	std::set<QString> allModelNameStrings;

	//for (int i = 0; i < m_sceneList.size(); i++)
	for (int i = 0; i < stanfordSceneNum; i++)
	{
		for (int j = 0; j <  m_sceneList[i]->getModelNum(); j++)
		{
			allModelNameStrings.insert(m_sceneList[i]->getModelNameString(j));
		}
	}

	if (!allModelNameStrings.empty())
	{
		// collect corrected model category
		//QString currPath = QDir::currentPath();
		//QString correctedModelCatFileName = currPath + "/stanford_NameCatMap_corrected.txt";

		QString correctedModelCatFileName = m_projectPath + "/meta_data/stanford_NameCatMap_corrected.txt";

		QFile outFile(correctedModelCatFileName);
		QTextStream ofs(&outFile);

		if (!outFile.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
		{
			std::cout << "SceneLab: cannot open model category name map file.\n";
			return;
		}

		std::set<QString> allModelCats;
		for (auto it = allModelNameStrings.begin(); it != allModelNameStrings.end(); it++)
		{
			if (m_shapeNetModelDB != NULL && m_shapeNetModelDB->dbMetaModels.count(*it))
			{
				auto metaIt = m_shapeNetModelDB->dbMetaModels.find(*it);
				DBMetaModel *m = metaIt->second;
				QString modelCat = m->getProcessedCatName();
				allModelCats.insert(modelCat);

				ofs << m->getIdStr() << "," << modelCat << "\n";
			}
		}

		outFile.close();

		//QString modelCatFileName = currPath + "/stanford_model_category_corrected.txt";

		QString modelCatFileName = m_projectPath + "/meta_data/stanford_model_category_corrected.txt";
		outFile.setFileName(modelCatFileName);

		if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			std::cout << "SceneLab: cannot open model category file.\n"; 
			return;
		}

		for (auto it = allModelCats.begin(); it != allModelCats.end(); it++)
		{
			ofs << *it << "\n";
		}

		outFile.close();
	}

	// collect model categories for tsinghua scenes
	std::set<QString> allModelCats;
	for (int i = stanfordSceneNum; i < stanfordSceneNum + tsinghuaSceneNum; i++)
	{
		for (int j = 0; j < m_sceneList[i]->getModelNum(); j++)
		{
			allModelCats.insert(m_sceneList[i]->getModelCatName(j));  // directly get model category name for tsinghua scenes
		}
	}

	if (!allModelCats.empty())
	{
		//QString currPath = QDir::currentPath();
		//QString modelCatFileName = currPath + "/tsinghua_model_category.txt";
		QString modelCatFileName = m_projectPath + "/meta_data/tsinghua_model_category.txt";

		QFile outFile(modelCatFileName);
		QTextStream ofs(&outFile);

		if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			std::cout << "\tCannot open file " << modelCatFileName.toStdString() << std::endl;
		}

		for (auto it = allModelCats.begin(); it != allModelCats.end(); it++)
		{
			ofs << *it << "\n";
		}

		outFile.close();
	}

	std::cout << "\nSceneLab: model category saved.\n";
}

// update cat name, front dir, up dir for model
void scene_lab::updateModelMetaInfoForScene(CScene *s)
{
	if (s == NULL) return;

	QString sceneFormat = s->getSceneFormat();
	if (sceneFormat == SceneFormat[DBTypeID::Stanford] || sceneFormat == SceneFormat[DBTypeID::SceneNN])
	{
		for (int i = 0; i < s->getModelNum(); i++)
		{
			QString modelNameString = s->getModelNameString(i);

			if (m_shapeNetModelDB->dbMetaModels.count(modelNameString))
			{
				MathLib::Vector3 frontDir = m_shapeNetModelDB->dbMetaModels[modelNameString]->frontDir;
				s->updateModelFrontDir(i, frontDir);

				MathLib::Vector3 upDir = m_shapeNetModelDB->dbMetaModels[modelNameString]->upDir;
				s->updateModelUpDir(i, upDir); // actually, no need to update up dir as it is already be rotated to (0,0,1) ?

				QString catName = m_shapeNetModelDB->dbMetaModels[modelNameString]->getProcessedCatName();
				s->updateModelCat(i, catName);
			}

			if (modelNameString.contains("room"))
			{
				s->updateModelCat(i, "room");
			}
		}
	}

	if (sceneFormat == SceneFormat[DBTypeID::SunCG])
	{
		for (int i = 0; i < s->getModelNum(); i++)
		{
			QString modelNameString = s->getModelNameString(i);

			if (m_sunCGModelDB->dbMetaModels.count(modelNameString))
			{
				// update front dir based on annotation
				// no need to update up dir as it is already be rotated to (0,0,1)
				MathLib::Vector3 frontDir = m_sunCGModelDB->dbMetaModels[modelNameString]->frontDir;
				s->updateModelFrontDir(i, frontDir);  

				QString catName = m_sunCGModelDB->dbMetaModels[modelNameString]->getCatName();
				s->updateModelCat(i, catName);
			}
		}
	}
}

void scene_lab::BuildOBBForSceneList()
{
	loadSceneDBList();

	for (auto it = m_loadedSceneFileNames.begin(); it != m_loadedSceneFileNames.end(); it++)
	{
		QStringList& sceneFullNames = it->second;
		foreach(QString sceneName, sceneFullNames)
		{
			if (m_currScene != NULL)
			{
				delete m_currScene;
			}

			loadSceneWithName(sceneName, 0, 0, 1, 0);

			for (int i = 0; i < m_currScene->getModelNum(); i++)
			{
				CModel *m = m_currScene->getModel(i);

				// re-compute model OBB if it is skewed when placing into current scene
				if (m->m_OBBSkewed)
				{
					if (m_currScene->getUprightVec() == MathLib::Vector3(0,0,1))
					{
						m->computeOBB(2);
					}
					else
					{
						m->computeOBB(1);
					}
				}
			}
		}
	}
}

void scene_lab::destroy_widget()
{
	if (m_widget != NULL)
	{
		delete m_widget;
	}
}

void scene_lab::setDrawArea(Starlab::DrawArea *drawArea)
{
	m_drawArea = drawArea;
}

void scene_lab::updateSceneRenderingOptions()
{
	bool showModelOBB = m_widget->ui->showOBBCheckBox->isChecked();
	bool showSuppGraph = m_widget->ui->showGraphCheckBox->isChecked();
	bool showFrontDir = m_widget->ui->showFrontDirCheckBox->isChecked();
	bool showSuppPlane = m_widget->ui->showSuppPlaneCheckBox->isChecked();
	
	if (m_currScene != NULL)
	{
		m_currScene->setShowModelOBB(showModelOBB);
		m_currScene->setShowSceneGraph(showSuppGraph);
		m_currScene->setShowSuppPlane(showSuppPlane);
		m_currScene->setShowModelFrontDir(showFrontDir);

	}

	if (m_modelDBViewer_widget !=NULL)
	{
		m_modelDBViewer_widget->updateRenderingOptions(showModelOBB, showFrontDir, showSuppPlane);
	}

	emit sceneRenderingUpdated();
}

void scene_lab::create_modelDBViewer_widget()
{
	if (m_shapeNetModelDB == NULL)
	{
		initShapeNetDB();
	}

	m_modelDBViewer_widget = new ModelDBViewer_widget(m_shapeNetModelDB);
	m_modelDBViewer_widget->show();
}

void scene_lab::destory_modelDBViewer_widget()
{

}

#define BUFSIZE 1000
void scene_lab::testMatlab()
{
	// 1. Open MATLAB engine
	matlabEngine = engOpen(NULL);
	if (matlabEngine == NULL) {
		printf("Can't start MATLAB engine!!!\n");
		exit(-1);
	}

	// 2. Call MATLAB engine
	{
		// 2.1. Pre-work: capture MATLAB output. Ensure the buffer is always NULL terminated.
		char buffer[BUFSIZE + 1];
		buffer[BUFSIZE] = '\0';
		engOutputBuffer(matlabEngine, buffer, BUFSIZE);
		engEvalString(matlabEngine, "clc;clear;"); // clear all variables (optional)

											 // 2.2. Setup inputs: a, b
		mxArray *a = mxCreateDoubleScalar(2); // assume a=2
		mxArray *b = mxCreateDoubleScalar(3); // assume b=3
		engPutVariable(matlabEngine, "a", a); // put into matlab
		engPutVariable(matlabEngine, "b", b); // put into matlab

										// 2.3. Call MATLAB
		engEvalString(matlabEngine, "cd \'C:\\Ruim\\Graphics\\T2S_MPC\\text2scene-learning\\scene_lab'");
		engEvalString(matlabEngine, "[y, z] = myadd2(a, b);");
		printf("%s\n", buffer); // get error messages or prints (optional)

							 // 2.4. Get result: y, z
		mxArray *y = engGetVariable(matlabEngine, "y");
		mxArray *z = engGetVariable(matlabEngine, "z");
		double y_res = mxGetScalar(y);
		double z_res = mxGetScalar(z);
		printf("y=%f\nz=%f\n", y_res, z_res);

		// 2.5. Release (to all mxArray)
		mxDestroyArray(a);
		mxDestroyArray(b);
		mxDestroyArray(y);
		mxDestroyArray(z);
	}

	// 3. Close MATLAB engine
	engClose(matlabEngine);
}

void scene_lab::ScreenShotForCurrScene()
{
	if (m_currScene != NULL)
	{
		// reset camera
		m_drawArea->camera()->setOrientation(0, -MathLib::ML_PI_2);
		m_drawArea->camera()->setViewDirection(qglviewer::Vec(-1, 1, -1));
		m_drawArea->camera()->showEntireScene();
		m_drawArea->updateGL();

		QString sceneFilePath = m_currScene->getFilePath();
		QString sceneName = m_currScene->getSceneName();
		m_drawArea->saveSnapshot(sceneFilePath + "/" + sceneName + ".png");
		std::cout << "Screenshot for " << sceneName.toStdString() << " saved\n";
	}
}

void scene_lab::ScreenShotForSceneList()
{
	loadParas();
	LoadWholeSceneList(0,0,0);

	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];

		emit sceneLoaded();
		ScreenShotForCurrScene();
	}
}

void scene_lab::BuildSemGraphForCurrentScene()
{
	if (m_currScene == NULL)
	{
		Simple_Message_Box("No scene is loaded");
		return;
	}

	if (m_currSceneSemGraph!=NULL)
	{
		delete m_currSceneSemGraph;
	}

	m_currSceneSemGraph = new SceneSemGraph(m_currScene, m_shapeNetModelDB, m_relationExtractor);
	m_currSceneSemGraph->generateGraph();
	m_currSceneSemGraph->saveGraph();

}

void scene_lab::BuildSemGraphForSceneList()
{
	loadParas();

	// only load meta data (stanford and scenenn) and obb (tsinghua)
	LoadWholeSceneList(1,1,0);

	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];
		BuildSemGraphForCurrentScene();
	}

	std::cout << "\nSceneLab: all scene semantic graph generated.\n";
}

void scene_lab::BuildRelationGraphForCurrentScene()
{
	if (m_currScene == NULL)
	{
		Simple_Message_Box("No scene is loaded");
		return;
	}

	m_currScene->buildRelationGraph();
}

void scene_lab::BuildRelationGraphForSceneList()
{
	loadParas();
	loadSceneDBList();

	for (auto it = m_loadedSceneFileNames.begin(); it != m_loadedSceneFileNames.end(); it++)
	{
		QStringList& sceneFullNames = it->second;
		foreach(QString sceneName, sceneFullNames)
		{
			if (m_currScene != NULL)
			{
				delete m_currScene;
			}

			loadSceneWithName(sceneName, 0, 0, 0, 0);
			BuildRelationGraphForCurrentScene();
		}
	}

	//if (m_sceneDBType == "stanford")
	//{
	//	LoadWholeSceneList(0, 1);
	//}
	//else
	//{
	//	LoadWholeSceneList(0, 0, 1);
	//}

	//for (int i = 0; i < m_sceneList.size(); i++)
	//{
	//	m_currScene = m_sceneList[i];

	//	BuildRelationGraphForCurrentScene();
	//}

	std::cout << "\nSceneLab: all scene relation graph generated.\n";
}

void scene_lab::ExtractMetaFileForSceneList()
{
	// collect model meta info for current scene list. (subset of the whole shapenetsem meta file)

	std::set<QString> allModelNameStrings;

	// load scene list file
	//QString currPath = QDir::currentPath();
	//QString sceneListFileName = currPath + "/scene_list.txt";

	QString sceneListFileName = m_projectPath + "/paras/scene_list.txt";

	//QString sceneDBPath = "C:/Ruim/Graphics/T2S_MPC/SceneDB/StanfordSceneDB/scenes";
	QString sceneDBPath = m_localSceneDBPath + "/StanfordSceneDB/scenes";

	QFile inFile(sceneListFileName);
	QTextStream ifs(&inFile);

	if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		std::cout << "SceneLab: cannot open scene list file.\n";
		return;
	}

	QString currLine = ifs.readLine();

	if (currLine.contains("SceneNum"))
	{
		int sceneNum = StringToIntegerList(currLine.toStdString(), "StanfordSceneDatabase ")[0];

		for (int i = 0; i < sceneNum; i++)
		{
			QString sceneName = ifs.readLine();

			if (m_currScene != NULL)
			{
				delete m_currScene;
			}

			CScene *scene = new CScene();
			QString filename = sceneDBPath + "/" + sceneName + ".txt";
			scene->loadStanfordScene(filename, 1, 0, 0);
			m_currScene = scene;
			
			for (int mi = 0; mi < m_currScene->getModelNum(); mi++)
			{
				allModelNameStrings.insert(m_currScene->getModelNameString(mi));
			}			
		}
	}

	//QString modelMetaInfoFileName = currPath + "/scene_list_model_info.txt";
	QString modelMetaInfoFileName = m_localSceneDBPath + "/meta_data/scene_list_model_info.txt";

	QFile outFile(modelMetaInfoFileName);
	QTextStream ofs(&outFile);

	if (!outFile.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
	{
		std::cout << "SceneLab: cannot open model meta info file.\n";
		return;
	}

	if (m_shapeNetModelDB == NULL)
	{
		initShapeNetDB();
	}

	for (auto it = allModelNameStrings.begin(); it != allModelNameStrings.end(); it++)
	{
		if (m_shapeNetModelDB->dbMetaModels.count(*it))
		{
			auto metaIt = m_shapeNetModelDB->dbMetaModels.find(*it);
			DBMetaModel *m = metaIt->second;
			ofs << QString(m_shapeNetModelDB->modelMetaInfoStrings[m->dbID].c_str()) << "\n";				
		}
	}

	outFile.close();

	std::cout << "\nSceneLab: model meta info saved.\n";
}

void scene_lab::BuildRelativeRelationModels()
{
	//testMatlab();

	loadParas();
	// load metadata
	LoadWholeSceneList(1);

	if (m_relationModelManager!=NULL)
	{
		delete m_relationModelManager;
	}

	m_relationModelManager = new RelationModelManager(m_relationExtractor);

	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];
		m_relationModelManager->updateCurrScene(m_currScene);
		m_relationModelManager->loadRelativePosFromCurrScene();
	}

	m_relationModelManager->buildRelativeRelationModels();
	m_relationModelManager->saveRelativeRelationModels(m_localSceneDBPath, m_sceneDBType);
}

void scene_lab::BuildPairwiseRelationModels()
{
	loadParas();
	LoadWholeSceneList(1);

	if (m_relationModelManager != NULL)
	{
		delete m_relationModelManager;
	}

	m_relationModelManager = new RelationModelManager(m_relationExtractor);

	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];
		m_currScene->loadSSG();

		m_relationModelManager->updateCurrScene(m_currScene);
		m_relationModelManager->loadRelativePosFromCurrScene();
		m_relationModelManager->collectPairwiseInstanceFromCurrScene();
	}

	m_relationModelManager->buildPairwiseRelationModels();
	m_relationModelManager->computeSimForPairwiseModels(m_relationModelManager->m_pairwiseRelModels, m_relationModelManager->m_pairRelModelKeys, m_sceneList, false, m_localSceneDBPath);

	m_relationModelManager->savePairwiseRelationModels(m_localSceneDBPath, m_sceneDBType);
	m_relationModelManager->savePairwiseModelSim(m_localSceneDBPath, m_sceneDBType);
}

void scene_lab::BuildGroupRelationModels()
{
	loadParas();
	LoadWholeSceneList(1);

	if (m_relationModelManager != NULL)
	{
		delete m_relationModelManager;
	}

	m_relationModelManager = new RelationModelManager(m_relationExtractor);

	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];
		m_currScene->loadSSG();

		m_relationModelManager->updateCurrScene(m_currScene);
		m_relationModelManager->loadRelativePosFromCurrScene();
		m_relationModelManager->collectGroupInstanceFromCurrScene();
	}

	m_relationModelManager->buildGroupRelationModels();
	m_relationModelManager->computeSimForPairModelInGroup(m_sceneList);

	m_relationModelManager->saveGroupRelationModels(m_localSceneDBPath, m_sceneDBType);
	m_relationModelManager->saveGroupModelSim(m_localSceneDBPath, m_sceneDBType);

	m_relationModelManager->saveCoOccurInGroupModels(m_localSceneDBPath, m_sceneDBType);
}

void scene_lab::BatchBuildModelsForList()
{
	uint64 startTime = GetTimeMs64();

	loadParas();
	// load metadata
	LoadWholeSceneList(1);

	if (m_relationModelManager != NULL)
	{
		delete m_relationModelManager;
	}

	m_relationModelManager = new RelationModelManager(m_relationExtractor);

	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];
		m_currScene->loadSSG();

		m_relationModelManager->updateCurrScene(m_currScene);
		m_relationModelManager->loadRelativePosFromCurrScene();

		m_relationModelManager->collectPairwiseInstanceFromCurrScene();
		m_relationModelManager->collectGroupInstanceFromCurrScene();
		m_relationModelManager->collectSupportRelationInCurrentScene();
	}

	// relative
	m_relationModelManager->buildRelativeRelationModels();
	m_relationModelManager->saveRelativeRelationModels(m_localSceneDBPath, m_sceneDBType);

	// pairwise
	m_relationModelManager->buildPairwiseRelationModels();
	m_relationModelManager->computeSimForPairwiseModels(m_relationModelManager->m_pairwiseRelModels, m_relationModelManager->m_pairRelModelKeys, m_sceneList, false, m_localSceneDBPath);

	m_relationModelManager->savePairwiseRelationModels(m_localSceneDBPath, m_sceneDBType);
	m_relationModelManager->savePairwiseModelSim(m_localSceneDBPath, m_sceneDBType);

	// group
	m_relationModelManager->buildGroupRelationModels();
	m_relationModelManager->computeSimForPairModelInGroup(m_sceneList);

	m_relationModelManager->saveGroupRelationModels(m_localSceneDBPath, m_sceneDBType);
	m_relationModelManager->saveGroupModelSim(m_localSceneDBPath, m_sceneDBType);

	// support
	m_relationModelManager->buildSupportRelationModels();
	m_relationModelManager->saveSupportRelationModels(m_localSceneDBPath, m_sceneDBType);

	uint64 endTime = GetTimeMs64();
	qDebug() << QString("Done in %1 seconds").arg((endTime-startTime)/1000);
}

void scene_lab::ComputeBBAlignMatForSceneList()
{
	loadParas();

	// load mesh without OBB
	LoadWholeSceneList(0, 0, 0);

	for (int i = 0; i < m_sceneList.size(); i++)
	{
		// update front dir for alignment
		m_currScene = m_sceneList[i];
		m_currScene->computeModelBBAlignMat();

		qDebug() << "SceneLab: bounding box alignment matrix saved for " << m_currScene->getSceneName();
	}
}

void scene_lab::ExtractRelPosForSceneList()
{
	loadParas();

	// load OBB only
	LoadWholeSceneList(0, 1, 0);

	if (m_relationModelManager != NULL)
	{
		delete m_relationModelManager;
	}

	m_relationModelManager = new RelationModelManager(m_relationExtractor);

	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];
		m_relationModelManager->updateCurrScene(m_currScene);

		m_relationModelManager->collectRelativePosInCurrScene();

		qDebug() << "SceneLab: relative position saved for " << m_currScene->getSceneName();
	}
}

void scene_lab::ExtractSuppProbForSceneList()
{
	loadParas();
	LoadWholeSceneList(1);

	if (m_relationModelManager != NULL)
	{
		delete m_relationModelManager;
	}

	m_relationModelManager = new RelationModelManager(m_relationExtractor);

	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];
		m_currScene->loadSSG();

		m_relationModelManager->updateCurrScene(m_currScene);
		m_relationModelManager->collectSupportRelationInCurrentScene();

		m_relationModelManager->collectCoOccInCurrentScene();
	}

	m_relationModelManager->buildSupportRelationModels();
	m_relationModelManager->saveSupportRelationModels(m_localSceneDBPath, m_sceneDBType);

	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];
		m_relationModelManager->updateCurrScene(m_currScene);
		m_relationModelManager->collectSupportRelationInCurrentScene();

		m_relationModelManager->addOccToCoOccFromCurrentScene();
	}

	m_relationModelManager->computeOccToCoccOnSameParent();
	m_relationModelManager->saveCoOccurOnParentModels(m_localSceneDBPath, m_sceneDBType);
}

