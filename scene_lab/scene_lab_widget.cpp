#include "scene_lab_widget.h"
#include "scene_lab.h"

#include <QFileDialog>
#include <QTextStream>

scene_lab_widget::scene_lab_widget(scene_lab *s_lab, QWidget *parent/*=0*/)
	: m_scene_lab(s_lab), ui(new Ui::scene_lab_widget)
{
	ui->setupUi(this);

	m_lastUsedDirectory = QString();

	// scene processing
	connect(ui->loadSceneButton, SIGNAL(clicked()), m_scene_lab, SLOT(LoadScene()));
	connect(ui->buildRelationGraphButton, SIGNAL(clicked()), m_scene_lab, SLOT(BuildRelationGraphForCurrentScene()));
	connect(ui->buildSemGraphButton, SIGNAL(clicked()), m_scene_lab, SLOT(BuildSemGraphForCurrentScene()));


	// scene list processing
	connect(ui->buildOBBForListButton, SIGNAL(clicked()), m_scene_lab, SLOT(BuildOBBForSceneList()));
	connect(ui->buildRGForListButton, SIGNAL(clicked()), m_scene_lab, SLOT(BuildRelationGraphForSceneList()));
	connect(ui->buildSSGForListButton, SIGNAL(clicked()), m_scene_lab, SLOT(BuildSemGraphForSceneList()));
	connect(ui->extractMetaFileButton, SIGNAL(clicked()), m_scene_lab, SLOT(ExtractMetaFileForSceneList()));

	// relation model
	connect(ui->computeBBAlignMatForListButton, SIGNAL(clicked()), m_scene_lab, SLOT(ComputeBBAlignMatForSceneList()));
	connect(ui->extractRelPosForList, SIGNAL(clicked()), m_scene_lab, SLOT(ExtractRelPosForSceneList()));
	connect(ui->extractSuppProbButton, SIGNAL(clicked()), m_scene_lab, SLOT(ExtractSuppProbForSceneList()));

	connect(ui->buildRelativeRelationButton, SIGNAL(clicked()), m_scene_lab, SLOT(BuildRelativeRelationModels()));
	connect(ui->buildPairRelationButton, SIGNAL(clicked()), m_scene_lab, SLOT(BuildPairwiseRelationModels()));
	connect(ui->buildGroupRelationButton, SIGNAL(clicked()), m_scene_lab, SLOT(BuildGroupRelationModels()));

	connect(ui->batchBuildModelsButton, SIGNAL(clicked()), m_scene_lab, SLOT(BatchBuildModelsForList()));
	
	
	// rendering
	connect(ui->showOBBCheckBox, SIGNAL(stateChanged(int)), m_scene_lab, SLOT(updateSceneRenderingOptions()));
	connect(ui->showGraphCheckBox, SIGNAL(stateChanged(int)), m_scene_lab, SLOT(updateSceneRenderingOptions()));
	connect(ui->showFrontDirCheckBox, SIGNAL(stateChanged(int)), m_scene_lab, SLOT(updateSceneRenderingOptions()));
	connect(ui->showSuppPlaneCheckBox, SIGNAL(stateChanged(int)), m_scene_lab, SLOT(updateSceneRenderingOptions()));

	// model DB
	connect(ui->openModelDBViewerButton, SIGNAL(clicked()), m_scene_lab, SLOT(create_modelDBViewer_widget()));



}

scene_lab_widget::~scene_lab_widget()
{
	delete ui;
}

QString scene_lab_widget::loadSceneName()
{
	QString lastDirFileName = QDir::currentPath() + "/lastSceneDir.txt";
	QFile lastDirFile(lastDirFileName);

	if (lastDirFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream ifs(&lastDirFile);

		m_lastUsedDirectory = ifs.readLine();
		lastDirFile.close();
	}

	QString filename = QFileDialog::getOpenFileName(0, tr("Load scene"),
		m_lastUsedDirectory,
		tr("Scene File (*.txt *.th)"));

	if (filename.isEmpty()) return "";

	QFileInfo fileInfo(filename);
	m_lastUsedDirectory = fileInfo.absolutePath();

	// save last open dir
	if (lastDirFile.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		QTextStream ofs(&lastDirFile);
		ofs << m_lastUsedDirectory;

		lastDirFile.close();
	}

	return filename;
}
