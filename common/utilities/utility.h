#pragma once

#include <QString>
#include <QVector>
#include <QObject>
#include <QColor>
#include <QMessageBox>
#include <QPainter>

#include <fstream>
#include <sstream>
#include "mathlib.h"
#include "qglviewer/quaternion.h"
#include <QQuaternion>
#include <time.h>
#include <Eigen/Dense>
#include <random>
#include <chrono>


const double MAX_VALUE = std::numeric_limits<double>::max();
const double ML_PI = 3.141592653589793238;
const double ML_PI_2 = 1.570796326794896558;
const double ML_PI_4 = 0.785398163397448279;

#define MAX_STR_BUF_SIZE	1024

#define SAFE_DELETE( ptr ) \
	if(ptr) \
			{ \
		delete ptr; \
		ptr = NULL; \
			}
#define SAFE_DELETE_ARRAY( ptr ) \
	if(ptr) \
			{ \
		delete [] ptr; \
		ptr = NULL; \
			}

const int ColorNum = 16;

const int ColorSet[16][3] = {
		{ 130, 130, 240 }, { 255, 120, 120 }, { 46, 254, 100 }, { 250, 88, 172 },
		{ 250, 172, 88 }, { 129, 247, 216 }, { 200, 200, 50 }, { 226, 169, 143 }, { 8, 138, 41 },
		{ 1, 223, 215 }, { 11, 76, 95 }, { 190, 182, 90 },
		{ 245, 169, 242 }, { 75, 138, 8 }, { 247, 254, 46 }, { 88, 172, 250 }
};

static QColor GetColorFromSet(int i)
{
	i = i%ColorNum;
	return QColor(ColorSet[i][0], ColorSet[i][1], ColorSet[i][2], 255);
}

//////////////////////////////////////////////////////////////////////////
// Primitive Shapes

// OBB Box
//    4------0
//   /|     /|
//  5-|----1 |
//  | 7----|-3
//  |/     |/
//  6------2
const int psBoxNNbEPerE = 4;
const int boxNumQuadFace = 6;

// order: TopRightCeil  TopRight BottomRight BottomRightCeil TopLeftCeil TopLeft BottomLeft BottomLeftCeil    
const int boxTriFace[12][3] = {
	// triangular faces
	{ 0, 1, 2 }, { 2, 3, 0 },
	{ 6, 5, 4 }, { 4, 7, 6 },
	{ 1, 0, 4 }, { 4, 5, 1 },
	{ 1, 5, 6 }, { 6, 2, 1 },
	{ 2, 6, 7 }, { 7, 3, 2 },
	{ 0, 3, 7 }, { 7, 4, 0 }
};

const int boxTriFacePair[12] = {1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10};

const int boxQuadFace[6][4] = {
	// quad faces
		{ 0, 1, 2, 3 }, { 6, 5, 4, 7 },
		{ 1, 0, 4, 5 }, { 1, 5, 6, 2 },
		{ 2, 6, 7, 3 }, { 0, 3, 7, 4 }
};

const int boxTriToQuadFaceMap[12] = {
	0,0,1,1,2,2,3,3,4,4,5,5
};

const int boxQuadToTriFaceMap[6][2] = {
		{ 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 8, 9 }, { 10, 11 }
};

const int boxFaceNormalOrientAlongAxis[12][2] = {
	// face normal orientation along axis
	// {axis_id (along which axis), orientation (1 means pointing to positive and -1 to negative)}
	{ 0, 1 }, { 0, 1 },
	{ 0, -1 }, { 0, -1 },
	{ 2, 1 }, { 2, 1 },
	{ 1, -1 }, { 1, -1 },
	{ 2, -1 }, { 2, -1 },
	{ 1, 1 }, { 1, 1 }
};


const int boxNumEdge = 12;
const int boxEdge[12][2] = {
		{ 0, 1 }, { 1, 2 },	// 0, 1
		{ 2, 3 }, { 3, 0 },	// 2, 3
		{ 0, 4 }, { 1, 5 },	// 4, 5
		{ 2, 6 }, { 3, 7 },	// 6, 7
		{ 4, 5 }, { 5, 6 },	// 8, 9
		{ 6, 7 }, { 7, 4 }	// 10, 11
};

