/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "pch.hpp"

#include "python_adapter.hpp"

#include "pyscript/script.hpp"
#include "resmgr/bwresource.hpp"

#include "afxcmn.h"

DECLARE_DEBUG_COMPONENT2( "PythonAdapter", 0 )

PythonAdapter::PythonAdapter() : 
	pScriptObject_(NULL),
	proActive_(true)
{
	std::string scriptFile = "UIAdapter";

    PyObject * pModule = PyImport_ImportModule(
    		const_cast<char *>( scriptFile.c_str()) );

    if (PyErr_Occurred())
    {
		ERROR_MSG( "PythonAdapter - Failed to init adapter\n" );
        PyErr_Print();
	}

    if (pModule != NULL)
    {
        pScriptObject_ = pModule;
        DEBUG_MSG( "PythonAdapter was loaded correctly\n" );
    }
    else
    {
        ERROR_MSG( "PythonAdapter - Could not get module %s\n", scriptFile.c_str() );
        PyErr_Print();
    }
}


PythonAdapter::~PythonAdapter()
{
	Py_XDECREF( pScriptObject_ );
}

bool PythonAdapter::hasScriptObject() const
{
	return pScriptObject_ != NULL;
}


void PythonAdapter::reloadUIAdapter()
{
	std::string scriptFile = "UIAdapter";

    // get rid of script object
	Py_XDECREF( pScriptObject_ );
	pScriptObject_ = NULL;

	//Check if python has ever loaded the module
	PyObject *modules = PyImport_GetModuleDict();
	PyObject *m;

	// if so, reload it
	if ((m = PyDict_GetItemString(modules,
    		const_cast<char*>( scriptFile.c_str() ))) != NULL &&
	    	PyModule_Check(m))
	{
		pScriptObject_ = PyImport_ReloadModule( m );
	}
	else
	{
		pScriptObject_ = PyImport_ImportModule(
        	const_cast<char*>( scriptFile.c_str() ) );
	}

    if ( pScriptObject_ == NULL )
    {
    	ERROR_MSG( "PythonAdapter - Could not load UIAdapter Module\n" );
        PyErr_Print();
    }
}


bool PythonAdapter::call(const std::string& fnName)
{
	if (!pScriptObject_)
		return false;

	bool handled = false;

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
							const_cast<char*>( fnName.c_str() ) );

	if ( pFunction != NULL )
	{
		handled = Script::call( pFunction,
							PyTuple_New(0),
							"PythonAdapter::call: " );
	}
	else
	{
		ERROR_MSG( "PythonAdapter::call - no script for [%s]\n", fnName.c_str());
		PyErr_Print();
	}

	return handled;
}

bool PythonAdapter::callString( const std::string& fnName, const std::string& param )
{
	if (!pScriptObject_)
		return false;

	bool handled = false;

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
							const_cast<char*>( fnName.c_str() ) );

	if ( pFunction != NULL )
	{
		handled = Script::call( pFunction,
							Py_BuildValue( "(s)", param.c_str() ),
							"PythonAdapter::callString: " );
	}
	else
	{
		ERROR_MSG( "PythonAdapter::callString - no script for [%s]\n", fnName.c_str());
		PyErr_Print();
	}

	return handled;
}

bool PythonAdapter::callString2( const std::string& fnName, const std::string& param1, const std::string& param2 )
{
	if (!pScriptObject_)
		return false;

	bool handled = false;

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
							const_cast<char*>( fnName.c_str() ) );

	if ( pFunction != NULL )
	{
		handled = Script::call( pFunction,
							Py_BuildValue( "(ss)", param1.c_str(), param2.c_str() ),
							"PythonAdapter::callString: " );
	}
	else
	{
		ERROR_MSG( "PythonAdapter::callString - no script for [%s]\n", fnName.c_str());
		PyErr_Print();
	}

	return handled;
}


