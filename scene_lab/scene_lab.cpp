#include "scene_lab.h"
#include "scene_lab_widget.h"
#include "modelDatabase.h"
#include "modelDBViewer_widget.h"
#include "RelationModelManager.h"
#include "../common/geometry/Scene.h"
#include "../t2scene/SceneSemGraph.h"
#include <set>

scene_lab::scene_lab(QObject *parent)
	: QObject(parent)
{
	m_widget = NULL;
	m_currScene = NULL;
	m_currSceneSemGraph = NULL;

	m_relationModelManager = NULL;

	m_modelDB = NULL; 
	m_modelDBViewer_widget = NULL;
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

void scene_lab::loadScene()
{
	if (m_currScene != NULL)
	{
		delete m_currScene;
	}

	CScene *scene = new CScene();
	scene->loadSceneFromFile(m_widget->loadSceneName(), 0, 0, 1);
	m_currScene = scene;

	loadModelMetaInfoForScene();

	emit sceneLoaded();
}

void scene_lab::loadSceneList()
{
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

	// load scene list file
	QString currPath = QDir::currentPath();
	QString sceneListFileName = currPath + "/scene_list.txt";
	QString sceneDBPath = "C:/Ruim/Graphics/T2S_MPC/SceneDB/StanfordSceneDB/scenes";

	QFile inFile(sceneListFileName);
	QTextStream ifs(&inFile);

	if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		std::cout << "SceneLab: cannot open scene list file.\n";
		return;
	}

	QString currLine = ifs.readLine();

	if (currLine.contains("StanfordSceneDatabase"))
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
			scene->loadSceneFromFile(filename, 1, 0, 0);

			m_sceneList.push_back(scene);
		}
	}
}

void scene_lab::loadModelMetaInfoForScene()
{
	if (m_modelDB == NULL)
	{
		m_modelDB = new ModelDatabase();
		m_modelDB->loadShapeNetSemTxt();
	}

	if (m_currScene!= NULL)
	{
		for (int i = 0; i < m_currScene->getModelNum(); i++)
		{
			QString modelNameString = m_currScene->getModelNameString(i);

			if (m_modelDB->dbMetaModels.count(modelNameString))
			{
				MathLib::Vector3 frontDir = m_modelDB->dbMetaModels[modelNameString]->frontDir;
				m_currScene->updateModelFrontDir(i, frontDir);

				MathLib::Vector3 upDir = m_modelDB->dbMetaModels[modelNameString]->upDir;
				m_currScene->updateModelUpDir(i, upDir);

				QString catName = m_modelDB->dbMetaModels[modelNameString]->getProcessedCatName();
				m_currScene->updateModelCat(i, catName);
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
	if (m_modelDB == NULL)
	{
		m_modelDB = new ModelDatabase();
		m_modelDB->loadShapeNetSemTxt();
	}

	m_modelDBViewer_widget = new ModelDBViewer_widget(m_modelDB);
	m_modelDBViewer_widget->show();
}

void scene_lab::destory_modelDBViewer_widget()
{

}

void scene_lab::buildSemGraphForCurrentScene(int batchLoading)
{
	if (m_modelDB == NULL)
	{
		m_modelDB = new ModelDatabase();
		m_modelDB->loadShapeNetSemTxt();
	}

	if (m_currScene == NULL)
	{
		Simple_Message_Box("No scene is loaded");
		return;
	}

	if (m_currSceneSemGraph!=NULL)
	{
		delete m_currSceneSemGraph;
	}

	m_currSceneSemGraph = new SceneSemGraph(m_currScene, m_modelDB);
	m_currSceneSemGraph->generateGraph();
	m_currSceneSemGraph->saveGraph();

	if (batchLoading == 0)
	{
		QString currPath = QDir::currentPath();
		QString mapFilename = currPath + "./SSGNodeLabelMap.txt";
		QString gmtAttriFilename = currPath + "./.gm_default_attributes";

		m_currSceneSemGraph->saveNodeStringToLabelIDMap(mapFilename);
		m_currSceneSemGraph->saveGMTNodeAttributeFile(gmtAttriFilename);
	}
}

void scene_lab::buildSemGraphForSceneList()
{
	loadSceneList();

	std::set<QString> allModelNameStrings;
	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];
		loadModelMetaInfoForScene();
		buildSemGraphForCurrentScene(1);

		for (int i = 0; i < m_currScene->getModelNum(); i++)
		{
			allModelNameStrings.insert(m_currScene->getModelNameString(i));
		}
	}

	//// load scene list file
	//QString currPath = QDir::currentPath();
	//QString sceneListFileName = currPath + "/scene_list.txt";
	//QString sceneDBPath = "C:/Ruim/Graphics/T2S_MPC/SceneDB/StanfordSceneDB/scenes";

	//QFile inFile(sceneListFileName);
	//QTextStream ifs(&inFile);

	//if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
	//{
	//	std::cout << "SceneLab: cannot open scene list file.\n";
	//	return;
	//}

	//std::set<QString> allModelNameStrings;

	//QString currLine = ifs.readLine();

	//if (currLine.contains("StanfordSceneDatabase"))
	//{
	//	int sceneNum = StringToIntegerList(currLine.toStdString(), "StanfordSceneDatabase ")[0];

	//	for (int i = 0; i < sceneNum; i++)
	//	{
	//		QString sceneName = ifs.readLine();

	//		if (m_currScene != NULL)
	//		{
	//			delete m_currScene;
	//		}

	//		CScene *scene = new CScene();
	//		QString filename = sceneDBPath + "/" + sceneName + ".txt";
	//		scene->loadSceneFromFile(filename, 1, 0, 0);
	//		m_currScene = scene;
	//		loadModelMetaInfoForScene();

	//		buildSemGraphForCurrentScene(1);

	//		for (int i = 0; i < m_currScene->getModelNum(); i++)
	//		{
	//			allModelNameStrings.insert(m_currScene->getModelNameString(i));
	//		}			
	//	}
	//}

	QString currPath = QDir::currentPath();
	QString correctedModelCatFileName = currPath + "/model_category_corrected.txt";

	QFile outFile(correctedModelCatFileName);
	QTextStream ofs(&outFile);

	if (!outFile.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
	{
		std::cout << "SceneLab: cannot open model category file.\n";
		return;
	}

	for (auto it = allModelNameStrings.begin(); it != allModelNameStrings.end(); it++)
	{
		if (m_modelDB->dbMetaModels.count(*it))
		{
			auto metaIt = m_modelDB->dbMetaModels.find(*it);
			DBMetaModel *m = metaIt->second;
			ofs << m->getIdStr()<<"," << m->getProcessedCatName() << "\n";
		}
	}

	outFile.close();

	std::cout << "\nSceneLab: model category saved.\n";
	std::cout << "\nSceneLab: all scene semantic graph generated.\n";
}

void scene_lab::buildRelationGraphForCurrentScene()
{
	m_currScene->buildRelationGraph();
}

void scene_lab::buildRelationGraphForSceneList()
{
	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];

		buildRelationGraphForCurrentScene()
	}

	//// load scene list file
	//QString currPath = QDir::currentPath();
	//QString sceneListFileName = currPath + "/scene_list.txt";
	//QString sceneDBPath = "C:/Ruim/Graphics/T2S_MPC/SceneDB/StanfordSceneDB/scenes";

	//QFile inFile(sceneListFileName);
	//QTextStream ifs(&inFile);

	//if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
	//{
	//	std::cout << "SceneLab: cannot open scene list file.\n";
	//	return;
	//}

	//QString currLine = ifs.readLine();

	//if (currLine.contains("StanfordSceneDatabase"))
	//{
	//	int sceneNum = StringToIntegerList(currLine.toStdString(), "StanfordSceneDatabase ")[0];

	//	for (int i = 0; i < sceneNum; i++)
	//	{
	//		QString sceneName = ifs.readLine();

	//		if (m_currScene != NULL)
	//		{
	//			delete m_currScene;
	//		}

	//		CScene *scene = new CScene();
	//		QString filename = sceneDBPath + "/" + sceneName + ".txt";
	//		scene->loadSceneFromFile(filename, 1, 0, 0);
	//		m_currScene = scene;

	//		m_currScene->buildRelationGraph();
	//	}
	//}

	std::cout << "\nSceneLab: all scene relation graph generated.\n";
}