const int boxNumFace = 12;

static void Simple_Message_Box(const QString &text)
{
	QMessageBox msg;
	msg.setText(text);
	msg.setButtonText(QMessageBox::Ok, "OK");
	msg.exec();
}

static void Simple_Message_Box(const char* text)
{
	QMessageBox msg;
	msg.setText(QString(text));
	msg.setButtonText(QMessageBox::Ok, "OK");
	msg.exec();
}

static bool FileExists(const std::string &filename)
{
	std::ifstream file(filename);
	return (!file.fail());
}

static std::vector<std::string> GetFileLines(const std::string &filename, unsigned int minLineLength)
{
	std::vector<std::string> result;
	if (!FileExists(filename)) {
		Simple_Message_Box(QString("Required file not found:%1").arg(QString(filename.c_str())));
		return result;
	}
	std::ifstream file(filename);

	std::string curLine;
	while (!file.fail())
	{
		std::getline(file, curLine);
		if (!file.fail() && curLine.length() >= minLineLength)
		{
			if (curLine.at(curLine.length() - 1) == '\r')
				curLine = curLine.substr(0, curLine.size() - 1);
			result.push_back(curLine);
		}
	}
	return result;
}

static std::vector<std::string> PartitionString(const std::string &s, const std::string &separator)
{
	std::vector<std::string> result;
	std::string curEntry;
	for (unsigned int outerCharacterIndex = 0; outerCharacterIndex < s.length(); outerCharacterIndex++)
	{
		bool isSeperator = true;
		for (unsigned int innerCharacterIndex = 0; innerCharacterIndex < separator.length() && outerCharacterIndex + innerCharacterIndex < s.length() && isSeperator; innerCharacterIndex++)
		{
			if (s[outerCharacterIndex + innerCharacterIndex] != separator[innerCharacterIndex]) {
				isSeperator = false;
			}
		}
		if (isSeperator) {
			if (curEntry.length() > 0) {
				result.push_back(curEntry);
				curEntry.clear();
			}
			outerCharacterIndex += separator.length() - 1;
		}
		else {
			curEntry.push_back(s[outerCharacterIndex]);
		}
	}
	if (curEntry.length() > 0) {
		result.push_back(curEntry);
	}
	return result;
}

// string between the protector string will not be partitioned, current support protector length is 1
// empty part will also be included in the parsing result
static std::vector<std::string> PartitionString(const std::string &s, const std::string &separator, const std::string &protectorStr)
{
	std::vector<std::string> result;
	std::string curEntry;
	for (unsigned int outerCharacterIndex = 0; outerCharacterIndex < s.length(); outerCharacterIndex++)
	{
		bool isSeperator = true;
		for (unsigned int innerCharacterIndex = 0; innerCharacterIndex < separator.length() && outerCharacterIndex + innerCharacterIndex < s.length() && isSeperator; innerCharacterIndex++)
		{
			if (s[outerCharacterIndex + innerCharacterIndex] != separator[innerCharacterIndex]) {
				isSeperator = false;
			}
		}
		if (isSeperator) {
			if (s[outerCharacterIndex + 1] != protectorStr[0])
			{
				if (curEntry.length() > 0) 
				{
					result.push_back(curEntry);
					curEntry.clear();
				}

				// if next character is also separator, push empty to curEntry
				if (s[outerCharacterIndex + 1] == separator[0])
				{
					result.push_back(curEntry);
					curEntry.clear();			
				}

				outerCharacterIndex += separator.length() - 1;
			}
			else
			{
				if (curEntry.length() > 0) 
				{
					result.push_back(curEntry);
					curEntry.clear();
				}
				outerCharacterIndex += separator.length() - 1;
				
				// add the length of protector string and skip the first protector
				outerCharacterIndex += 2; 
				// find another protector string
				bool isProtector = false;
				for (int j = outerCharacterIndex; j < s.length(); j++)
				{					
					if (s[j] == protectorStr[0])
					{
						isProtector = true;
					}

					if (isProtector)
					{
						if (curEntry.length() > 0) 
						{
							result.push_back(curEntry);
							curEntry.clear();

							outerCharacterIndex = j;  // set the index to be the char after the seperator
							break;  // break out of the loop for finding protector
						}
					}
					else
					{
						curEntry.push_back(s[j]);
					}
				}
			}			
		}
		else {
			curEntry.push_back(s[outerCharacterIndex]);
		}
	}
	if (curEntry.length() > 0) {
		result.push_back(curEntry);
	}
	return result;
}

