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

Engine *matlabEngine;

scene_lab::scene_lab(QObject *parent)
	: QObject(parent)
{
	m_widget = NULL;
	m_currScene = NULL;
	m_currSceneSemGraph = NULL;

	m_relationModelManager = NULL;
	m_relationExtractor = NULL;

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

	// only load scene mesh
	scene->loadSceneFromFile(m_widget->loadSceneName(), 0, 0, 1);
	m_currScene = scene;

	if (m_modelDB == NULL)
	{
		m_modelDB = new ModelDatabase();
		m_modelDB->loadShapeNetSemTxt();
	}

	if (m_relationExtractor == NULL)
	{
		m_relationExtractor = new RelationExtractor();
	}

	updateModelMetaInfoForScene(m_currScene);

	emit sceneLoaded();
}

void scene_lab::loadSceneList(int metaDataOnly, int obbOnly, int meshAndOBB)
{
	if (m_modelDB == NULL)
	{
		m_modelDB = new ModelDatabase();
		m_modelDB->loadShapeNetSemTxt();
	}

	if (m_relationExtractor == NULL)
	{
		m_relationExtractor = new RelationExtractor();
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

			CScene *scene = new CScene();
			QString filename = sceneDBPath + "/" + sceneName + ".txt";
			scene->loadSceneFromFile(filename, metaDataOnly, obbOnly, meshAndOBB);
			
			updateModelMetaInfoForScene(scene);
			m_sceneList.push_back(scene);
		}
	}
}

// update cat name, front dir, up dir for model
void scene_lab::updateModelMetaInfoForScene(CScene *s)
{
	if (s!= NULL)
	{
		for (int i = 0; i < s->getModelNum(); i++)
		{
			QString modelNameString = s->getModelNameString(i);

			if (m_modelDB->dbMetaModels.count(modelNameString))
			{
				MathLib::Vector3 frontDir = m_modelDB->dbMetaModels[modelNameString]->frontDir;
				s->updateModelFrontDir(i, frontDir);

				MathLib::Vector3 upDir = m_modelDB->dbMetaModels[modelNameString]->upDir;
				s->updateModelUpDir(i, upDir);

				QString catName = m_modelDB->dbMetaModels[modelNameString]->getProcessedCatName();
				s->updateModelCat(i, catName);
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

void scene_lab::buildSemGraphForCurrentScene(int batchLoading)
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

	m_currSceneSemGraph = new SceneSemGraph(m_currScene, m_modelDB, m_relationExtractor);
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
	loadSceneList(1);

	std::set<QString> allModelNameStrings;
	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];
		updateModelMetaInfoForScene(m_currScene);
		buildSemGraphForCurrentScene(1);

		for (int i = 0; i < m_currScene->getModelNum(); i++)
		{
			allModelNameStrings.insert(m_currScene->getModelNameString(i));
		}
	}

	// collect corrected model cateogry
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
	if (m_currScene == NULL)
	{
		Simple_Message_Box("No scene is loaded");
		return;
	}

	m_currScene->buildRelationGraph();
}

void scene_lab::buildRelationGraphForSceneList()
{
	loadSceneList(0,1);

	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];

		buildRelationGraphForCurrentScene();
	}

	std::cout << "\nSceneLab: all scene relation graph generated.\n";
}

void scene_lab::collectModelInfoForSceneList()
{
	// collect model meta info for current scene list. (subset of the whole shapenetsem meta file)

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
	//testMatlab();

	// load metadata
	loadSceneList(1);

	if (m_relationModelManager!=NULL)
	{
		delete m_relationModelManager;
	}

	m_relationModelManager = new RelationModelManager(m_relationExtractor);

	for (int i = 0; i < m_sceneList.size(); i++)
	{
		m_currScene = m_sceneList[i];
		m_relationModelManager->updateCurrScene(m_currScene);

		m_relationModelManager->addRelativePosFromCurrScene();
	}

	m_relationModelManager->buildRelationModels();


}

void scene_lab::computeBBAlignMatForSceneList()
{
	// load mesh without OBB
	loadSceneList(0, 0, 0);

	for (int i = 0; i < m_sceneList.size(); i++)
	{
		// update front dir for alignment
		m_currScene = m_sceneList[i];
		m_currScene->computeModelBBAlignMat();

		qDebug() << "SceneLab: bounding box alignment matrix saved for " << m_currScene->getSceneName();
	}
}

void scene_lab::extractRelPosForSceneList()
{
	// load OBB only
	loadSceneList(0, 1);

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


