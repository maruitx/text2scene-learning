#pragma once

#include <QGLViewer/qglviewer.h>

class ModelDatabase;
class ModelDBViewer_widget;

class CModel;

class ModelDBViewer : public QGLViewer
{
public:
	ModelDBViewer();
	~ModelDBViewer();
	
	void draw();
	void init();

	void updateModel(CModel *m);
	void setViewBounds();

	bool m_drawOBB;
	bool m_drawSupp;
	bool m_drawFaceClusters;
	bool m_drawFrontDir;

private:
	CModel* m_model;

};