static int StringToInt(const std::string &s)
{
	std::stringstream stream(std::stringstream::in | std::stringstream::out);
	stream << s;

	int result;
	stream >> result;
	return result;
}

static std::vector<int> StringToIntegerList(const std::string &s, const std::string &prefix, const std::string &separator = " ")
{
	std::string subString;
	if (prefix == "")
	{
		subString = s;
	}
	else
	{
		subString = PartitionString(s, prefix)[0];
	}

	std::vector<std::string> parts = PartitionString(subString, separator);

	std::vector<int> result(parts.size());
	for (unsigned int resultIndex = 0; resultIndex < result.size(); resultIndex++)
	{
		result[resultIndex] = StringToInt(parts[resultIndex]);
	}
	return result;
}

static float StringToFloat(const std::string &s)
{
	std::stringstream stream(std::stringstream::in | std::stringstream::out);
	stream << s;

	float result;
	stream >> result;
	return result;
}

static std::vector<float> StringToFloatList(const std::string &s, const std::string &prefix, const std::string &separator = " ")
{
	std::string subString;
	if (prefix == "")
	{
		subString = s;
	}
	else
	{
		subString = PartitionString(s, prefix)[0];
	}
	std::vector<std::string> parts = PartitionString(subString, separator);

	std::vector<float> result(parts.size());
	for (unsigned int resultIndex = 0; resultIndex < result.size(); resultIndex++)
	{
		result[resultIndex] = StringToFloat(parts[resultIndex]);
	}
	return result;
}

static QString GetTransformationString(const MathLib::Matrix4d &transMat)
{
	QString outString;
	QTextStream outStream(&outString);

	// Matrix4d.M is saved column-wise
	// transformation string is saved column-wise
	outStream << QString("%1").arg(transMat.M[0], 0, 'f') << " " << QString("%1").arg(transMat.M[1], 0, 'f') << " " << QString("%1").arg(transMat.M[2], 0, 'f') << " " << QString("%1").arg(transMat.M[3], 0, 'f') << " "
		<< QString("%1").arg(transMat.M[4], 0, 'f') << " " << QString("%1").arg(transMat.M[5], 0, 'f') << " " << QString("%1").arg(transMat.M[6], 0, 'f') << " " << QString("%1").arg(transMat.M[7], 0, 'f') << " "
		<< QString("%1").arg(transMat.M[8], 0, 'f') << " " << QString("%1").arg(transMat.M[9], 0, 'f') << " " << QString("%1").arg(transMat.M[10], 0, 'f') << " " << QString("%1").arg(transMat.M[11], 0, 'f') << " "
		<< QString("%1").arg(transMat.M[12], 0, 'f') << " " << QString("%1").arg(transMat.M[13], 0, 'f') << " " << QString("%1").arg(transMat.M[14], 0, 'f') << " " << QString("%1").arg(transMat.M[15], 0, 'f'); 

	return outString;
}

static QString GetTransformationString(const Eigen::MatrixXd &transMat)
{
	QString outString;
	QTextStream outStream(&outString);

	int numRow = transMat.rows();
	int numCol = transMat.cols();

	if (numRow == 0)
	{
		qDebug() << "";
	}
	
	for (int j = 0; j < numCol; j++)
	{
		for (int i = 0; i < numRow; i++)
		{
			outStream << QString("%1").arg(transMat(i, j), 0, 'f') << " ";
		}
	}

	outString = outString.left(outString.size() - 1);

	return outString;
}

