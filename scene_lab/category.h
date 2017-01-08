#include "../common/utilities/utility.h"


class DBMetaModel;

class Category
{
public:
	Category();
	Category(const QString &s);
	~Category();
	DBMetaModel* sampleInstance();
	void setCatName(const QString &n) { m_catName = n; };
	QString getCatName() { return m_catName; };

	void addInstance(DBMetaModel *m) { modelInstances.push_back(m); };
	int getInstanceNum() { return modelInstances.size(); };

	void setCatgoryLevel(int l){ m_categoryLevel = l; };
	int getCatgoryLevel() { return m_categoryLevel; };
	void addSubCatNames(const QString &s) { m_subCatNames.push_back(s); };

	bool isInModelBlackList(const QString &s);
	bool isSharedModel(const QString &s);

	std::vector<DBMetaModel*> modelInstances;

private:
	QString m_catName;
	std::vector<QString> m_subCatNames;
	

	QStringList m_modelBlackList;
	QStringList m_sharedModelList;

	int m_categoryLevel;
};