bool PythonAdapter::ActionScriptExecute(const std::string & actionName)
{
	if (!pScriptObject_)
		return false;

	bool handled = false;

	std::string fnName = actionName + "Execute";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
							const_cast<char*>( fnName.c_str() ) );

	if ( pFunction != NULL )
	{
		handled = Script::call( pFunction,
							PyTuple_New(0),
							"PythonAdapter::ActionScriptExecute: " );
	}
	else
	{
		// use the default handler
		PyObject * pDefault = PyObject_GetAttrString( pScriptObject_,
							"defaultActionExecute" );

		if (pDefault != NULL)
		{
			handled = Script::call(	pDefault,
							Py_BuildValue( "(s)", actionName.c_str() ),
							"UI::defaultActionExecute: " );
		}
		else
		{
			ERROR_MSG( "PythonAdapter::ActionScriptExecute - "
					"No default action handler (defaultActionExecute).\n" );
			PyErr_Print();
		}
	}

	return handled;
}


bool PythonAdapter::ActionScriptUpdate(const std::string & actionName, int & enabled, int & checked)
{
	if ( !pScriptObject_ )
		return false;

	bool handled = false;

	std::string fnName = actionName + "Update";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	PyObject * pResult = NULL;

	if ( pFunction != NULL )
	{
		pResult = Script::ask( pFunction,
				PyTuple_New(0),
				"PythonAdapter::ActionScriptUpdate: " );
	}
	else
	{
		// use the default handler
		PyErr_Clear();
		PyObject * pDefaultFunction =
			PyObject_GetAttrString( pScriptObject_,
				"defaultActionUpdate" );

		if (pDefaultFunction != NULL)
		{
			pResult = Script::ask(
				pDefaultFunction,
				Py_BuildValue( "(s)", actionName.c_str() ),
				"PythonAdapter::ActionScriptUpdate: " );
		}
		else
		{
			PyErr_Clear();
		}
	}

	if (pResult != NULL)
	{
		if (PyArg_ParseTuple( pResult, "ii", &enabled, &checked ))
		{
			handled = true;
		}
		else
		{
			ERROR_MSG( "PythonAdapter::ActionScriptUpdate - "
						"%s did not return a tuple of two integers.\n",
						actionName.c_str() );
			PyErr_Print();
		}

		Py_DECREF( pResult );
	}

	return handled;
}

void PythonAdapter::onSliderAdjust( const std::string& name, int pos, int min, int max )
{
	if ( !proActive_ || !pScriptObject_ )
		return;

	std::string fnName = name + "Adjust";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	if ( pFunction != NULL )
	{
		Script::call( pFunction,
			Py_BuildValue(
				"(fff)",
				(float)pos,
				(float)min,
				(float)max ),
			"PythonAdapter::onSliderAdjust: " );
	}
	else
	{
		PyErr_Clear();
		Script::call(
			PyObject_GetAttrString( pScriptObject_,
				"onSliderAdjust" ),
			Py_BuildValue(
				"(sfff)",
				name.c_str(),
				(float)pos,
				(float)min,
				(float)max ),
			"PythonAdapter::onSliderAdjust: " );
	}
}

void PythonAdapter::sliderUpdate( CSliderCtrl* slider, const std::string& sliderName )
{
	if ( !pScriptObject_ )
		return;

    proActive_ = false;
    
	std::string fnName = sliderName + "Update";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	PyObject * pResult = NULL;

	if ( pFunction != NULL )
	{
		pResult = Script::ask( pFunction,
				PyTuple_New(0),
				"PythonAdapter::sliderUpdate: " );
	}
	else
	{
		PyErr_Clear();
	}

	if (pResult != NULL)
	{
		if (PyFloat_Check( pResult ))
		{
			slider->SetPos((int)PyFloat_AsDouble( pResult ));
		}
		else if (PyInt_Check( pResult ))
		{
			slider->SetPos((int)PyInt_AsLong( pResult ));
		}
		else
		{
			float value;
			float min;
			float max;

			if (PyArg_ParseTuple( pResult, "fff", &value, &min, &max ))
			{
				slider->SetRangeMin((int)min);
				slider->SetRangeMax((int)max);
				slider->SetPos((int)value);
			}
			else
			{
				ERROR_MSG( "PythonAdapter::sliderUpdate - %s did not return a float (or three).\n",
					sliderName.c_str() );
				PyErr_Clear();
			}
		}

		Py_DECREF( pResult );
	}

    proActive_ = true;
}