static QString GetIntString(const std::vector<int> &intVector, const QString &separator)
{
	if (intVector.empty())
	{
		return QString("");
	}

	QString outString;
	QTextStream outStream(&outString);

	for (int i = 0; i < intVector.size()-1; i++)
	{
		outStream << intVector[i] << separator;
	}

	outStream << intVector[intVector.size() - 1];

	return outString;
}

static void DrawText_QPixmap(QPixmap *p, const QString &text, const QPoint pos = QPoint(50, 50), const QColor c = QColor(0, 255, 255, 255))
{
	QPainter painter(p);
	QPen pen(c);
	painter.setPen(c);

	QFont font(QFont("Arial"));
	font.setPointSize(24);
	painter.setFont(font);
	painter.drawText(pos, text);
}

static double GenRandomDouble(double minV, double maxV)
{
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937_64 rng(seed);
	//// initialize the random number generator with time-dependent seed
	//uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	//std::seed_seq ss{ uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32) };
	//rng.seed(ss);
	// initialize a uniform distribution between 0 and 1
	std::uniform_real_distribution<double> unif(minV, maxV);

	return unif(rng);
};

static void GenTwoRandomDouble(double minV, double maxV, double &v1, double &v2)
{
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937_64 rng(seed);
	//// initialize the random number generator with time-dependent seed
	//uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	//std::seed_seq ss{ uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32) };
	//rng.seed(ss);
	// initialize a uniform distribution between 0 and 1
	std::uniform_real_distribution<double> unif(minV, maxV);

	v1 = unif(rng);
	v2 = unif(rng);
};

static int GenRandomInt(int minV, int maxV)
{
	// generate a random number between minV and maxV, not including maxV

	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

	std::mt19937_64 rng(seed);
	//// initialize the random number generator with time-dependent seed
	//uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	//std::seed_seq ss{ uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32) };
	//rng.seed(ss);
	// initialize a uniform distribution between 0 and 1
	std::uniform_int_distribution<int> unif(minV, maxV - 1);

	return unif(rng);
}

static std::vector<int> GenNRandomInt(int minV, int maxV, int n, std::mt19937_64 rng)
{
	// generate N random number without rep between minV and maxV, not including maxV

	std::vector<int> ids(maxV - minV);
	std::vector<int> shuffledIds(n);

	for (int i = 0; i < ids.size(); i++)
	{
		ids[i] = minV + i;
	}

	//unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	//std::mt19937_64 rng(seed);

	std::shuffle(ids.begin(), ids.end(), rng);

	for (int i = 0; i < n; i++)
	{
		shuffledIds[i] = ids[i];
	}

	return shuffledIds;
}

// cannot put generator inside loop
static double GenNormalDistribution(double dMean, double dVar)
{
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

	std::default_random_engine generator(seed);
	std::normal_distribution<double> distribution(dMean, dVar);

	return distribution(generator);
}

static double GetNormalDistributionProb(double x, double dMean, double dVar)
{
	return (1.0 / sqrt(2 * MathLib::ML_PI))*exp(-0.5*(x - dMean)*(x - dMean) / dVar);
}

static double GenLogNormalDistribution(double dLogMean, double dLogVar)
{
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

	std::default_random_engine generator(seed);
	std::lognormal_distribution<double> distribution(dLogMean, dLogVar);

	return distribution(generator);
}

static double GetLogNormalDistributionProb(double x, double dMean, double dVar)
{
	return (1.0 / (x*dVar*sqrt(2 * MathLib::ML_PI)))*exp(-0.5*(log(x) - dMean)*(log(x) - dMean) / (dVar*dVar));

}

//mixture of von mises http://suvrit.de/work/soft/movmf/

