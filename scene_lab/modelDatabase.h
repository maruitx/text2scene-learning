#pragma once
#include "../common/utilities/utility.h"

class CModel;
class Category;

class MetaModel
{
public:
	MetaModel();
	MetaModel(const QString &s);

	void setCatName(const QString &s) { m_categoryName = s; };
	QString getCatName() { return m_categoryName; };

	void addShapeNetCatName(const QString &s) { m_shapeNetCategoryNames.push_back(s); };
	QString getProcessedCatName();

	void setScale(double s) { m_scale = s; };
	double getScale() { return m_scale; };

	QString getIdStr() { return m_idStr; };
	const QString& getShapeNetCatsStr();

	void setTransMat(const MathLib::Matrix4d &m) { m_initTrans = m; };
	MathLib::Matrix4d getTransMat() { return m_initTrans; };

private:
	QString m_idStr;
	QString m_categoryName;

	std::vector<QString> m_shapeNetCategoryNames;
	std::vector<QString> m_tags;

	double m_scale;
	MathLib::Matrix4d m_initTrans;
};

class ModelDatabase{
public:
	ModelDatabase();
	ModelDatabase(const QString &dbPath);
	~ModelDatabase();

	QString getDBPath() { return m_dbPath; };

	void loadModelTsv(const QString &modelsTsvFile);
	void readModelScaleFile(const QString &filename);

	bool loadSceneSpecifiedModelFile(const QString &filename, QStringList &objNameStrings = QStringList(), bool isSharedModelFile = false);
	bool isCatInDB(QString catname);

	void extractScaledAnnoModels();
	void extractModelWithTexture();

	// ShapeNet
	void loadShapeNetSemTxt();
	MetaModel* getMetaModelByNameString(const QString &s){ return dbMetaModels[s]; };

	CModel* getModelById(QString idStr);
	CModel* getModelByCat(const QString &catName);	
	Category* getCategory(QString catName);

	QString getModelCat(const QString &idStr);

	QString getModelIdStr(int id);
	int getModelNum();

	int getParentCatNum() { return m_parentCatNum; };

	std::map<QString, MetaModel*> dbMetaModels; // <modelIdStr, CandidateModel>
	std::map<QString, Category*> dbCategories;  // <categoryName, categoryStruct>


private:
	QString m_dbPath;
	QString m_dbMetaFileType;

	int m_modelNum;
	int m_parentCatNum;
};