#pragma once
#include "../common/utilities/utility.h"

class CModel;
class Category;

enum ModelDBType
{
	ShapeNetDB = 0,
	SunCGDB
};

class DBMetaModel
{
public:
	DBMetaModel();
	DBMetaModel(const QString &s);
	~DBMetaModel();
	DBMetaModel(DBMetaModel *m);

	void setCatName(const QString &s) { m_categoryName = s; };
	QString getCatName() { return m_categoryName; };

	void addCandidateCatName(const QString &s) { m_CandidateCategoryNames.push_back(s); };
	void addWordNetLemmas(const QString &s) { m_wordNetLemmas.push_back(s); };
	QString getProcessedCatName();

	void extractAttributeFromCandidateCatNames();

	void setScale(double s) { m_scale = s; };
	double getScale() { return m_scale; };

	void setIdStr(const QString &idStr) { m_idStr = idStr; };
	QString getIdStr() { return m_idStr; };
	const QString& getShapeNetCatsStr();

	void setTransMat(const MathLib::Matrix4d &m) { m_initTrans = m; };
	MathLib::Matrix4d getTransMat() { return m_initTrans; };

	int dbID;
	MathLib::Vector3 frontDir;
	MathLib::Vector3 upDir;
	MathLib::Vector3 position;

	std::vector<MathLib::Vector3> suppPlaneCorners;
	std::vector<MathLib::Vector3> bbTopPlaneCorners;

	int parentId;
	std::vector<double> onSuppPlaneUV;
	double positionToSuppPlaneDist; // distance after the transformation (in the real world unit, inch for StanfordSceneDataBase)

	bool m_isCatNameProcessed;
	QString m_processedCatName;
	std::vector<QString> m_attributes;

private:
	QString m_idStr;
	QString m_categoryName;

	std::vector<QString> m_CandidateCategoryNames;
	std::vector<QString> m_wordNetLemmas;
	std::vector<QString> m_tags;

	double m_scale;
	MathLib::Matrix4d m_initTrans;
};

class ModelDatabase{
public:

	ModelDatabase();

	ModelDatabase(const QString &projectPath, int dBType);
	~ModelDatabase();

	QString getDBPath() { return m_dbPath; };

	// for ActSynth
	void loadModelTsv(const QString &modelsTsvFile);
	void readModelScaleFile(const QString &filename);

	bool loadSceneSpecifiedModelFile(const QString &filename, QStringList &objNameStrings = QStringList(), bool isSharedModelFile = false);
	bool isCatInDB(QString catname);

	void extractScaledAnnoModels();
	void extractModelWithTexture();

	// ShapeNet
	void loadShapeNetSemTxt();
	bool isModelInDB(const QString &s);
	DBMetaModel* getMetaModelByNameString(const QString &s);

	// SunCG
	void loadSunCGMetaData();
	void loadSunCGModelCat();
	void loadSunCGModelCatMap(); // map SunCG category to shapenet cats

	QString getUpdatedModelCat(const QString &catName, const QString &modelIdStr="");

	// annotation for specified models
	void loadSpecifiedCatMap();


	QString getMetaFileType() { return m_dbMetaFileType; };

	CModel* getModelById(QString idStr);
	CModel* getModelByCat(const QString &catName);	
	Category* getCategory(QString catName);

	QString getModelCat(const QString &idStr);

	QString getModelIdStr(int id);
	int getModelNum();

	int getParentCatNum() { return m_parentCatNum; };

	std::map<QString, DBMetaModel*> dbMetaModels; // <modelIdStr, CandidateModel>
	std::map<QString, Category*> dbCategories;  // <categoryName, categoryStruct>

	std::vector<std::string> modelMetaInfoStrings;

private:
	QString m_dbPath;
	QString m_dbMetaFileType;
	QString m_projectPath;

	int m_dbType;

	int m_modelNum;
	int m_parentCatNum;

	std::map<QString, QString> m_modelCatMapSunCG;
	std::map<QString, QString> m_specifiedModelCatMap;  // other manual annotation for specified models
};