static double randVonMise(double mean, double k)
{
	// https://sourceware.org/ml/gsl-discuss/2006-q1/msg00033.html

	double result = 0.0;

	double a = 1.0 + sqrt(1 + 4.0 * (k * k));
	double b = (a - sqrt(2.0 * a)) / (2.0 * k);
	double r = (1.0 + b * b) / (2.0 * b);

	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	std::normal_distribution<double> distribution(0, 1);

	while (1)
	{
		double U1 = distribution(generator);
		double z = cos(M_PI * U1);
		double f = (1.0 + r * z) / (r + z);
		double c = k * (r - f);
		double U2 = distribution(generator);

		if (c * (2.0 - c) - U2 > 0.0)
		{
			double U3 = distribution(generator);
			double sign = 0.0;
			if (U3 - 0.5 < 0.0)
				sign = -1.0;
			if (U3 - 0.5 > 0.0)
				sign = 1.0;
			result = sign * acos(f) + mean;
			while (result >= 2.0 * M_PI)
				result -= 2.0 * M_PI;
			break;
		}
		else
		{
			if (log(c / U2) + 1.0 - c >= 0.0)
			{
				double U3 = distribution(generator);
				double sign = 0.0;
				if (U3 - 0.5 < 0.0)
					sign = -1.0;
				if (U3 - 0.5 > 0.0)
					sign = 1.0;
				result = sign * acos(f) + mean;
				while (result >= 2.0 * M_PI)
					result -= 2.0 * M_PI;
				break;
			}
		}
	}
	return result;
}


// angle should be in radians
static MathLib::Matrix4d GetRotMat(MathLib::Vector3 rotAxis, double angle)
{
	MathLib::Matrix4d rotMat;
	rotMat.setidentity();

	qglviewer::Quaternion rotQ(qglviewer::Vec(rotAxis[0], rotAxis[1], rotAxis[2]), angle);

    double rotMatData[3][3];
	rotQ.getRotationMatrix(rotMatData);   // row-wise data?

	for (unsigned int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			rotMat.m[i][j] = rotMatData[i][j];
		}
	}

	return rotMat;
}

static MathLib::Matrix4d GetRotMat(const MathLib::Vector3 &fromVec, const MathLib::Vector3 &toVec)
{
	QQuaternion rotQ = QQuaternion::rotationTo(QVector3D(fromVec[0], fromVec[1], fromVec[2]), QVector3D(toVec[0], toVec[1], toVec[2]));

	MathLib::Matrix4d rotMat;
	rotMat.setidentity();
	QMatrix3x3 qrotMat = rotQ.toRotationMatrix();

	for (unsigned int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			//rotMat.m[i][j] = qrotMat(i, j);   // QMatrix3x3 is saved in 
			rotMat.m[j][i] = qrotMat(i, j);
		}
	}

	return rotMat;
}

static double GetRotAngleR(MathLib::Vector3 beforeDir, MathLib::Vector3 afterDir, MathLib::Vector3 zDir)
{
	double angle = MathLib::AcosR(beforeDir.dot(afterDir));
	MathLib::Vector3 crossDir = beforeDir.cross(afterDir);

	if (crossDir.dot(zDir) < 0)
	{
		angle = -angle;
	}

	return angle;
}

static MathLib::Vector3 PermuteVectorInXY(MathLib::Vector3 inputVec, double permuRange)
{
	MathLib::Vector3 rotedVec;

	double rotTheta = GenRandomDouble(-0.5, 0.5);
	MathLib::Matrix4d rotMat = GetRotMat(MathLib::Vector3(0,0,1), rotTheta*permuRange);
	rotedVec = rotMat.transform(inputVec);
	return rotedVec;
}

template <class Type>
class CSymMat {
public:
	CSymMat(void) {}
	CSymMat(int s) { Resize(s); }
	CSymMat(int s, const Type &v) { Resize(s, v); }
	std::vector<Type> m;
	unsigned int n;
	bool IsEmpty(void) { return m.empty(); }
	void Resize(int s) { n = s; m.resize(n*(n + 1) / 2); }
	void Resize(int s, const Type &v) { n = s; m.resize(n*(n + 1) / 2, v); }
	void Set(int i, int j, const Type &v) {
		if (i > j) { std::swap(i, j); }
		m[i*(i + 1) / 2 + j] = v;
	}
	Type Get(int i, int j) {
		if (i > j) { std::swap(i, j); }
		return m[i*(i + 1) / 2 + j];
	}
	const Type& Get(int i, int j) const {
		if (i > j) { std::swap(i, j); }
		return m[i*(i + 1) / 2 + j];
	}
};