void scene_lab::collectModelInfoForSceneList()
{

	std::set<QString> allModelNameStrings;

	// load scene list file
	QString currPath = QDir::currentPath();
	QString sceneListFileName = currPath + "/scene_list.txt";
	QString sceneDBPath = "C:/Ruim/Graphics/T2S_MPC/SceneDB/StanfordSceneDB/scenes";

	QFile inFile(sceneListFileName);
	QTextStream ifs(&inFile);

	if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		std::cout << "SceneLab: cannot open scene list file.\n";
		return;
	}

	QString currLine = ifs.readLine();

	if (currLine.contains("StanfordSceneDatabase"))
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
			scene->loadSceneFromFile(filename, 1, 0, 0);
			m_currScene = scene;
			
			for (int i = 0; i < m_currScene->getModelNum(); i++)
			{
				allModelNameStrings.insert(m_currScene->getModelNameString(i));
			}			
		}
	}

	QString modelMetaInfoFileName = currPath + "/scene_list_model_info.txt";

	QFile outFile(modelMetaInfoFileName);
	QTextStream ofs(&outFile);

	if (!outFile.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
	{
		std::cout << "SceneLab: cannot open model meta info file.\n";
		return;
	}

	if (m_modelDB == NULL)
	{
		m_modelDB = new ModelDatabase();
		m_modelDB->loadShapeNetSemTxt();
	}

	for (auto it = allModelNameStrings.begin(); it != allModelNameStrings.end(); it++)
	{
		if (m_modelDB->dbMetaModels.count(*it))
		{
			auto metaIt = m_modelDB->dbMetaModels.find(*it);
			DBMetaModel *m = metaIt->second;
			ofs << QString(m_modelDB->modelMetaInfoStrings[m->dbID].c_str()) << "\n";				
		}
	}

	outFile.close();

	std::cout << "\nSceneLab: model meta info saved.\n";
}

void scene_lab::buildRelationModels()
{
	loadSceneList();

	if (m_relationModelManager!=NULL)
	{
		delete m_relationModelManager;
	}

	m_relationModelManager = new RelationModelManager();


}


