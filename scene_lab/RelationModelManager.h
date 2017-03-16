#pragma once
#include "RelationModel.h"

class RelationModel;
class CScene;

class RelationModelManager
{
public:
	RelationModelManager();
	~RelationModelManager();

	void extractRelativePositions();

public:
	std::vector<RelationModel*> m_relationModels;

private:
	std::vector<RelativePos> m_relativePostions;
	CScene *m_currScene;
};

