/**
 * MATLAB <-> Eigen interface
 * Emanuele Ruffaldi Scuola Superiore Sant'Anna - PERCRO 2012-2013
 *
 * V2
 *  - added struct
 *
 * Note Eigen is column major same as MATLAB differently to C
 *
 * TODO:
 * - uint8 for testing image based
 * - support for sparse <-> Eigen
 * - multidimensional <-> Eigen (but Eigen does not support multidimensional)
 *
 * For translation help see: http://eigen.tuxfamily.org/dox-devel/AsciiQuickReference.txt
 * 
 */
#pragma once

#include "mex.h"
#include <signal.h>
#include <vector>
#include <Eigen/Dense>


/// type used for accessig MAT matrices without copy
typedef Eigen::Map<Eigen::MatrixXd> mappedmatd_t;

typedef Eigen::Map<Eigen::MatrixXi> mappedmati_t;


/// check for a MATLAB array compatible with Matrixd
inline bool isValidMATd(const mxArray * p)
{
  return (!mxIsComplex(p) && mxIsDouble(p) && !mxIsSparse(p) && mxGetNumberOfDimensions(p) <= 2);
}

/// check for a MATLAB array compatible with Matrixd
inline bool isValidMATi(const mxArray * p)
{
  return (!mxIsComplex(p) && !mxIsSparse(p) && mxGetNumberOfDimensions(p) <= 2);
}

/// check for a MATLAB array compatible with std::string
inline bool isValidMATs(const mxArray * p)
{
  return (!mxIsComplex(p) && mxIsChar(p) && !mxIsSparse(p) && mxGetNumberOfDimensions(p) <= 2);
}


/**
 * Maps a mxArray to a Double Eigen Matrix
 */
inline mappedmatd_t fromMATd(const mxArray * p)
{
  if(!isValidMATd(p))
  {
      return mappedmatd_t(0,0,0);
  }

  mwSize xmrows = mxGetM(p);
  mwSize xncols = mxGetN(p);  
  return mappedmatd_t(mxGetPr(p),xmrows,xncols);
}

inline std::vector<mappedmatd_t> from3DMATd(const mxArray *p, int depth)
{
	const mwSize *dims = mxGetDimensions(p);
	mwSize xmrows = dims[0];
	mwSize xncols = dims[1];

	std::vector<mappedmatd_t> eignMat;

	for (int i=0; i < depth; i++)
	{
		eignMat.push_back(mappedmatd_t(&mxGetPr(p)[i*xmrows*xncols], xmrows, xncols));
	}

	return eignMat;
}

/**
 * Maps a mxArray to a Double Eigen Matrix
 */
inline mappedmati_t fromMATi(const mxArray * p)
{
  if(!isValidMATi(p))
  {
      return mappedmati_t(0,0,0);
  }

  mwSize xmrows = mxGetM(p);
  mwSize xncols = mxGetN(p);  
  return mappedmati_t((int*)mxGetPr(p),xmrows,xncols);
}


/**
 * toMAT converts an Eigen double matrix to a Matlab class
 */
inline mxArray * toMAT(const Eigen::MatrixXd& x)
{
  mxArray * r = mxCreateNumericMatrix(x.rows(), x.cols(), mxDOUBLE_CLASS, mxREAL);
  if(!r)
        return 0;
#if OLDCODE
  // explicit
  int n =  (x.rows()*x.cols());
  double * p = mxGetPr(r); // by columns
  
  for ( int index = 0; index < n ; index++ ) 
    p[index] = x(index);        
#else
	fromMATd(r) = x;
#endif
 return r;
 
}

/**
 * toMATi converts an Eigen double matrix to a Matlab class
 */
inline mxArray * toMATi(const Eigen::MatrixXi& x)
{
  mxArray * r = mxCreateNumericMatrix(x.rows(), x.cols(), mxINT32_CLASS, mxREAL);
  if(!r)
        return 0;
#if OLDCODE
  // explicit
  int n =  (x.rows()*x.cols());
  double * p = mxGetPr(r); // by columns
  
  for ( int index = 0; index < n ; index++ ) 
    p[index] = x(index);        
#else
  fromMATi(r) = x;
#endif
 return r;
 
}

/**
 * toMAT converts an Eigen double matrix to a Matlab class
 */
inline mxArray * toMAT(const double x)
{
  return mxCreateDoubleScalar(x);

}

/// conversion of a C string
inline mxArray * toMAT(const char * s)
{
  return mxCreateString(s);
}

/// conversion of a std::string
inline mxArray * toMAT(const std::string & x )
{
  mwSize s = x.size();
  mxArray * p = mxCreateCharArray(1,&s);
  if(!p)
    return p;
  mxChar *pd = mxGetChars(p);
  for(int i = 0; i < x.size(); i++)
    pd[i] = x[i];
  return p;
}

/// extracts the field key from the scalar struct
inline bool getStructField(const mxArray * p, const char * key, mxArray* & out)
{
    out = 0;
    if(!mxIsStruct(p) || mxGetNumberOfElements(p) != 1)
      return false;
    int index = mxGetFieldNumber(p,key);
    if(index < 0)
      return false;
    out = mxGetFieldByNumber(p,0,index);
    return out != 0;
}

inline bool getStructField(const mxArray * p, const char * key, double & out)
{
  mxArray *pp = 0;
  if(!getStructField(p,key,pp))
    return false;
  if(!isValidMATd(pp) || mxGetNumberOfElements(pp) != 1)
    return false;
  out = ((double*)mxGetPr(pp))[0];
  return true;
}

inline bool getStructField(const mxArray * p, const char * key, int & out)
{
  mxArray *pp = 0;
  if(!getStructField(p,key,pp))
    return false;
  if(!isValidMATd(pp) || mxGetNumberOfElements(pp) != 1)
    return false;
  out = (int)((double*)mxGetPr(pp))[0];
  return true;
}

/// returns the struct field as std::string
inline bool getStructField(const mxArray * p, const char * key, std::string & out)
{
  mxArray *pp = 0;
  if(!getStructField(p,key,pp))
    return false;
  if(!isValidMATs(pp))
    return false;
  int n = mxGetNumberOfElements(pp);
  out.resize(n);
  mxChar * cp = (mxChar*)mxGetPr(pp);
  for(int i = 0; i < n; i++)
    out[i] = (char)cp[i]; //(typeof(out)::value_type)
  return true;
}

/// returns the struct field as matrix
inline bool getStructField(const mxArray * p, const char * key, Eigen::MatrixXd & out)
{
  mxArray *pp = 0;
  if(!getStructField(p,key,pp))
    return false;
  if(!isValidMATd(pp))
    return false;
  out = fromMATd(pp);  
  return true;
}

/// returns the struct field as mapped array
inline bool getStructField(const mxArray * p, const char * key, mappedmatd_t & out)
{
  mxArray *pp = 0;
  if(!getStructField(p,key,pp))
    return false;
  if(!isValidMATd(pp))
    return false;
  out = fromMATd(pp);  
  return true;
}