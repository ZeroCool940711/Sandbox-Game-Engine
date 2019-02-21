/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef SCRIPT_HPP
#define SCRIPT_HPP

#include "Python.h"

#include <map>
#include <string>
#include <typeinfo>
#include <vector>

#include "cstdmf/intrusive_object.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stdmf.hpp"

class Pickler;
class Vector2;
class Vector3;
class Vector4;
struct Direction3D;
class Matrix;
class Capabilities;

namespace Mercury
{
	class Address;
}


typedef SmartPointer<PyObject> PyObjectPtr;

/**
 * 	This namespace provides scripting helper functions.
 *
 * 	@ingroup script
 */
namespace Script
{
	extern int g_scriptArgc;
	extern char * g_scriptArgv[];

	bool init( const std::string & pythonPaths,
					const char * componentName = "Unknown" );
	void fini( bool shouldFinalise = true );

	bool isFinalised();

	PyThreadState* createInterpreter();
	void destroyInterpreter( PyThreadState* pInterpreter );
	PyThreadState* swapInterpreter( PyThreadState* pInterpreter );

	/**
	 * TODO: to be documented.
	 */
	class AutoInterpreterSwapper
	{
		PyThreadState*	pSwappedOutInterpreter_;
	public:
		explicit AutoInterpreterSwapper( PyThreadState* pNewInterpreter ) :
			pSwappedOutInterpreter_( swapInterpreter( pNewInterpreter ) )
		{}
		~AutoInterpreterSwapper()
		{
			swapInterpreter( pSwappedOutInterpreter_ );
		}
	};

	void initThread( bool plusOwnInterpreter = false );
	void finiThread( bool plusOwnInterpreter = false );

	void acquireLock();
	void releaseLock();

	bool call(
		PyObject * pFunction,
		PyObject * pArgs,
		const char * errorPrefix = "",
		bool okIfFunctionNull = false );

	PyObject * ask(
		PyObject * pFunction,
		PyObject * pArgs,
		const char * errorPrefix = "",
		bool okIfFunctionNull = false,
		bool printException = true );

	PyObject * newClassInstance( PyObject * pClass );

	bool unloadModule( const char * moduleName );

	int setData( PyObject * pObj, bool & rVal, const char * varName = "" );
	int setData( PyObject * pObj, int  & rVal, const char * varName = "" );
	int setData( PyObject * pObj, uint & rVal, const char * varName = "" );
	int setData( PyObject * pObj, float & rVal, const char * varName = "" );
	int setData( PyObject * pObj, double & rVal, const char * varName = "" );
	int setData( PyObject * pObj, int64 & rVal, const char * varName = "" );
	int setData( PyObject * pObj, uint64 & rVal, const char * varName = "" );
	int setData( PyObject * pObj, Vector2 & rVal, const char * varName = "" );
	int setData( PyObject * pObj, Vector3 & rVal, const char * varName = "" );
	int setData( PyObject * pObj, Vector4 & rVal, const char * varName = "" );
	int setData( PyObject * pObj, Matrix & rVal, const char * varName = "" );
	int setData( PyObject * pObj, PyObject * & rVal,
		const char * varName = "" );
	int setData( PyObject * pObj, SmartPointer<PyObject> & rPyObject,
		const char * varName = "" );
	int setData( PyObject * pObj, Capabilities & rCaps,
		const char * varName = "" );
	int setData( PyObject * pObj, std::string & rString,
		const char * varName = "" );
	int setData( PyObject * pObj, std::wstring & rString,
		const char * varName = "" );
	int setData( PyObject * pObj, char * & rString,
		const char * varName = "" );
	int setData( PyObject * pObj, Mercury::Address & rAddr,
		const char * varName = "" );

	template <class T>
	PyObject * getReadOnlyData( const T & d )
	{
		return getData( d );
	}

	PyObject * getData( const bool data );
	PyObject * getData( const int data );
	PyObject * getData( const uint data );
	PyObject * getData( const float data );
	PyObject * getData( const double data );
	PyObject * getData( const int64 data );
	PyObject * getData( const uint64 data );

	PyObject * getData( const Vector2 & data );
	PyObject * getData( const Vector3 & data );
	PyObject * getData( const Vector4 & data );
	PyObject * getData( const Direction3D & data );

	PyObject * getReadOnlyData( const Vector2 & data );
	PyObject * getReadOnlyData( const Vector3 & data );
	PyObject * getReadOnlyData( const Vector4 & data );

	PyObject * getDataRef( PyObject * pOwner, Vector2 * pData );
	PyObject * getDataRef( PyObject * pOwner, Vector3 * pData );
	PyObject * getDataRef( PyObject * pOwner, Vector4 * pData );

	PyObject * getData( const Matrix & data );
	PyObject * getData( const PyObject * data );
	PyObject * getData( ConstSmartPointer<PyObject> data );
	PyObject * getData( const Capabilities & data );
	PyObject * getData( const std::string & data );
	PyObject * getData( const std::wstring & data );
	PyObject * getData( const char * data );
	PyObject * getData( const Mercury::Address & addr );

	inline PyObject* newPyNoneRef()	{ Py_RETURN_NONE; }

#define INT_ACCESSOR( INPUT_TYPE, COMMON_TYPE )					\
	inline PyObject * getData( const INPUT_TYPE data )			\
		{ return getData( COMMON_TYPE( data ) ); }				\
	inline int setData( PyObject * pObject, INPUT_TYPE & rInt,	\
						const char * varName = "" )				\
		{														\
			COMMON_TYPE value;									\
			int result = setData( pObject, value, varName );	\
			rInt = INPUT_TYPE( value );							\
			return result;										\
		}


	INT_ACCESSOR( int8,  int );
	INT_ACCESSOR( int16, int );

	INT_ACCESSOR( uint8,  uint );
	INT_ACCESSOR( uint16, uint );



	/**
	 *	setData function for directly-addressable sequences (vectors, strings).
	 *	Lists would require a push_back-based implementation.
	 */
	template <class T, class SEQ> int setDataSequence( PyObject * pObj,
		SEQ & res, const char * varName )
	{
		if (!PySequence_Check( pObj ))
		{
			PyErr_Format( PyExc_TypeError, "%s must be set to a sequence of %s",
				varName, typeid(T).name() );
			return -1;
		}
		std::string eltVarName = varName; eltVarName += " element";
		uint sz = PySequence_Size( pObj );
		res.resize( sz );
		for (uint i = 0; i < sz; ++i)
		{
			PyObjectPtr pItem( PySequence_GetItem( pObj, i ), true );
			if (setData( &*pItem, res[i], eltVarName.c_str() ) != 0) return -1;
		}
		return 0;
	}

	/// setData for vectors
	template <class T, class A> int setData( PyObject * pObj,
		std::vector<T,A> & res, const char * varName = "" )
	{
		return setDataSequence<T>( pObj, res, varName );
	}

	// This does not appear to match std::string and std::wstring (at least with
	// VC7.0 and so the specific functions are still used.
	/// setData for basic_strings
	template <class C, class Tr, class A> int setData( PyObject * pObj,
		std::basic_string<C,Tr,A> & res, const char * varName = "" )
	{
		return setDataSequence<C>( pObj, res, varName );
	}