template <class Type>
class CDistMat : public CSymMat < Type > {
public:
	CDistMat(void) {}
	CDistMat(int s) { Resize(s); }
	CDistMat(int s, const Type &v) { Resize(s, v); }
	void Resize(int s) { n = s; m.resize(n*(n - 1) / 2); }
	void Resize(int s, const Type &v) { n = s; m.resize(n*(n - 1) / 2, v); }
	void Set(int i, int j, const Type &v) {
		if (i == j) { return; }
		if (i > j) { std::swap(i, j); }
		m[i*(2 * n - i - 1) / 2 + j - i - 1] = v;
	}
	Type Get(int i, int j) {
		if (i == j) { return 0; }
		else if (i > j) { std::swap(i, j); }
		return m[i*(2 * n - i - 1) / 2 + j - i - 1];
	}
	const Type& Get(int i, int j) const {
		if (i == j) { return 0; }
		else if (i > j) { std::swap(i, j); }
		return m[i*(2 * n - i - 1) / 2 + j - i - 1];
	}
};

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
static const std::string currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
	// for more information about date/time format
	//strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
	strftime(buf, sizeof(buf), "%m-%d_%H-%M-%S", &tstruct);
	
	return buf;
}

static const std::vector<int> GetRandIntList(int sampleNum, int upperBound)
{
	std::vector<int> randList;

	for (int i = 0; i < sampleNum; i++) {
		bool flag;
		do {
			flag = false;
			int randIndex = GenRandomInt(0, upperBound);

			//make sure same row not chosen twice
			for (unsigned int j = 0; j < randList.size(); ++j) {
				if (randIndex == randList[j]) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				randList.push_back(randIndex);
			}
		} while (flag);
	}

	return randList;
}

static const std::vector<int> getRandIntList(int upperBound);

static Eigen::Matrix4d convertToEigenMat(const MathLib::Matrix4d &mat)
{
	Eigen::Matrix4d eigenMat;
	eigenMat << mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
		mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
		mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
		mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3];
	
	eigenMat.transpose();

	return eigenMat;
}

static MathLib::Matrix4d convertToMatrix4d(const Eigen::Matrix4d &eigenMat)
{
	MathLib::Matrix4d mat;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			//mat.m[i][j] = eigenMat(i,j);
			mat.m[j][i] = eigenMat(i, j);
		}
	}
	return mat;
}

static std::vector<int> topValueIDsFromPairs(std::vector<std::pair<double, int>> vpairs, int K, int sortMethod = 0)
{
	// sort <vaule, id> pair and return top K ids

	std::vector<int> topIDs;

	if (sortMethod == 0)  // default: sort by ascending order
	{
		std::sort(vpairs.begin(), vpairs.end());
	}
	else
	{
		std::sort(vpairs.begin(), vpairs.end());
		std::reverse(vpairs.begin(), vpairs.end());
	}

	for (int i = 0; i < K; i++)
	{
		topIDs.push_back(vpairs[i].second);
	}

	return topIDs;
}

static std::vector<int> topValueIDsFromVector(std::vector<double> v, int K, int sortMethod = 0)
{
	std::vector<std::pair<double, int>> vpairs(v.size());

	for (int i = 0; i < v.size(); i++)
	{
		vpairs[i].first = v[i];
		vpairs[i].second = i;
	}

	return topValueIDsFromPairs(vpairs, K, sortMethod);
}

static QString toQString(std::string s)
{
	return QString(s.c_str());
}

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#include <ctime>
#endif

/* Remove if already defined */
typedef long long int64; typedef unsigned long long uint64;

/* Returns the amount of milliseconds elapsed since the UNIX epoch. Works on both
* windows and linux. */

static uint64 GetTimeMs64()
{
#ifdef _WIN32
	/* Windows */
	FILETIME ft;
	LARGE_INTEGER li;

	/* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
	* to a LARGE_INTEGER structure. */
	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	uint64 ret = li.QuadPart;
	ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
	ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */

	return ret;
#else
	/* Linux */
	struct timeval tv;

	gettimeofday(&tv, NULL);

	uint64 ret = tv.tv_usec;
	/* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
	ret /= 1000;

	/* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
	ret += (tv.tv_sec * 1000);

	return ret;
#endif
}