#include "scene_lab.h"
#include "scene_lab_widget.h"
#include "modelDatabase.h"
#include "modelDBViewer_widget.h"
#include "../common/geometry/Scene.h"
#include "../t2scene/SceneSemGraph.h"

scene_lab::scene_lab(QObject *parent)
	: QObject(parent)
{
	m_widget = NULL;
	m_scene = NULL;
	m_currSceneSemGraph = NULL;

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
	if (m_scene != NULL)
	{
		delete m_scene;
	}

	CScene *scene = new CScene();
	scene->loadSceneFromFile(m_widget->loadSceneName(), 0, 0);

	m_scene = scene;

	emit sceneLoaded();
}

void scene_lab::loadSceneList()
{
	//CScene *scene = new CScene();
	//scene->loadSceneFile(m_widget->loadSceneName(), 0, 0);
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

	if (m_scene != NULL)
	{
		m_scene->setShowModelOBB(showModelOBB);
		m_scene->setShowSceneGraph(showSuppGraph);

	}
	else
	{
		m_widget->ui->showOBBCheckBox->setChecked(false);
		m_widget->ui->showGraphCheckBox->setChecked(false);
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

void scene_lab::buildSemGraphForCurrentScene()
{
	if (m_modelDB == NULL)
	{
		m_modelDB = new ModelDatabase();
		m_modelDB->loadShapeNetSemTxt();
	}

	if (m_scene == NULL)
	{
		Simple_Message_Box("No scene is loaded");
		return;
	}

	if (m_currSceneSemGraph!=NULL)
	{
		delete m_currSceneSemGraph;
	}

	m_currSceneSemGraph = new SceneSemGraph(m_scene, m_modelDB);
	m_currSceneSemGraph->generateGraph();
	m_currSceneSemGraph->saveGraph();
}

void scene_lab::buildSemGraphForSceneList()
{
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

			if (m_scene != NULL)
			{
				delete m_scene;
			}

			CScene *scene = new CScene();
			QString filename = sceneDBPath + "/" + sceneName + ".txt";
			scene->loadSceneFromFile(filename, 0, 1, 0);
			m_scene = scene;
			
			buildSemGraphForCurrentScene();
		}
	}

	std::cout << "\nSceneLab: all scene semantic graph generated.\n";
}