	/**
	 *	setData function for mappings (maps, multimaps).
	 */
	template <class K, class T, class MAP> int setDataMapping( PyObject * pObj,
		MAP & res, const char * varName )
	{
		if (!PyDict_Check( pObj ))	// using PyMapping API would be expensive
		{
			PyErr_Format( PyExc_TypeError, "%s must be set to a dict of %s: %s",
				varName, typeid(K).name(), typeid(T).name() );
			return -1;
		}
		std::string keyVarName = varName;
		std::string valueVarName = keyVarName;
		keyVarName += " key";
		valueVarName += " value";

		res.clear();
		Py_ssize_t pos = 0;
		PyObject * pKey, * pValue;
		while (PyDict_Next( pObj, &pos, &pKey, &pValue ))
		{
			std::pair<K,T> both;
			if (setData( pKey, both.first, keyVarName.c_str() ) != 0) return -1;
			if (setData( pValue, both.second, valueVarName.c_str() ) != 0) return -1;
			res.insert( both );
		}
		return 0;
	}

	// setData for maps
	template <class K, class T, class C, class A> int setData( PyObject * pObj,
		std::map<K,T,C,A> & res, const char * varName = "" )
	{
		return setDataMapping<K,T>( pObj, res, varName );
	}

	// setData for multimaps
	template <class K, class T, class C, class A> int setData( PyObject * pObj,
		std::multimap<K,T,C,A> & res, const char * varName = "" )
	{
		return setDataMapping<K,T>( pObj, res, varName );
	}

	// Compares two PyObject*, handling the case where one or both of them are
	// NULL.
	inline int compare( PyObject* apple, PyObject* orange )
	{
		if (apple && orange)
			return PyObject_Compare( apple, orange );
		return (apple) ? 1 : -1;
	}

	PyObject * buildReduceResult( const char * consName, PyObject * pConsArgs );

	PyObject * runString( const char * expression, bool printResult );

	/**
	 *	This class is a job that should be run at script init time.
	 *	Simply derive from it and implement the init method and your
	 *	job will be run immediately after scripts are initialised.
	 *
	 *	The rung specified in the constructor indicates how early on
	 *	you want your job to run. Negative rungs are for 'before
	 *	PyInitialise' and positive ones are for after that time.
	 *	(although before PyInitialise ones may not actually be run
	 *	before then - this is just to reduce possible conflicts).
	 *	Rung zero is reserved for module link initialisations.
	 *
	 *	If an init time job is constructed after scripts have been
	 *	initialised then it currently generates a critical error
	 *	(it cannot call its init fn because the derived constructor
	 *	 hasn't finished)
	 */
	class InitTimeJob
	{
	public:
		InitTimeJob( int rung );
		virtual ~InitTimeJob();

		virtual void init() = 0;
	};

	/**
	 *	This class is a job that should be run at script fini time.
	 *	Simply derive from it and implement the fini method and your
	 *	job will be run immediately after scripts are finalised.
	 *
	 *	The rung specified in the constructor indicates how early on
	 *	you want your job to run. Negative rungs are for 'after
	 *	PyFinalize' and positive ones are for before that time.
	 */
	class FiniTimeJob
	{
	public:
		FiniTimeJob( int rung = 1 );
		virtual ~FiniTimeJob();

		virtual void fini() = 0;
	};

	/**
	 *	A little helper function to deal with the result of an 'ask',
	 *	and display any resultant errors.
	 *
	 *	@return true on success
	 *	@see ask
	 */
	template <class C> bool setAnswer( PyObject * pResult, C & res,
		const char * errStr = "", bool printException = true )
	{
		if (pResult != NULL)
		{
			int err = setData( pResult, res, errStr );
			Py_DECREF( pResult );
			if (err == 0) return true;

			if (printException)
			{
				PyErr_PrintEx(0);
				PyErr_Clear();
			}
		}

		return false;
	}


	/**
	 *	A helper method to generate the python error when a function
	 *	is called with the wrong number of arguments
	 */
	PyObject * argCountError( const char * fn, int optas, int allas, ... );

	/**
	 *	A templatised helper method to get the name of a zero value for
	 *	various data types.
	 */
	template <class C> const char * zeroValueName()
	{
		return "None";
	}

	template <> const char * zeroValueName<int>();

	template <> const char * zeroValueName<float>();

#ifdef PY_EXTERNAL_FOPEN
	void addResPath( const std::string & resPath );
	void delResPath();
#endif

/*
 *	This function sets the input value to the value in the input dictionary
 *	with the specified name. If the dictionary does not have this entry, the
 *	value is unchanged.
 *
 *	@return If the value exists but has an invalid type, the Python exception
 *	state is set and false is returned.
 */
template <class TYPE>
bool getValueFromDict( PyObject * pDict, const char * name, TYPE & value )
{
	PyObject * pObj = PyDict_GetItemString( pDict, name );
	if (pObj)
	{
		if (Script::setData( pObj, value, name ) == -1)
		{
			return false;
		}
	}
	else
	{
		PyErr_Clear();
	}

	return true;
}


#if BWCLIENT_AS_PYTHON_MODULE
	const std::string getMainScriptPath();
#endif // BWCLIENT_AS_PYTHON_MODULE

};

// -----------------------------------------------------------------------------
// Section: Utility functions in the style of the python library
// -----------------------------------------------------------------------------

PyObject * PyClass_MatchNameInHierarchy( PyClassObject * pClass,
	const char * name );

PyObject * PyTuple_FromStringVector( const std::vector< std::string > & v );


// -----------------------------------------------------------------------------
// Section: SmartPointer glue
// -----------------------------------------------------------------------------


/**
 *	This template specialisation allows us to have a SmartPointer to
 *	ordinary an PyObject (PyObjectPlus works fine without these)
 */
template <>
inline void incrementReferenceCount( const PyObject & Q )
{
	Py_INCREF( const_cast<PyObject*>( &Q ) );	// Q guaranteed non-null
}

/**
 *	This template specialisation allows us to have a SmartPointer to
 *	ordinary an PyObject (PyObjectPlus works fine without these)
 */
template <>
inline void decrementReferenceCount( const PyObject & Q )
{
	Py_DECREF( const_cast<PyObject*>( &Q ) );	// Q guaranteed non-null
}

/**
 *	This template specialisation allows us to have a SmartPointer to
 *	an ordinary PyObject (PyObjectPlus works fine without these)
 */
template <>
inline bool hasZeroReferenceCount( const PyObject & Q )
{
	return (const_cast<PyObject*>( &Q )->ob_refcnt == 0);	// Q guaranteed non-null
}


// -----------------------------------------------------------------------------
// Section: Macros: Converters
// -----------------------------------------------------------------------------


/// This macro declares set and get data methods for pointers to a given class
#define PY_SCRIPT_CONVERTERS_DECLARE( CLASS )							\
namespace Script														\
{																		\
	int setData( PyObject * pObject, CLASS * & rpModel,					\
		const char * varName = "" );									\
																		\
	PyObject * getData( const CLASS * pModel );							\
																		\
	int setData( PyObject * pObject, SmartPointer<CLASS> & rpModel,		\
		const char * varName = "" );									\
																		\
	PyObject * getData( ConstSmartPointer<CLASS> pModel );				\
};																		\