void PythonAdapter::onListItemSelect( const std::string& name, int index )
{
	if ( proActive_ && pScriptObject_ )
    {
    	std::string fnName = name + "ItemSelect";

      PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
                	const_cast<char*>( fnName.c_str() ) );

        if ( pFunction != NULL )
        {
        	Script::call(
        		pFunction,
                Py_BuildValue( "(i)", index ),
                "PythonAdapter::onListItemSelect: " );
        }
        else
		{
			PyErr_Clear();
    		Script::call(
				PyObject_GetAttrString( pScriptObject_,
					"onListItemSelect" ),
				Py_BuildValue( "(si)", name.c_str(), index ),
				"PythonAdapter::onListItemSelect: " );
        }
    }
}

void PythonAdapter::onListSelectUpdate( CListBox* listBox, const std::string& listBoxName )
{
	if ( !pScriptObject_ )
		return;

    proActive_ = false;
    
	std::string fnName = listBoxName + "Update";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	PyObject * pResult = NULL;

	if ( pFunction != NULL )
	{
		pResult = Script::ask( pFunction,
				PyTuple_New(0),
				"PythonAdapter::onListSelectUpdate: " );
	}
	else
	{
		PyErr_Clear();
	}

	if (pResult != NULL)
	{
		if (PyInt_Check( pResult ))
		{
			if( (int)PyInt_AsLong( pResult ) != -1 )
				listBox->SetCurSel((int)PyInt_AsLong( pResult ));
		}
		else
		{
			ERROR_MSG( "PythonAdapter::onListSelectUpdate - %s did not return a int.\n",
				listBoxName.c_str() );
			PyErr_Clear();
		}

		Py_DECREF( pResult );
	}

    proActive_ = true;
}

void PythonAdapter::contextMenuGetItems( const std::string& type, const std::string path,
	std::map<int,std::string>& items )
{
	if ( !pScriptObject_ )
		return;

	const std::string fnName = "contextMenuGetItems";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	if (pFunction == NULL)
	{
		PyErr_Clear();
		return;
	}

	PyObject * pResult = Script::ask(
		pFunction,
		Py_BuildValue( "(ss)",
		type.c_str(), path.c_str() ),
		"PythonAdapter::contextMenuGetItems: " );

	if ( pResult == NULL )
		return;

	MF_ASSERT(PyList_Check( pResult ));
	int size = PyList_Size( pResult );
	for ( int i = 0; i < size; i++ )
	{
		PyObject * pItem = PyList_GetItem( pResult, i );
		MF_ASSERT(PyTuple_Check( pItem ));
		MF_ASSERT(PyTuple_Size( pItem ) == 2);
		items.insert( std::pair<int,std::string>(
			PyInt_AS_LONG( PyTuple_GetItem( pItem, 0 ) ),
			PyString_AsString( PyTuple_GetItem( pItem, 1 ) ) ) );
	}

	Py_DECREF( pResult );
}

void PythonAdapter::contextMenuHandleResult( const std::string& type, const std::string path, int result )
{
	if ( !pScriptObject_ )
		return;

	const std::string fnName = "contextMenuHandleResult";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	if (pFunction == NULL)
	{
		PyErr_Clear();
		return;
	}

	Script::call(
		pFunction,
		Py_BuildValue( "(ssi)",
		type.c_str(), path.c_str(), result ),
		"PythonAdapter::contextMenuHandleResult: " );
}