/// This macro defines set and get data methods for pointers to a PyObjectPlus
#define PY_SCRIPT_CONVERTERS( CLASS )									\
int Script::setData( PyObject * pObject, CLASS * & rpDerived,			\
	const char * varName )												\
{																		\
	CLASS * inputDerived = rpDerived;									\
																		\
	if (pObject == Py_None)												\
	{																	\
		rpDerived = NULL;												\
	}																	\
	else if (CLASS::Check( pObject ))									\
	{																	\
		rpDerived = static_cast<CLASS *>( pObject );					\
	}																	\
	else																\
	{																	\
		PyErr_Format( PyExc_TypeError,									\
			"%s must be set to a " #CLASS " or None", varName );		\
		return -1;														\
	}																	\
																		\
	Py_XINCREF( rpDerived );											\
	Py_XDECREF( inputDerived );											\
																		\
	return 0;															\
}																		\
																		\
PyObject * Script::getData( const CLASS * pDerived )					\
{																		\
	return Script::getData(												\
		static_cast< const PyObject * >( pDerived ) );					\
}																		\
																		\
int Script::setData( PyObject * pObject,								\
	SmartPointer<CLASS> & rpDerived, const char * varName )				\
{																		\
	if (pObject == Py_None)												\
	{																	\
		rpDerived = NULL;												\
	}																	\
	else if (CLASS::Check( pObject ))									\
	{																	\
		if (rpDerived.getObject() != pObject)							\
			rpDerived = static_cast<CLASS *>( pObject );				\
	}																	\
	else																\
	{																	\
		PyErr_Format( PyExc_TypeError,									\
			"%s must be set to a " #CLASS " or None", varName );		\
		return -1;														\
	}																	\
																		\
	return 0;															\
}																		\
																		\
PyObject * Script::getData( ConstSmartPointer<CLASS> pDerived )			\
{																		\
	return Script::getData(												\
		static_cast< const PyObject * >( pDerived.getObject() ) );		\
}																		\



// -----------------------------------------------------------------------------
// Section: Macros: Module Links
// -----------------------------------------------------------------------------

/// This macro declares an ordinary (class-less) python function
#define PY_MODULE_FUNCTION_DECLARE( NAME )								\
	PyObject * NAME( PyObject * pArgs );								\

/// This macro defines an ordinary function and adds it to a module
#define PY_MODULE_FUNCTION( FUNC_NAME, MODULE_NAME ) 					\
	PY_MODULE_FUNCTION_WITH_DOC( FUNC_NAME, MODULE_NAME, NULL )					\

#define PY_MODULE_FUNCTION_WITH_DOC( FUNC_NAME, MODULE_NAME, DOC )		\
	static PyObject * _py_##FUNC_NAME( PyObject *, PyObject * args )	\
	{																	\
		return py_##FUNC_NAME( args );									\
	}																	\
																		\
	static PyModuleMethodLink s_link_py_##FUNC_NAME(					\
		#MODULE_NAME, #FUNC_NAME, _py_##FUNC_NAME, DOC );				\

/// This macro defines an ordinary function and adds it to a module
#define PY_MODULE_FUNCTION_WITH_KEYWORDS( FUNC_NAME, MODULE_NAME )		\
	PY_MODULE_FUNCTION_WITH_KEYWORDS_WITH_DOC(							\
		FUNC_NAME, MODULE_NAME, NULL )

#define PY_MODULE_FUNCTION_WITH_KEYWORDS_WITH_DOC( FUNC_NAME, 			\
		MODULE_NAME, DOC ) 												\
	static PyObject * _py_##FUNC_NAME( PyObject *, PyObject * args,		\
			PyObject * kwargs )											\
	{																	\
		return py_##FUNC_NAME( args, kwargs );							\
	}																	\
																		\
	static PyModuleMethodLink s_link_py_##FUNC_NAME(					\
		#MODULE_NAME, #FUNC_NAME, _py_##FUNC_NAME, DOC );				\

/// This macro defines an ordinary auto-parsed function and adds it to a module
#define PY_AUTO_MODULE_FUNCTION( RET, FUNC_NAME, ARGS, MODULE_NAME )	\
	PY_AUTO_MODULE_FUNCTION_WITH_DOC( RET, FUNC_NAME, ARGS, 			\
		MODULE_NAME, NULL )

#define PY_AUTO_MODULE_FUNCTION_WITH_DOC( RET, FUNC_NAME, ARGS, 		\
		MODULE_NAME, DOC )												\
	static PyObject * _py_##FUNC_NAME( PyObject *, PyObject * args )	\
	{																	\
		PY_AUTO_DEFINE_INT( RET, FUNC_NAME, FUNC_NAME, ARGS )			\
	}																	\
																		\
	static PyModuleMethodLink s_link_py_##FUNC_NAME(					\
		#MODULE_NAME, #FUNC_NAME, _py_##FUNC_NAME, DOC );				\

/// This macro defines an alias (another name) for a function in a module
#define PY_MODULE_FUNCTION_ALIAS( OLD_NAME, NEW_NAME, MODULE_NAME )		\
	PY_MODULE_FUNCTION_ALIAS_WITH_DOC( OLD_NAME, NEW_NAME, MODULE_NAME,	\
		NULL )

#define PY_MODULE_FUNCTION_ALIAS_WITH_DOC( OLD_NAME, NEW_NAME, 			\
		MODULE_NAME, DOC ) 												\
	static PyModuleMethodLink s_link_py_##NEW_NAME(						\
		#MODULE_NAME, #NEW_NAME, _py_##OLD_NAME, DOC );					\

/// This macro declares a static method in a class definition
#define PY_MODULE_STATIC_METHOD_DECLARE( NAME )							\
	static PyObject * NAME( PyObject * pArgs );							\
																		\
	static PyObject * _##NAME( PyObject *, PyObject * args )			\
	{																	\
		return This::NAME( args );										\
	}																	\
																		\
	static PyModuleMethodLink s_link_##NAME;							\

// This macro declares an auto-parsed static method in a class definition
#define PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RET, NAME, ARGS )			\
	static PyObject * _py_##NAME( PyObject *, PyObject * args )			\
	{																	\
		PY_AUTO_DEFINE_INT( RET, NAME, This::NAME, ARGS )				\
	}																	\
																		\
	static PyModuleMethodLink s_link_py_##NAME;							\

/// This macro defines a static method and adds it to a module
#define PY_MODULE_STATIC_METHOD( THIS_CLASS, METHOD_NAME, MODULE_NAME )	\
	PY_MODULE_STATIC_METHOD_WITH_DOC( THIS_CLASS, METHOD_NAME,			\
		MODULE_NAME, NULL )

#define PY_MODULE_STATIC_METHOD_WITH_DOC( THIS_CLASS, METHOD_NAME, 		\
		MODULE_NAME, DOC ) 												\
	PyModuleMethodLink THIS_CLASS::s_link_py_##METHOD_NAME(				\
		#MODULE_NAME, #METHOD_NAME, _py_##METHOD_NAME, DOC );			\

/// This macro adds the object returned by a given expression at script
/// init time. The expression is evaluated after scripts have been initialised.
#define PY_MODULE_ATTRIBUTE( MODULE_NAME, OBJECT_NAME, EXPR )			\
	static PyObject * s_construct_pyAttr##OBJECT_NAME()					\
		{ return EXPR; }												\
	static PyModuleResultLink s_link_pyAttr##OBJECT_NAME(				\
		#MODULE_NAME, #OBJECT_NAME, &s_construct_pyAttr##OBJECT_NAME );	\



/// This macro defines an unpickle function
#define PY_UNPICKLING_FUNCTION( FUNC_NAME, CONS_NAME )					\
	PY_UNPICKLING_FUNCTION_WITH_DOC( FUNC_NAME, CONS_NAME, NULL )

#define PY_UNPICKLING_FUNCTION_WITH_DOC( FUNC_NAME, CONS_NAME, DOC )	\
	static PyObject * _py_##FUNC_NAME( PyObject *, PyObject * args )	\
	{																	\
		return py_##FUNC_NAME( args );									\
	}																	\
																		\
	static PyModuleMethodLink s_link_py_##FUNC_NAME(					\
		"_BWp", #CONS_NAME, _py_##FUNC_NAME, DOC );						\

/// This macro defines an unpickle function with auto arg parsing
#define PY_AUTO_UNPICKLING_FUNCTION( RET, FUNC_NAME, ARGS, CONS_NAME )	\
	PY_AUTO_UNPICKLING_FUNCTION_WITH_DOC( RET, FUNC_NAME, ARGS, 		\
		CONS_NAME, NULL )

#define PY_AUTO_UNPICKLING_FUNCTION_WITH_DOC( RET, FUNC_NAME, ARGS, 	\
		CONS_NAME, DOC ) \
	static PyObject * _py_##FUNC_NAME( PyObject *, PyObject * args )	\
	{																	\
		PY_AUTO_DEFINE_INT( RET, FUNC_NAME, FUNC_NAME, ARGS )			\
	}																	\
																		\
	static PyModuleMethodLink s_link_py_##FUNC_NAME(					\
		"_BWp", #CONS_NAME, _py_##FUNC_NAME, DOC );						\


/// This macro declares an unpickle static method
#define PY_UNPICKLING_FACTORY_DECLARE()									\
	PY_MODULE_STATIC_METHOD_DECLARE( pyPickleResolve )					\

/// This macro declares an auto-parsed unpickle static method in a class defn
#define PY_AUTO_UNPICKLING_FACTORY_DECLARE( ARGS, CONS_NAME )			\
	static PyObject * _pyPickleResolve( PyObject *, PyObject * args )	\
	{																	\
		PY_AUTO_DEFINE_INT( RETOWN, PickleResolve, This::PickleResolve, ARGS )\
	}																	\
																		\
	static PyModuleMethodLink s_link_pyPickleResolve;					\

/// This macro defines an unpickling static method and adds it to the module
#define PY_UNPICKLING_FACTORY( THIS_CLASS, CONS_NAME )					\
	PyModuleMethodLink THIS_CLASS::s_link_pyPickleResolve(				\
		"_BWp", #CONS_NAME, THIS_CLASS::_pyPickleResolve );				\



// -----------------------------------------------------------------------------
// Section: Macros: Enums
// -----------------------------------------------------------------------------

/// This macro declares get/setData functions for an enum type
#define PY_ENUM_CONVERTERS_DECLARE( ENUMTYPE )							\
	namespace Script													\
	{																	\
		int setData( PyObject * pObject, ENUMTYPE & rData,				\
			const char * varName = "" );								\
																		\
		PyObject * getData( const ENUMTYPE data );						\
	};																	\


/// This macro defines get/setData functions for a simple enum type
#define PY_ENUM_CONVERTERS_CONTIGUOUS( ENUMTYPE )		\
	PY_ENUM_CONVERTER_SET( ENUMTYPE )					\
	PY_ENUM_CONVERTER_GET_CONTIGUOUS( ENUMTYPE )		\

/// This macro defines get/setData functions for a complex enum type
#define PY_ENUM_CONVERTERS_SCATTERED( ENUMTYPE )		\
	PY_ENUM_CONVERTER_SET( ENUMTYPE )					\
	PY_ENUM_CONVERTER_GET_SCATTERED( ENUMTYPE )			\

/// This internal macro writes the setData function for an enum type
#define PY_ENUM_CONVERTER_SET( ENUMTYPE )									\
	int Script::setData( PyObject * pObject, ENUMTYPE & rEnum,				\
		const char * varName )												\
	{																		\
		StringMap<ENUMTYPE>::iterator iter = ENUMTYPE##_smap.end();			\
																			\
		if (PyString_Check( pObject ))										\
			iter = ENUMTYPE##_smap.find( PyString_AsString( pObject ) );	\
																			\
		if (iter != ENUMTYPE##_smap.end())									\
		{																	\
			rEnum = iter->second;											\
			return 0;														\
		}																	\
																			\
		std::string errStr = varName;										\
		errStr += " must be set to one of ";								\
		uint i;																\
		for (i = 0; i < ENUMTYPE##_smap.size(); i++)						\
		{																	\
			if (i)															\
			{																\
				if (i+1 != ENUMTYPE##_smap.size())							\
					errStr += ", ";											\
				else														\
					errStr += ", or ";										\
			}																\
			errStr += (ENUMTYPE##_smap.begin() + i)->first;					\
		}																	\
		if (!i) errStr += "<No legal values>";								\
																			\
		PyErr_SetString( PyString_Check( pObject ) ?						\
			PyExc_ValueError : PyExc_TypeError,								\
			errStr.c_str() );												\
		return -1;															\
	}																		\

/// This internal macro writes the getData function for a contiguous enum type
#define PY_ENUM_CONVERTER_GET_CONTIGUOUS( ENUMTYPE )						\
	PyObject * Script::getData( const ENUMTYPE data )						\
	{																		\
		int index = data;													\
		if (index >= 0 && index < (int)ENUMTYPE##_smap.size())				\
		{																	\
			return PyString_FromString(										\
				(ENUMTYPE##_smap.begin() + index)->first );					\
		}																	\
																			\
		Py_Return;															\
	}																		\

/// This internal macro writes the setData function for a scattered enum type
#define PY_ENUM_CONVERTER_GET_SCATTERED( ENUMTYPE )							\
	PyObject * Script::getData( const ENUMTYPE data )						\
	{																		\
		std::map<ENUMTYPE,StringMap<ENUMTYPE>::size_type>::iterator iter =	\
			ENUMTYPE##_emap.find( data );									\
		if (iter != ENUMTYPE##_emap.end())									\
		{																	\
			return PyString_FromString(										\
				(ENUMTYPE##_smap.begin() + iter->second)->first );			\
		}																	\
																			\
		Py_Return;															\
	}																		\


/// This macros starts the declaration of an enum map
#define PY_BEGIN_ENUM_MAP( ENUMTYPE, ENUMPREFIX )							\
	PY_BEGIN_ENUM_MAP_BODY( ENUMTYPE )										\
	PY_BEGIN_ENUM_MAP_SKIP( ENUMTYPE, ENUMPREFIX )

#define PY_BEGIN_ENUM_MAP_NOPREFIX( ENUMTYPE )								\
	PY_BEGIN_ENUM_MAP_BODY( ENUMTYPE )										\
	PY_BEGIN_ENUM_MAP_NOSKIP( ENUMTYPE )

#define PY_BEGIN_ENUM_MAP_BODY( ENUMTYPE )									\
	/** @internal */														\
	class ENUMTYPE##_SMap : public StringMap<ENUMTYPE>						\
	{																		\
	public:																	\
		ENUMTYPE##_SMap() { ENUMTYPE##_initMaps(); }						\
	};																		\
																			\
	/** @internal */														\
	class ENUMTYPE##_EMap :													\
				public std::map<ENUMTYPE,StringMap<ENUMTYPE>::size_type >	\
	{																		\
	public:																	\
		ENUMTYPE##_EMap() { ENUMTYPE##_initMaps(); }						\
	};																		\
																			\
	static ENUMTYPE##_SMap ENUMTYPE##_smap;									\
	static ENUMTYPE##_EMap ENUMTYPE##_emap;									\
																			\
	static void ENUMTYPE##_initMaps()										\
	{																		\
		static int callCount = 0;											\
		if (++callCount != 2) return;										\
		ENUMTYPE##_SMap & smap = ENUMTYPE##_smap;							\
		ENUMTYPE##_EMap & emap = ENUMTYPE##_emap;							\

#define PY_BEGIN_ENUM_MAP_SKIP( ENUMTYPE, ENUMPREFIX )						\
		const int skip = sizeof(#ENUMPREFIX)-1;								\
		const char * enumName;												\

#define PY_BEGIN_ENUM_MAP_NOSKIP( ENUMTYPE )								\
		const int skip = 0;													\
		const char * enumName;												\


/// This macro declares one element of an enum map
#define PY_ENUM_VALUE( ENUMVALUE )											\
		enumName = #ENUMVALUE + skip;										\
		smap.insert( std::make_pair( enumName, ENUMVALUE ) );				\
		emap.insert( std::make_pair( ENUMVALUE, smap.size()-1 ) );			\

/// This macro completes the declaration of an enum map
#define PY_END_ENUM_MAP()													\
	}																		\

/// This macro defines the static initialiser of an enum map
#define PY_ENUM_MAP( ENUMTYPE )												\
	ENUMTYPE##_SMap ENUMTYPE##_smap;										\
	ENUMTYPE##_EMap ENUMTYPE##_emap;										\


// -----------------------------------------------------------------------------
// Section: Macros: Recursion utilities
// -----------------------------------------------------------------------------
#if defined(__GNUC__)

// The following is a bit of wierdness to get around difference in
// preprocessers. Not exactly sure why the following works yet.

#define EXS(X) X
#define EX0(X) X
#define EX1(X) X
#define EX2(X) X
#define EX3(X) X
#define EX4(X) X
#define EX5(X) X
#define EX6(X) X
#define EX7(X) X
#define EX8(X) X
#define EX9(X) X

#else

#define EXS(X) EXXS(X)
#define EX0(X) EXX0(X)
#define EX1(X) EXX1(X)
#define EX2(X) EXX2(X)
#define EX3(X) EXX3(X)
#define EX4(X) EXX4(X)
#define EX5(X) EXX5(X)
#define EX6(X) EXX6(X)
#define EX7(X) EXX7(X)
#define EX8(X) EXX8(X)
#define EX9(X) EXX9(X)

#define EXXS(X) X
#define EXX0(X) X
#define EXX1(X) X
#define EXX2(X) X
#define EXX3(X) X
#define EXX4(X) X
#define EXX5(X) X
#define EXX6(X) X
#define EXX7(X) X
#define EXX8(X) X
#define EXX9(X) X

#endif


#define CHAIN_S( F, X ) EXS( CHAIN_0 F##_##X )
#define CHAIN_0( M, F, A, R ) EX0( M##_##F(1,A,R) )
#define CHAIN_1( M, F, A, R ) EX1( M##_##F(2,A,R) )
#define CHAIN_2( M, F, A, R ) EX2( M##_##F(3,A,R) )
#define CHAIN_3( M, F, A, R ) EX3( M##_##F(4,A,R) )
#define CHAIN_4( M, F, A, R ) EX4( M##_##F(5,A,R) )
#define CHAIN_5( M, F, A, R ) EX5( M##_##F(6,A,R) )
#define CHAIN_6( M, F, A, R ) EX6( M##_##F(7,A,R) )
#define CHAIN_7( M, F, A, R ) EX7( M##_##F(8,A,R) )
#define CHAIN_8( M, F, A, R ) EX8( M##_##F(9,A,R) )
#define CHAIN_9( M, F, A, R ) EX9( M##_##F(10,A,R) )

#define PAIR_1(A,B) A
#define PAIR_2(A,B) B


// -----------------------------------------------------------------------------
// Section: Macros: Automatic argument parsing
// -----------------------------------------------------------------------------


/// entry macro for the body of parsing functions
#define PY_AUTO_DEFINE_INT( RET, NAME, FNNAME, ARGS )					\
	const int argc = PyTuple_Size( args );								\
																		\
	if (argc < PYAUTO_OPTARGC(ARGS) || argc > PYAUTO_ALLARGC(ARGS))		\
	{																	\
		return Script::argCountError( #NAME,							\
			PYAUTO_OPTARGC(ARGS), PYAUTO_ALLARGC(ARGS)					\
			PYAUTO_ARGTYPES(ARGS) ;										\
	}																	\
																		\
	PYAUTO_WRITE(ARGS)													\
																		\
	PYAUTO_##RET(FNNAME,ARGS)											\


/// macros for arg count up to optional args
#define PYAUTO_OPTARGC_PARSE_ARG(T,R) (PYAUTO_OPTARGC_DO,ARG,U,R)
#define PYAUTO_OPTARGC_PARSE_NZARG(T,R) (PYAUTO_OPTARGC_DO,ARG,U,R)
#define PYAUTO_OPTARGC_PARSE_OPTARG(T,DEF,R) (PYAUTO_OPTARGC_DO,END,U,U)
#define PYAUTO_OPTARGC_PARSE_OPTNZARG(T,DEF,R) (PYAUTO_OPTARGC_DO,END,U,U)
#define PYAUTO_OPTARGC_PARSE_MAX_ARG(T,MAX,R) (PYAUTO_OPTARGC_DO,ARG,U,R)
#define PYAUTO_OPTARGC_PARSE_OPTMAX_ARG(T,MAX,DEF,R) (PYAUTO_OPTARGC_DO,END,U,U)
#define PYAUTO_OPTARGC_PARSE_CALLABLE_ARG(T,R) (PYAUTO_OPTARGC_DO,ARG,U,R)
#define PYAUTO_OPTARGC_PARSE_OPTCALLABLE_ARG(T,DEF,R) (PYAUTO_OPTARGC_DO,END,U,U)
#define PYAUTO_OPTARGC_PARSE_END (PYAUTO_OPTARGC_DO,END,U,U)

#define PYAUTO_OPTARGC_DO_ARG(N,A,R) CHAIN_##N PYAUTO_OPTARGC_PARSE_##R
#define PYAUTO_OPTARGC_DO_END(N,A,R) (N-1)

#define PYAUTO_OPTARGC( X ) CHAIN_S( PYAUTO_OPTARGC_PARSE, X )

/// macros for arg count of all args
#define PYAUTO_ALLARGC_PARSE_ARG(T,R) (PYAUTO_ALLARGC_DO,ARG,U,R)
#define PYAUTO_ALLARGC_PARSE_NZARG(T,R) (PYAUTO_ALLARGC_DO,ARG,U,R)
#define PYAUTO_ALLARGC_PARSE_OPTARG(T,DEF,R) (PYAUTO_ALLARGC_DO,ARG,U,R)
#define PYAUTO_ALLARGC_PARSE_OPTNZARG(T,DEF,R) (PYAUTO_ALLARGC_DO,ARG,U,R)
#define PYAUTO_ALLARGC_PARSE_MAX_ARG(T,MAX,R) (PYAUTO_ALLARGC_DO,ARG,U,R)
#define PYAUTO_ALLARGC_PARSE_OPTMAX_ARG(T,MAX,DEF,R) (PYAUTO_ALLARGC_DO,ARG,U,R)
#define PYAUTO_ALLARGC_PARSE_CALLABLE_ARG(T,R) (PYAUTO_ALLARGC_DO,ARG,U,R)
#define PYAUTO_ALLARGC_PARSE_OPTCALLABLE_ARG(T,DEF,R) (PYAUTO_ALLARGC_DO,ARG,U,R)
#define PYAUTO_ALLARGC_PARSE_END (PYAUTO_ALLARGC_DO,END,U,U)

#define PYAUTO_ALLARGC_DO_ARG(N,A,R) CHAIN_##N PYAUTO_ALLARGC_PARSE_##R
#define PYAUTO_ALLARGC_DO_END(N,A,R) (N-1)

#define PYAUTO_ALLARGC( X ) CHAIN_S( PYAUTO_ALLARGC_PARSE, X )

/// macros for the types of arguments
#define PYAUTO_ARGTYPES_PARSE_ARG(T,R) (PYAUTO_ARGTYPES_DO,ARG,T,R)
#define PYAUTO_ARGTYPES_PARSE_NZARG(T,R) (PYAUTO_ARGTYPES_DO,ARG,T,R)
#define PYAUTO_ARGTYPES_PARSE_OPTARG(T,DEF,R) (PYAUTO_ARGTYPES_DO,ARG,T,R)
#define PYAUTO_ARGTYPES_PARSE_OPTNZARG(T,DEF,R) (PYAUTO_ARGTYPES_DO,ARG,T,R)
#define PYAUTO_ARGTYPES_PARSE_MAX_ARG(T,MAX,R) (PYAUTO_ARGTYPES_DO,ARG,T,R)
#define PYAUTO_ARGTYPES_PARSE_OPTMAX_ARG(T,MAX,DEF,R) (PYAUTO_ARGTYPES_DO,ARG,T,R)
#define PYAUTO_ARGTYPES_PARSE_CALLABLE_ARG(T,R) (PYAUTO_ARGTYPES_DO,ARG,T,R)
#define PYAUTO_ARGTYPES_PARSE_OPTCALLABLE_ARG(T,DEF,R) (PYAUTO_ARGTYPES_DO,ARG,T,R)
#define PYAUTO_ARGTYPES_PARSE_END (PYAUTO_ARGTYPES_DO,END,U,U)

#define PYAUTO_ARGTYPES_DO_ARG(N,A,R) ,#A CHAIN_##N PYAUTO_ARGTYPES_PARSE_##R
#define PYAUTO_ARGTYPES_DO_END(N,A,R) )

#define PYAUTO_ARGTYPES( X ) CHAIN_S( PYAUTO_ARGTYPES_PARSE, X )


/// macros for writing the actual argument parsing code
#define PYAUTO_WRITE_PARSE_ARG(T,R) (PYAUTO_WRITE_DO,ARG,T,R)
#define PYAUTO_WRITE_PARSE_NZARG(T,R) (PYAUTO_WRITE_DO,NZARG,T,R)
#define PYAUTO_WRITE_PARSE_OPTARG(T,DEF,R) (PYAUTO_WRITE_DO,OPTARG,(T,DEF),R)
#define PYAUTO_WRITE_PARSE_OPTNZARG(T,DEF,R) (PYAUTO_WRITE_DO,ONZARG,(T,DEF),R)
#define PYAUTO_WRITE_PARSE_MAX_ARG(T,MAX,R) (PYAUTO_WRITE_DO,MAX_ARG,(T,MAX),R)
#define PYAUTO_WRITE_PARSE_OPTMAX_ARG(T,MAX,DEF,R) (PYAUTO_WRITE_DO,OMAX_ARG,((T,MAX),DEF),R)
#define PYAUTO_WRITE_PARSE_CALLABLE_ARG(T,R) (PYAUTO_WRITE_DO,CALLABLE_ARG,T,R)
#define PYAUTO_WRITE_PARSE_OPTCALLABLE_ARG(T,DEF,R) (PYAUTO_WRITE_DO,OCALLABLE_ARG,(T,DEF),R)
#define PYAUTO_WRITE_PARSE_END (PYAUTO_WRITE_DO,END,U,U)


#define PYAUTO_WRITE_STD(N,T)										\
	typedef T arg##N##_type;										\
	T arg##N = arg##N##_type();										\
	if (Script::setData( PyTuple_GetItem( args, N-1 ), arg##N,		\
		"() argument " #N ) != 0)									\
			return NULL;											\

#define PYAUTO_WRITE_OPT(N,T,DEF)									\
	T arg##N = DEF;													\
	if (argc >= N &&												\
		Script::setData( PyTuple_GetItem( args, N-1 ), arg##N,		\
			"() argument " #N " optionally" ) != 0)					\
				return NULL;										\

#define PYAUTO_WRITE_NZ(N,T)										\
	if (!arg##N)													\
	{																\
		PyErr_Format( PyExc_TypeError,								\
			"() argument " #N " cannot be %s",						\
			Script::zeroValueName< T >() );							\
		return NULL;												\
	}																\

#define PYAUTO_WRITE_MAX(N,T,MAX)									\
	if (arg##N > MAX )												\
	{																\
		PyErr_Format( PyExc_TypeError,								\
			"() argument " #N " is greater than maximum value" );	\
		return NULL;												\
	}																\

#define PYAUTO_WRITE_CALLABLE(N,T)									\
	if (!PyCallable_Check(&*arg##N))								\
	{																\
		PyErr_Format( PyExc_TypeError,								\
			"() argument " #N " must be callable" );				\
		return NULL;												\
	}																\

#define PYAUTO_WRITE_DO_ARG(N,A,R)									\
	PYAUTO_WRITE_STD(N,A)											\
	CHAIN_##N PYAUTO_WRITE_PARSE_##R								\

#define PYAUTO_WRITE_DO_NZARG(N,A,R)								\
	PYAUTO_WRITE_STD(N,A)											\
	PYAUTO_WRITE_NZ(N,A)											\
	CHAIN_##N PYAUTO_WRITE_PARSE_##R								\

#define PYAUTO_WRITE_DO_OPTARG(N,A,R)								\
	PYAUTO_WRITE_OPT(N,PAIR_1 A, PAIR_2 A)							\
	CHAIN_##N PYAUTO_WRITE_PARSE_##R								\

#define PYAUTO_WRITE_DO_ONZARG(N,A,R)								\
	PYAUTO_WRITE_OPT(N,PAIR_1 A, PAIR_2 A)							\
	PYAUTO_WRITE_NZ(N,PAIR_1 A)										\
	CHAIN_##N PYAUTO_WRITE_PARSE_##R								\

#define PYAUTO_WRITE_DO_MAX_ARG(N,A,R)								\
	PYAUTO_WRITE_STD(N,PAIR_1 A)									\
	PYAUTO_WRITE_MAX(N,PAIR_1 A, PAIR_2 A)							\
	CHAIN_##N PYAUTO_WRITE_PARSE_##R								\

#define PYAUTO_WRITE_DO_OMAX_ARG(N,A,R)								\
	PYAUTO_WRITE_OPT(N,PAIR_1 PAIR_1 A, PAIR_2 A)					\
	PYAUTO_WRITE_MAX(N,PAIR_1 PAIR_1 A, PAIR_2 PAIR_1 A)			\
	CHAIN_##N PYAUTO_WRITE_PARSE_##R								\

#define PYAUTO_WRITE_DO_CALLABLE_ARG(N,A,R)							\
	PYAUTO_WRITE_STD(N,A)											\
	PYAUTO_WRITE_NZ(N,A)											\
	PYAUTO_WRITE_CALLABLE(N,A)										\
	CHAIN_##N PYAUTO_WRITE_PARSE_##R								\

#define PYAUTO_WRITE_DO_OCALLABLE_ARG(N,A,R)						\
	PYAUTO_WRITE_OPT(N,PAIR_1 A, PAIR_2 A)							\
	PYAUTO_WRITE_NZ(N,PAIR_1 A)										\
	PYAUTO_WRITE_CALLABLE(N,PAIR_1 A)								\
	CHAIN_##N PYAUTO_WRITE_PARSE_##R								\

#define PYAUTO_WRITE_DO_END(N,A,R) (void)0;

#define PYAUTO_WRITE( X ) CHAIN_S( PYAUTO_WRITE_PARSE, X )


/// macros for calling the function with the parsed arguments
#define PYAUTO_CALLFN_PARSE_ARG(T,R) (PYAUTO_CALLFN_DO,ARG,U,R)
#define PYAUTO_CALLFN_PARSE_NZARG(T,R) (PYAUTO_CALLFN_DO,ARG,U,R)
#define PYAUTO_CALLFN_PARSE_OPTARG(T,DEF,R) (PYAUTO_CALLFN_DO,ARG,U,R)
#define PYAUTO_CALLFN_PARSE_OPTNZARG(T,DEF,R) (PYAUTO_CALLFN_DO,ARG,U,R)
#define PYAUTO_CALLFN_PARSE_MAX_ARG(T,MAX,R) (PYAUTO_CALLFN_DO,ARG,U,R)
#define PYAUTO_CALLFN_PARSE_OPTMAX_ARG(T,MAX,DEF,R) (PYAUTO_CALLFN_DO,ARG,U,R)
#define PYAUTO_CALLFN_PARSE_CALLABLE_ARG(T,R) (PYAUTO_CALLFN_DO,ARG,U,R)
#define PYAUTO_CALLFN_PARSE_OPTCALLABLE_ARG(T,DEF,R) (PYAUTO_CALLFN_DO,ARG,U,R)
#define PYAUTO_CALLFN_PARSE_END (PYAUTO_CALLFN_DO,END,U,U)

#define PYAUTO_CALLFN_PARSE1_ARG(T,R) (PYAUTO_CALLFN_DO,ARG1,U,R)
#define PYAUTO_CALLFN_PARSE1_NZARG(T,R) (PYAUTO_CALLFN_DO,ARG1,U,R)
#define PYAUTO_CALLFN_PARSE1_OPTARG(T,DEF,R) (PYAUTO_CALLFN_DO,ARG1,U,R)
#define PYAUTO_CALLFN_PARSE1_OPTNZARG(T,DEF,R) (PYAUTO_CALLFN_DO,ARG1,U,R)
#define PYAUTO_CALLFN_PARSE1_MAX_ARG(T,MAX,R) (PYAUTO_CALLFN_DO,ARG1,U,R)
#define PYAUTO_CALLFN_PARSE1_OPTMAX_ARG(T,MAX,DEF,R) (PYAUTO_CALLFN_DO,ARG1,U,R)
#define PYAUTO_CALLFN_PARSE1_CALLABLE_ARG(T,R) (PYAUTO_CALLFN_DO,ARG1,U,R)
#define PYAUTO_CALLFN_PARSE1_OPTCALLABLE_ARG(T,DEF,R) (PYAUTO_CALLFN_DO,ARG1,U,R)
#define PYAUTO_CALLFN_PARSE1_END (PYAUTO_CALLFN_DO,END,U,U)

#define PYAUTO_CALLFN_DO_ARG1(N,A,R)								\
	arg##N CHAIN_##N PYAUTO_CALLFN_PARSE_##R						\

#define PYAUTO_CALLFN_DO_ARG(N,A,R)									\
	, PYAUTO_CALLFN_DO_ARG1(N,A,R)									\

#define PYAUTO_CALLFN_DO_END(N,A,R) )

#define PYAUTO_CALLFN( NAME, X )									\
	NAME( CHAIN_S( PYAUTO_CALLFN_PARSE1, X )


/// macros to handle the various return value cases
#define PYAUTO_RETVOID( NAME, ARGS )								\
	PYAUTO_CALLFN( NAME, ARGS );									\
	Py_Return;														\

#define PYAUTO_RETDATA( NAME, ARGS )								\
	return Script::getData( PYAUTO_CALLFN( NAME, ARGS ) );			\

#define PYAUTO_RETOWN( NAME, ARGS )									\
	return PYAUTO_CALLFN( NAME, ARGS );								\

#define PYAUTO_RETOK( NAME, ARGS )									\
	if (PYAUTO_CALLFN( NAME, ARGS )) Py_Return;						\
	return NULL;													\

#define PYAUTO_RETERR( NAME, ARGS )									\
	if (PYAUTO_CALLFN( NAME, ARGS ) == 0) Py_Return;				\
	return NULL;													\



// -----------------------------------------------------------------------------
// Section: PyModuleLink
// -----------------------------------------------------------------------------


/**
 *	This class provides a link between functions and modules. Simply create a
 *	static object of this type to have your function put into the module you
 *	specify.
 *
 *	@note Objects of this type must be static or global, because the module
 *		retains a pointer into it.
 */
class PyModuleMethodLink : public Script::InitTimeJob
{
public:
	PyModuleMethodLink( const char * moduleName,
		const char * methodName, PyCFunction method,
		const char * docString = NULL );
	PyModuleMethodLink( const char * moduleName,
		const char * methodName, PyCFunctionWithKeywords method,
		const char * docString = NULL );
	~PyModuleMethodLink();

	const char * moduleName()	{ return moduleName_; }
	const char * methodName()	{ return methodName_; }

	virtual void init();

private:
	PyMethodDef	mdReal_;
	PyMethodDef	mdStop_;

	const char *	moduleName_;
	const char *	methodName_;
};


/**
 *	This class allows easy adding of objects to modules. Simply create a static
 *	object of this type to have your object put into the module specified. This
 *	steals a reference to the object.
 */
class PyModuleAttrLink : public Script::InitTimeJob
{
public:
	PyModuleAttrLink( const char * moduleName,
		const char * objectName,
		PyObject * pObject );

	virtual void init();

private:
	const char *	moduleName_;
	const char *	objectName_;
	PyObject *		pObject_;
};


/**
 *	This class allows easy adding of objects returned from functions to
 *	modules. Simply create a static object of this type to have the object
 *	returned by the function put into the module specified. This function
 *	will be called after scripts have been initialised (which makes it
 *	actually useful!). This function should return a new reference.
 */
class PyModuleResultLink : public Script::InitTimeJob
{
public:
	PyModuleResultLink( const char * moduleName,
		const char * objectName,
		PyObject * (*pFunction)() );

	virtual void init();

private:
	const char *	moduleName_;
	const char *	objectName_;
	PyObject *		(*pFunction_)();
};

typedef struct _typeobject PyTypePlus;

/**
 *	This class allows factory methods to be added to modules. It is the
 *	same as PyModuleMethodLink except that it also sets the module name
 *	in the type dictionary.
 *
 *	@see PyModuleMethodLink
 */
class PyFactoryMethodLink : public Script::InitTimeJob
{
public:
	PyFactoryMethodLink( const char * moduleName,
		const char * methodName, PyTypeObject * pType );
	~PyFactoryMethodLink();

	virtual void init();

private:
	const char * moduleName_;
	const char * methodName_;
	PyTypeObject * pType_;
};



// -----------------------------------------------------------------------------
// Section: WeakPyPtr
// -----------------------------------------------------------------------------

/**
 *	This template class is a weak reference to a Python object.
 *	It distinuishes between a reference which is just NULL and a reference
 *	which to an object which has expired (or is not weakly referencable)
 *	TODO: should use callback instead of checking in good all the time.
 *	Note: you cannot have a reference to Py_None.
 */
template <class Ty> class WeakPyPtr
{
public:
	static const bool STEAL_REFERENCE = true;
	static const bool NEW_REFERENCE = false;

	typedef Ty Object;
	typedef WeakPyPtr<Ty> This;

public:
	/**
	 *	This constructor initialises this pointer to refer to the input object.
	 */
	WeakPyPtr( Object * P = NULL, bool alreadyIncremented = false )
	{
		if (P != NULL)
		{
			weakref_ = PyWeakref_NewRef( P, NULL );
			if (weakref_ == NULL)
			{
				weakref_ = (Object*)1;
				PyErr_Clear();
			}
			if (alreadyIncremented) decrementReferenceCount( *P );
		}
		else weakref_ = NULL;
	}

	/**
	 *	The copy constructor.
	 */
	WeakPyPtr( const This& P )
	{
		weakref_ = P.weakref_;
		if (this->good()) Py_INCREF( weakref_ );
	}

	/**
	 *	The assignment operator.
	 */
	This & operator=( const This& X )
	{
		if (weakref_ != X.weakref_)
		{
			// ok to decref before incref here since a weakref object
			// disappearing will never have side effects (its a _weak_ ref!)
			if (this->good())
			{
				Py_DECREF( weakref_ );
			}
			weakref_ = X.weakref_;
			if (this->good())
			{
				Py_INCREF( weakref_ );
			}
		}
		return *this;
	}

	/**
	 *	Destructor.
	 */
	~WeakPyPtr()
	{
		if (this->good()){ Py_DECREF( weakref_ );}
		weakref_ = NULL;
	}

	/**
	 *	This method returns the object that this pointer points to.
	 */
	const PyObject * getPyObj() const
	{
		if (uintptr(weakref_) > 1)
		{
			// 'good' has been inlined here
			PyObject * P = PyWeakref_GET_OBJECT( weakref_ );
			if (P != Py_None) return (const Object*)P;

			weakref_ = (PyObject*)1;
		}

		return NULL;
	}
	const Object * getObject() const { return (const Object*)this->getPyObj(); }
	Object * getObject()			 { return (Object*)this->getPyObj(); }

	/**
	 *	This method returns whether or not this WeakPyPtr has any object.
	 *	(Even if it has since disappeared or is not weakly referencable)
	 *	Not sure whether or not to put this functionality into another fn.
	 */
	bool hasObject() const
	{
		return weakref_ != 0;
	}

	/**
	 *	This method returns whether or not this WeakPyPtr has a good object.
	 */
	bool exists() const
	{
		return this->good();
	}

	/**
	 *	This method returns whether or not this WeakPyPtr has a good object.
	 *	TODO: use a callback instead of checking every time here!
	 */
	bool good() const
	{
		if (uintptr(weakref_) <= 1) return false;
		if (PyWeakref_GET_OBJECT( weakref_ ) != Py_None) return true;
		// ok it just went bad ... write it down then
		weakref_ = (PyObject*)1;
		return false;
	}

	/**
	 *	This method implements the dereference operator. It helps allow this
	 *	object to be used as you would a normal pointer.
	 */
	const Object& operator*() const
	{
		return *this->getObject();
	}
	Object & operator*()	{ return const_cast<Object&>( *this->getObject() );}

	/**
	 *	This method implements the dereference operator. It helps allow this
	 *	object to be used as you would a normal pointer.
	 */
	const Object* operator->() const
	{
		return this->getObject();
	}
	Object * operator->()	{ return const_cast<Object*>( this->getObject() ); }

	/**
	 *	This function returns whether or not the input objects refer to the same
	 *	object.
	 */
	friend bool operator==( const WeakPyPtr<Ty>& A,
		const WeakPyPtr<Ty>& B )
	{
		if (A.weakref_ == B.weakref_) return true;
		return (A.good() && B.good() && &*A == &*B);
	}

	/**
	 *	This function returns not or whether the input objects refer to the same
	 *	object.
	 */
	friend bool operator!=( const WeakPyPtr<Ty>& A,
		const WeakPyPtr<Ty>& B )
	{
		return !(A == B);
	}

	/**
	 *	This function gives an ordering on WeakPyPtrs so that they can be
	 *	placed in sorted containers.
	 */
	friend bool operator<( const WeakPyPtr<Ty>& A,
		const WeakPyPtr<Ty>& B )
	{
		if (A.good() && B.good()) return &*A < &*B;
		return A.weakref_ < B.weakref_;
	}

	/**
	 *	This function gives an ordering on WeakPyPtrs so that they can be
	 *	compared.
	 */
	friend bool operator>( const WeakPyPtr<Ty>& A,
		const WeakPyPtr<Ty>& B )
	{
		if (A.good() && B.good()) return &*A > &*B;
		return A.weakref_ > B.weakref_;
	}

	/**
	 *	This method returns whether or not this pointers points to anything.
	 */
	typedef PyObject * This::*unspecified_bool_type;
	operator unspecified_bool_type() const
	{
		return this->good() ? &This::weakref_ : NULL;
	}

protected:
	mutable PyObject * weakref_;		///< A PyWeakref to the object (1=dead).
};




#ifdef CODE_INLINE
#include "script.ipp"
#endif

#endif // SCRIPT_HPP
