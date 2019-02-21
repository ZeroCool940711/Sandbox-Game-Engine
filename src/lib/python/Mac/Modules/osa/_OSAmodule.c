
/* ========================== Module _OSA =========================== */

#include "Python.h"



#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
        "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)


#if PY_VERSION_HEX < 0x02040000
PyObject *PyMac_GetOSErrException(void);
#endif
#include <Carbon/Carbon.h>

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_OSAObj_New(ComponentInstance);
extern int _OSAObj_Convert(PyObject *, ComponentInstance *);

#define OSAObj_New _OSAObj_New
#define OSAObj_Convert _OSAObj_Convert
#endif

static PyObject *OSA_Error;

/* ---------------- Object type OSAComponentInstance ---------------- */

PyTypeObject OSAComponentInstance_Type;

#define OSAObj_Check(x) ((x)->ob_type == &OSAComponentInstance_Type || PyObject_TypeCheck((x), &OSAComponentInstance_Type))

typedef struct OSAComponentInstanceObject {
	PyObject_HEAD
	ComponentInstance ob_itself;
} OSAComponentInstanceObject;

PyObject *OSAObj_New(ComponentInstance itself)
{
	OSAComponentInstanceObject *it;
	if (itself == NULL) {
	                                PyErr_SetString(OSA_Error,"NULL ComponentInstance");
	                                return NULL;
	                        }
	it = PyObject_NEW(OSAComponentInstanceObject, &OSAComponentInstance_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}

int OSAObj_Convert(PyObject *v, ComponentInstance *p_itself)
{

	                if (CmpInstObj_Convert(v, p_itself))
	                        return 1;
	                PyErr_Clear();
	                
	if (!OSAObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "OSAComponentInstance required");
		return 0;
	}
	*p_itself = ((OSAComponentInstanceObject *)v)->ob_itself;
	return 1;
}

static void OSAObj_dealloc(OSAComponentInstanceObject *self)
{
	/* Cleanup of self->ob_itself goes here */
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *OSAObj_OSALoad(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	AEDesc scriptData;
	long modeFlags;
	OSAID resultingScriptID;
#ifndef OSALoad
	PyMac_PRECHECK(OSALoad);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      AEDesc_Convert, &scriptData,
	                      &modeFlags))
		return NULL;
	_err = OSALoad(_self->ob_itself,
	               &scriptData,
	               modeFlags,
	               &resultingScriptID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     resultingScriptID);
	return _res;
}

static PyObject *OSAObj_OSAStore(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSAID scriptID;
	DescType desiredType;
	long modeFlags;
	AEDesc resultingScriptData;
#ifndef OSAStore
	PyMac_PRECHECK(OSAStore);
#endif
	if (!PyArg_ParseTuple(_args, "lO&l",
	                      &scriptID,
	                      PyMac_GetOSType, &desiredType,
	                      &modeFlags))
		return NULL;
	_err = OSAStore(_self->ob_itself,
	                scriptID,
	                desiredType,
	                modeFlags,
	                &resultingScriptData);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &resultingScriptData);
	return _res;
}

static PyObject *OSAObj_OSAExecute(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSAID compiledScriptID;
	OSAID contextID;
	long modeFlags;
	OSAID resultingScriptValueID;
#ifndef OSAExecute
	PyMac_PRECHECK(OSAExecute);
#endif
	if (!PyArg_ParseTuple(_args, "lll",
	                      &compiledScriptID,
	                      &contextID,
	                      &modeFlags))
		return NULL;
	_err = OSAExecute(_self->ob_itself,
	                  compiledScriptID,
	                  contextID,
	                  modeFlags,
	                  &resultingScriptValueID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     resultingScriptValueID);
	return _res;
}

static PyObject *OSAObj_OSADisplay(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSAID scriptValueID;
	DescType desiredType;
	long modeFlags;
	AEDesc resultingText;
#ifndef OSADisplay
	PyMac_PRECHECK(OSADisplay);
#endif
	if (!PyArg_ParseTuple(_args, "lO&l",
	                      &scriptValueID,
	                      PyMac_GetOSType, &desiredType,
	                      &modeFlags))
		return NULL;
	_err = OSADisplay(_self->ob_itself,
	                  scriptValueID,
	                  desiredType,
	                  modeFlags,
	                  &resultingText);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &resultingText);
	return _res;
}

static PyObject *OSAObj_OSAScriptError(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSType selector;
	DescType desiredType;
	AEDesc resultingErrorDescription;
#ifndef OSAScriptError
	PyMac_PRECHECK(OSAScriptError);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &selector,
	                      PyMac_GetOSType, &desiredType))
		return NULL;
	_err = OSAScriptError(_self->ob_itself,
	                      selector,
	                      desiredType,
	                      &resultingErrorDescription);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &resultingErrorDescription);
	return _res;
}

static PyObject *OSAObj_OSADispose(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSAID scriptID;
#ifndef OSADispose
	PyMac_PRECHECK(OSADispose);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &scriptID))
		return NULL;
	_err = OSADispose(_self->ob_itself,
	                  scriptID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *OSAObj_OSASetScriptInfo(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSAID scriptID;
	OSType selector;
	long value;
#ifndef OSASetScriptInfo
	PyMac_PRECHECK(OSASetScriptInfo);
#endif
	if (!PyArg_ParseTuple(_args, "lO&l",
	                      &scriptID,
	                      PyMac_GetOSType, &selector,
	                      &value))
		return NULL;
	_err = OSASetScriptInfo(_self->ob_itself,
	                        scriptID,
	                        selector,
	                        value);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *OSAObj_OSAGetScriptInfo(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSAID scriptID;
	OSType selector;
	long result;
#ifndef OSAGetScriptInfo
	PyMac_PRECHECK(OSAGetScriptInfo);
#endif
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &scriptID,
	                      PyMac_GetOSType, &selector))
		return NULL;
	_err = OSAGetScriptInfo(_self->ob_itself,
	                        scriptID,
	                        selector,
	                        &result);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     result);
	return _res;
}

static PyObject *OSAObj_OSAScriptingComponentName(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	AEDesc resultingScriptingComponentName;
#ifndef OSAScriptingComponentName
	PyMac_PRECHECK(OSAScriptingComponentName);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = OSAScriptingComponentName(_self->ob_itself,
	                                 &resultingScriptingComponentName);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &resultingScriptingComponentName);
	return _res;
}

static PyObject *OSAObj_OSACompile(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	AEDesc sourceData;
	long modeFlags;
	OSAID previousAndResultingScriptID;
#ifndef OSACompile
	PyMac_PRECHECK(OSACompile);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      AEDesc_Convert, &sourceData,
	                      &modeFlags))
		return NULL;
	_err = OSACompile(_self->ob_itself,
	                  &sourceData,
	                  modeFlags,
	                  &previousAndResultingScriptID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     previousAndResultingScriptID);
	return _res;
}

static PyObject *OSAObj_OSACopyID(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSAID fromID;
	OSAID toID;
#ifndef OSACopyID
	PyMac_PRECHECK(OSACopyID);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &fromID))
		return NULL;
	_err = OSACopyID(_self->ob_itself,
	                 fromID,
	                 &toID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     toID);
	return _res;
}

static PyObject *OSAObj_OSAGetSource(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSAID scriptID;
	DescType desiredType;
	AEDesc resultingSourceData;
#ifndef OSAGetSource
	PyMac_PRECHECK(OSAGetSource);
#endif
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &scriptID,
	                      PyMac_GetOSType, &desiredType))
		return NULL;
	_err = OSAGetSource(_self->ob_itself,
	                    scriptID,
	                    desiredType,
	                    &resultingSourceData);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &resultingSourceData);
	return _res;
}

static PyObject *OSAObj_OSACoerceFromDesc(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	AEDesc scriptData;
	long modeFlags;
	OSAID resultingScriptID;
#ifndef OSACoerceFromDesc
	PyMac_PRECHECK(OSACoerceFromDesc);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      AEDesc_Convert, &scriptData,
	                      &modeFlags))
		return NULL;
	_err = OSACoerceFromDesc(_self->ob_itself,
	                         &scriptData,
	                         modeFlags,
	                         &resultingScriptID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     resultingScriptID);
	return _res;
}

static PyObject *OSAObj_OSACoerceToDesc(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSAID scriptID;
	DescType desiredType;
	long modeFlags;
	AEDesc result;
#ifndef OSACoerceToDesc
	PyMac_PRECHECK(OSACoerceToDesc);
#endif
	if (!PyArg_ParseTuple(_args, "lO&l",
	                      &scriptID,
	                      PyMac_GetOSType, &desiredType,
	                      &modeFlags))
		return NULL;
	_err = OSACoerceToDesc(_self->ob_itself,
	                       scriptID,
	                       desiredType,
	                       modeFlags,
	                       &result);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &result);
	return _res;
}

static PyObject *OSAObj_OSASetDefaultTarget(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	AEAddressDesc target;
#ifndef OSASetDefaultTarget
	PyMac_PRECHECK(OSASetDefaultTarget);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      AEDesc_Convert, &target))
		return NULL;
	_err = OSASetDefaultTarget(_self->ob_itself,
	                           &target);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *OSAObj_OSAStartRecording(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSAID compiledScriptToModifyID;
#ifndef OSAStartRecording
	PyMac_PRECHECK(OSAStartRecording);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = OSAStartRecording(_self->ob_itself,
	                         &compiledScriptToModifyID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     compiledScriptToModifyID);
	return _res;
}

static PyObject *OSAObj_OSAStopRecording(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSAID compiledScriptID;
#ifndef OSAStopRecording
	PyMac_PRECHECK(OSAStopRecording);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &compiledScriptID))
		return NULL;
	_err = OSAStopRecording(_self->ob_itself,
	                        compiledScriptID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *OSAObj_OSALoadExecute(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	AEDesc scriptData;
	OSAID contextID;
	long modeFlags;
	OSAID resultingScriptValueID;
#ifndef OSALoadExecute
	PyMac_PRECHECK(OSALoadExecute);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      AEDesc_Convert, &scriptData,
	                      &contextID,
	                      &modeFlags))
		return NULL;
	_err = OSALoadExecute(_self->ob_itself,
	                      &scriptData,
	                      contextID,
	                      modeFlags,
	                      &resultingScriptValueID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     resultingScriptValueID);
	return _res;
}

static PyObject *OSAObj_OSACompileExecute(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	AEDesc sourceData;
	OSAID contextID;
	long modeFlags;
	OSAID resultingScriptValueID;
#ifndef OSACompileExecute
	PyMac_PRECHECK(OSACompileExecute);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      AEDesc_Convert, &sourceData,
	                      &contextID,
	                      &modeFlags))
		return NULL;
	_err = OSACompileExecute(_self->ob_itself,
	                         &sourceData,
	                         contextID,
	                         modeFlags,
	                         &resultingScriptValueID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     resultingScriptValueID);
	return _res;
}

static PyObject *OSAObj_OSADoScript(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	AEDesc sourceData;
	OSAID contextID;
	DescType desiredType;
	long modeFlags;
	AEDesc resultingText;
#ifndef OSADoScript
	PyMac_PRECHECK(OSADoScript);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&l",
	                      AEDesc_Convert, &sourceData,
	                      &contextID,
	                      PyMac_GetOSType, &desiredType,
	                      &modeFlags))
		return NULL;
	_err = OSADoScript(_self->ob_itself,
	                   &sourceData,
	                   contextID,
	                   desiredType,
	                   modeFlags,
	                   &resultingText);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &resultingText);
	return _res;
}

static PyObject *OSAObj_OSASetCurrentDialect(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	short dialectCode;
#ifndef OSASetCurrentDialect
	PyMac_PRECHECK(OSASetCurrentDialect);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &dialectCode))
		return NULL;
	_err = OSASetCurrentDialect(_self->ob_itself,
	                            dialectCode);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *OSAObj_OSAGetCurrentDialect(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	short resultingDialectCode;
#ifndef OSAGetCurrentDialect
	PyMac_PRECHECK(OSAGetCurrentDialect);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = OSAGetCurrentDialect(_self->ob_itself,
	                            &resultingDialectCode);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     resultingDialectCode);
	return _res;
}

static PyObject *OSAObj_OSAAvailableDialects(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	AEDesc resultingDialectInfoList;
#ifndef OSAAvailableDialects
	PyMac_PRECHECK(OSAAvailableDialects);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = OSAAvailableDialects(_self->ob_itself,
	                            &resultingDialectInfoList);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &resultingDialectInfoList);
	return _res;
}

static PyObject *OSAObj_OSAGetDialectInfo(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	short dialectCode;
	OSType selector;
	AEDesc resultingDialectInfo;
#ifndef OSAGetDialectInfo
	PyMac_PRECHECK(OSAGetDialectInfo);
#endif
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &dialectCode,
	                      PyMac_GetOSType, &selector))
		return NULL;
	_err = OSAGetDialectInfo(_self->ob_itself,
	                         dialectCode,
	                         selector,
	                         &resultingDialectInfo);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &resultingDialectInfo);
	return _res;
}

static PyObject *OSAObj_OSAAvailableDialectCodeList(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	AEDesc resultingDialectCodeList;
#ifndef OSAAvailableDialectCodeList
	PyMac_PRECHECK(OSAAvailableDialectCodeList);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = OSAAvailableDialectCodeList(_self->ob_itself,
	                                   &resultingDialectCodeList);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &resultingDialectCodeList);
	return _res;
}

static PyObject *OSAObj_OSAExecuteEvent(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	AppleEvent theAppleEvent;
	OSAID contextID;
	long modeFlags;
	OSAID resultingScriptValueID;
#ifndef OSAExecuteEvent
	PyMac_PRECHECK(OSAExecuteEvent);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      AEDesc_Convert, &theAppleEvent,
	                      &contextID,
	                      &modeFlags))
		return NULL;
	_err = OSAExecuteEvent(_self->ob_itself,
	                       &theAppleEvent,
	                       contextID,
	                       modeFlags,
	                       &resultingScriptValueID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     resultingScriptValueID);
	return _res;
}

static PyObject *OSAObj_OSADoEvent(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	AppleEvent theAppleEvent;
	OSAID contextID;
	long modeFlags;
	AppleEvent reply;
#ifndef OSADoEvent
	PyMac_PRECHECK(OSADoEvent);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      AEDesc_Convert, &theAppleEvent,
	                      &contextID,
	                      &modeFlags))
		return NULL;
	_err = OSADoEvent(_self->ob_itself,
	                  &theAppleEvent,
	                  contextID,
	                  modeFlags,
	                  &reply);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &reply);
	return _res;
}

static PyObject *OSAObj_OSAMakeContext(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	AEDesc contextName;
	OSAID parentContext;
	OSAID resultingContextID;
#ifndef OSAMakeContext
	PyMac_PRECHECK(OSAMakeContext);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      AEDesc_Convert, &contextName,
	                      &parentContext))
		return NULL;
	_err = OSAMakeContext(_self->ob_itself,
	                      &contextName,
	                      parentContext,
	                      &resultingContextID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     resultingContextID);
	return _res;
}

#ifdef HAVE_OSA_DEBUG
static PyObject *OSAObj_OSADebuggerCreateSession(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSAID inScript;
	OSAID inContext;
	OSADebugSessionRef outSession;
#ifndef OSADebuggerCreateSession
	PyMac_PRECHECK(OSADebuggerCreateSession);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &inScript,
	                      &inContext))
		return NULL;
	_err = OSADebuggerCreateSession(_self->ob_itself,
	                                inScript,
	                                inContext,
	                                &outSession);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outSession);
	return _res;
}

static PyObject *OSAObj_OSADebuggerGetSessionState(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSADebugSessionRef inSession;
	AERecord outState;
#ifndef OSADebuggerGetSessionState
	PyMac_PRECHECK(OSADebuggerGetSessionState);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inSession))
		return NULL;
	_err = OSADebuggerGetSessionState(_self->ob_itself,
	                                  inSession,
	                                  &outState);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &outState);
	return _res;
}

static PyObject *OSAObj_OSADebuggerSessionStep(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSADebugSessionRef inSession;
	OSADebugStepKind inKind;
#ifndef OSADebuggerSessionStep
	PyMac_PRECHECK(OSADebuggerSessionStep);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &inSession,
	                      &inKind))
		return NULL;
	_err = OSADebuggerSessionStep(_self->ob_itself,
	                              inSession,
	                              inKind);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *OSAObj_OSADebuggerDisposeSession(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSADebugSessionRef inSession;
#ifndef OSADebuggerDisposeSession
	PyMac_PRECHECK(OSADebuggerDisposeSession);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inSession))
		return NULL;
	_err = OSADebuggerDisposeSession(_self->ob_itself,
	                                 inSession);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *OSAObj_OSADebuggerGetStatementRanges(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSADebugSessionRef inSession;
	AEDescList outStatementRangeArray;
#ifndef OSADebuggerGetStatementRanges
	PyMac_PRECHECK(OSADebuggerGetStatementRanges);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inSession))
		return NULL;
	_err = OSADebuggerGetStatementRanges(_self->ob_itself,
	                                     inSession,
	                                     &outStatementRangeArray);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &outStatementRangeArray);
	return _res;
}

static PyObject *OSAObj_OSADebuggerGetBreakpoint(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSADebugSessionRef inSession;
	UInt32 inSrcOffset;
	OSAID outBreakpoint;
#ifndef OSADebuggerGetBreakpoint
	PyMac_PRECHECK(OSADebuggerGetBreakpoint);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &inSession,
	                      &inSrcOffset))
		return NULL;
	_err = OSADebuggerGetBreakpoint(_self->ob_itself,
	                                inSession,
	                                inSrcOffset,
	                                &outBreakpoint);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outBreakpoint);
	return _res;
}

static PyObject *OSAObj_OSADebuggerSetBreakpoint(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSADebugSessionRef inSession;
	UInt32 inSrcOffset;
	OSAID inBreakpoint;
#ifndef OSADebuggerSetBreakpoint
	PyMac_PRECHECK(OSADebuggerSetBreakpoint);
#endif
	if (!PyArg_ParseTuple(_args, "lll",
	                      &inSession,
	                      &inSrcOffset,
	                      &inBreakpoint))
		return NULL;
	_err = OSADebuggerSetBreakpoint(_self->ob_itself,
	                                inSession,
	                                inSrcOffset,
	                                inBreakpoint);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *OSAObj_OSADebuggerGetDefaultBreakpoint(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSADebugSessionRef inSession;
	OSAID outBreakpoint;
#ifndef OSADebuggerGetDefaultBreakpoint
	PyMac_PRECHECK(OSADebuggerGetDefaultBreakpoint);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inSession))
		return NULL;
	_err = OSADebuggerGetDefaultBreakpoint(_self->ob_itself,
	                                       inSession,
	                                       &outBreakpoint);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outBreakpoint);
	return _res;
}

static PyObject *OSAObj_OSADebuggerGetCurrentCallFrame(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSADebugSessionRef inSession;
	OSADebugCallFrameRef outCallFrame;
#ifndef OSADebuggerGetCurrentCallFrame
	PyMac_PRECHECK(OSADebuggerGetCurrentCallFrame);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inSession))
		return NULL;
	_err = OSADebuggerGetCurrentCallFrame(_self->ob_itself,
	                                      inSession,
	                                      &outCallFrame);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outCallFrame);
	return _res;
}

static PyObject *OSAObj_OSADebuggerGetCallFrameState(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSADebugCallFrameRef inCallFrame;
	AERecord outState;
#ifndef OSADebuggerGetCallFrameState
	PyMac_PRECHECK(OSADebuggerGetCallFrameState);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inCallFrame))
		return NULL;
	_err = OSADebuggerGetCallFrameState(_self->ob_itself,
	                                    inCallFrame,
	                                    &outState);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &outState);
	return _res;
}

static PyObject *OSAObj_OSADebuggerGetVariable(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSADebugCallFrameRef inCallFrame;
	AEDesc inVariableName;
	OSAID outVariable;
#ifndef OSADebuggerGetVariable
	PyMac_PRECHECK(OSADebuggerGetVariable);
#endif
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &inCallFrame,
	                      AEDesc_Convert, &inVariableName))
		return NULL;
	_err = OSADebuggerGetVariable(_self->ob_itself,
	                              inCallFrame,
	                              &inVariableName,
	                              &outVariable);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outVariable);
	return _res;
}

static PyObject *OSAObj_OSADebuggerSetVariable(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSADebugCallFrameRef inCallFrame;
	AEDesc inVariableName;
	OSAID inVariable;
#ifndef OSADebuggerSetVariable
	PyMac_PRECHECK(OSADebuggerSetVariable);
#endif
	if (!PyArg_ParseTuple(_args, "lO&l",
	                      &inCallFrame,
	                      AEDesc_Convert, &inVariableName,
	                      &inVariable))
		return NULL;
	_err = OSADebuggerSetVariable(_self->ob_itself,
	                              inCallFrame,
	                              &inVariableName,
	                              inVariable);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *OSAObj_OSADebuggerGetPreviousCallFrame(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSADebugCallFrameRef inCurrentFrame;
	OSADebugCallFrameRef outPrevFrame;
#ifndef OSADebuggerGetPreviousCallFrame
	PyMac_PRECHECK(OSADebuggerGetPreviousCallFrame);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inCurrentFrame))
		return NULL;
	_err = OSADebuggerGetPreviousCallFrame(_self->ob_itself,
	                                       inCurrentFrame,
	                                       &outPrevFrame);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outPrevFrame);
	return _res;
}

static PyObject *OSAObj_OSADebuggerDisposeCallFrame(OSAComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSAError _err;
	OSADebugCallFrameRef inCallFrame;
#ifndef OSADebuggerDisposeCallFrame
	PyMac_PRECHECK(OSADebuggerDisposeCallFrame);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inCallFrame))
		return NULL;
	_err = OSADebuggerDisposeCallFrame(_self->ob_itself,
	                                   inCallFrame);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#endif /* HAVE_OSA_DEBUG */

static PyMethodDef OSAObj_methods[] = {
	{"OSALoad", (PyCFunction)OSAObj_OSALoad, 1,
	 PyDoc_STR("(AEDesc scriptData, long modeFlags) -> (OSAID resultingScriptID)")},
	{"OSAStore", (PyCFunction)OSAObj_OSAStore, 1,
	 PyDoc_STR("(OSAID scriptID, DescType desiredType, long modeFlags) -> (AEDesc resultingScriptData)")},
	{"OSAExecute", (PyCFunction)OSAObj_OSAExecute, 1,
	 PyDoc_STR("(OSAID compiledScriptID, OSAID contextID, long modeFlags) -> (OSAID resultingScriptValueID)")},
	{"OSADisplay", (PyCFunction)OSAObj_OSADisplay, 1,
	 PyDoc_STR("(OSAID scriptValueID, DescType desiredType, long modeFlags) -> (AEDesc resultingText)")},
	{"OSAScriptError", (PyCFunction)OSAObj_OSAScriptError, 1,
	 PyDoc_STR("(OSType selector, DescType desiredType) -> (AEDesc resultingErrorDescription)")},
	{"OSADispose", (PyCFunction)OSAObj_OSADispose, 1,
	 PyDoc_STR("(OSAID scriptID) -> None")},
	{"OSASetScriptInfo", (PyCFunction)OSAObj_OSASetScriptInfo, 1,
	 PyDoc_STR("(OSAID scriptID, OSType selector, long value) -> None")},
	{"OSAGetScriptInfo", (PyCFunction)OSAObj_OSAGetScriptInfo, 1,
	 PyDoc_STR("(OSAID scriptID, OSType selector) -> (long result)")},
	{"OSAScriptingComponentName", (PyCFunction)OSAObj_OSAScriptingComponentName, 1,
	 PyDoc_STR("() -> (AEDesc resultingScriptingComponentName)")},
	{"OSACompile", (PyCFunction)OSAObj_OSACompile, 1,
	 PyDoc_STR("(AEDesc sourceData, long modeFlags) -> (OSAID previousAndResultingScriptID)")},
	{"OSACopyID", (PyCFunction)OSAObj_OSACopyID, 1,
	 PyDoc_STR("(OSAID fromID) -> (OSAID toID)")},
	{"OSAGetSource", (PyCFunction)OSAObj_OSAGetSource, 1,
	 PyDoc_STR("(OSAID scriptID, DescType desiredType) -> (AEDesc resultingSourceData)")},
	{"OSACoerceFromDesc", (PyCFunction)OSAObj_OSACoerceFromDesc, 1,
	 PyDoc_STR("(AEDesc scriptData, long modeFlags) -> (OSAID resultingScriptID)")},
	{"OSACoerceToDesc", (PyCFunction)OSAObj_OSACoerceToDesc, 1,
	 PyDoc_STR("(OSAID scriptID, DescType desiredType, long modeFlags) -> (AEDesc result)")},
	{"OSASetDefaultTarget", (PyCFunction)OSAObj_OSASetDefaultTarget, 1,
	 PyDoc_STR("(AEAddressDesc target) -> None")},
	{"OSAStartRecording", (PyCFunction)OSAObj_OSAStartRecording, 1,
	 PyDoc_STR("() -> (OSAID compiledScriptToModifyID)")},
	{"OSAStopRecording", (PyCFunction)OSAObj_OSAStopRecording, 1,
	 PyDoc_STR("(OSAID compiledScriptID) -> None")},
	{"OSALoadExecute", (PyCFunction)OSAObj_OSALoadExecute, 1,
	 PyDoc_STR("(AEDesc scriptData, OSAID contextID, long modeFlags) -> (OSAID resultingScriptValueID)")},
	{"OSACompileExecute", (PyCFunction)OSAObj_OSACompileExecute, 1,
	 PyDoc_STR("(AEDesc sourceData, OSAID contextID, long modeFlags) -> (OSAID resultingScriptValueID)")},
	{"OSADoScript", (PyCFunction)OSAObj_OSADoScript, 1,
	 PyDoc_STR("(AEDesc sourceData, OSAID contextID, DescType desiredType, long modeFlags) -> (AEDesc resultingText)")},
	{"OSASetCurrentDialect", (PyCFunction)OSAObj_OSASetCurrentDialect, 1,
	 PyDoc_STR("(short dialectCode) -> None")},
	{"OSAGetCurrentDialect", (PyCFunction)OSAObj_OSAGetCurrentDialect, 1,
	 PyDoc_STR("() -> (short resultingDialectCode)")},
	{"OSAAvailableDialects", (PyCFunction)OSAObj_OSAAvailableDialects, 1,
	 PyDoc_STR("() -> (AEDesc resultingDialectInfoList)")},
	{"OSAGetDialectInfo", (PyCFunction)OSAObj_OSAGetDialectInfo, 1,
	 PyDoc_STR("(short dialectCode, OSType selector) -> (AEDesc resultingDialectInfo)")},
	{"OSAAvailableDialectCodeList", (PyCFunction)OSAObj_OSAAvailableDialectCodeList, 1,
	 PyDoc_STR("() -> (AEDesc resultingDialectCodeList)")},
	{"OSAExecuteEvent", (PyCFunction)OSAObj_OSAExecuteEvent, 1,
	 PyDoc_STR("(AppleEvent theAppleEvent, OSAID contextID, long modeFlags) -> (OSAID resultingScriptValueID)")},
	{"OSADoEvent", (PyCFunction)OSAObj_OSADoEvent, 1,
	 PyDoc_STR("(AppleEvent theAppleEvent, OSAID contextID, long modeFlags) -> (AppleEvent reply)")},
	{"OSAMakeContext", (PyCFunction)OSAObj_OSAMakeContext, 1,
	 PyDoc_STR("(AEDesc contextName, OSAID parentContext) -> (OSAID resultingContextID)")},
#ifdef HAVE_OSA_DEBUG
	{"OSADebuggerCreateSession", (PyCFunction)OSAObj_OSADebuggerCreateSession, 1,
	 PyDoc_STR("(OSAID inScript, OSAID inContext) -> (OSADebugSessionRef outSession)")},
	{"OSADebuggerGetSessionState", (PyCFunction)OSAObj_OSADebuggerGetSessionState, 1,
	 PyDoc_STR("(OSADebugSessionRef inSession) -> (AERecord outState)")},
	{"OSADebuggerSessionStep", (PyCFunction)OSAObj_OSADebuggerSessionStep, 1,
	 PyDoc_STR("(OSADebugSessionRef inSession, OSADebugStepKind inKind) -> None")},
	{"OSADebuggerDisposeSession", (PyCFunction)OSAObj_OSADebuggerDisposeSession, 1,
	 PyDoc_STR("(OSADebugSessionRef inSession) -> None")},
	{"OSADebuggerGetStatementRanges", (PyCFunction)OSAObj_OSADebuggerGetStatementRanges, 1,
	 PyDoc_STR("(OSADebugSessionRef inSession) -> (AEDescList outStatementRangeArray)")},
	{"OSADebuggerGetBreakpoint", (PyCFunction)OSAObj_OSADebuggerGetBreakpoint, 1,
	 PyDoc_STR("(OSADebugSessionRef inSession, UInt32 inSrcOffset) -> (OSAID outBreakpoint)")},
	{"OSADebuggerSetBreakpoint", (PyCFunction)OSAObj_OSADebuggerSetBreakpoint, 1,
	 PyDoc_STR("(OSADebugSessionRef inSession, UInt32 inSrcOffset, OSAID inBreakpoint) -> None")},
	{"OSADebuggerGetDefaultBreakpoint", (PyCFunction)OSAObj_OSADebuggerGetDefaultBreakpoint, 1,
	 PyDoc_STR("(OSADebugSessionRef inSession) -> (OSAID outBreakpoint)")},
	{"OSADebuggerGetCurrentCallFrame", (PyCFunction)OSAObj_OSADebuggerGetCurrentCallFrame, 1,
	 PyDoc_STR("(OSADebugSessionRef inSession) -> (OSADebugCallFrameRef outCallFrame)")},
	{"OSADebuggerGetCallFrameState", (PyCFunction)OSAObj_OSADebuggerGetCallFrameState, 1,
	 PyDoc_STR("(OSADebugCallFrameRef inCallFrame) -> (AERecord outState)")},
	{"OSADebuggerGetVariable", (PyCFunction)OSAObj_OSADebuggerGetVariable, 1,
	 PyDoc_STR("(OSADebugCallFrameRef inCallFrame, AEDesc inVariableName) -> (OSAID outVariable)")},
	{"OSADebuggerSetVariable", (PyCFunction)OSAObj_OSADebuggerSetVariable, 1,
	 PyDoc_STR("(OSADebugCallFrameRef inCallFrame, AEDesc inVariableName, OSAID inVariable) -> None")},
	{"OSADebuggerGetPreviousCallFrame", (PyCFunction)OSAObj_OSADebuggerGetPreviousCallFrame, 1,
	 PyDoc_STR("(OSADebugCallFrameRef inCurrentFrame) -> (OSADebugCallFrameRef outPrevFrame)")},
	{"OSADebuggerDisposeCallFrame", (PyCFunction)OSAObj_OSADebuggerDisposeCallFrame, 1,
	 PyDoc_STR("(OSADebugCallFrameRef inCallFrame) -> None")},
#endif /* HAVE_OSA_DEBUG */
	{NULL, NULL, 0}
};

#define OSAObj_getsetlist NULL


#define OSAObj_compare NULL

#define OSAObj_repr NULL

#define OSAObj_hash NULL
#define OSAObj_tp_init 0

#define OSAObj_tp_alloc PyType_GenericAlloc

static PyObject *OSAObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
	PyObject *_self;
	ComponentInstance itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, OSAObj_Convert, &itself)) return NULL;
	if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((OSAComponentInstanceObject *)_self)->ob_itself = itself;
	return _self;
}

#define OSAObj_tp_free PyObject_Del


PyTypeObject OSAComponentInstance_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_OSA.OSAComponentInstance", /*tp_name*/
	sizeof(OSAComponentInstanceObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) OSAObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) OSAObj_compare, /*tp_compare*/
	(reprfunc) OSAObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) OSAObj_hash, /*tp_hash*/
	0, /*tp_call*/
	0, /*tp_str*/
	PyObject_GenericGetAttr, /*tp_getattro*/
	PyObject_GenericSetAttr, /*tp_setattro */
	0, /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
	0, /*tp_doc*/
	0, /*tp_traverse*/
	0, /*tp_clear*/
	0, /*tp_richcompare*/
	0, /*tp_weaklistoffset*/
	0, /*tp_iter*/
	0, /*tp_iternext*/
	OSAObj_methods, /* tp_methods */
	0, /*tp_members*/
	OSAObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	OSAObj_tp_init, /* tp_init */
	OSAObj_tp_alloc, /* tp_alloc */
	OSAObj_tp_new, /* tp_new */
	OSAObj_tp_free, /* tp_free */
};

/* -------------- End object type OSAComponentInstance -------------- */


static PyMethodDef OSA_methods[] = {
	{NULL, NULL, 0}
};




void init_OSA(void)
{
	PyObject *m;
	PyObject *d;



	/*
	        PyMac_INIT_TOOLBOX_OBJECT_NEW(ComponentInstance, OSAObj_New);
	        PyMac_INIT_TOOLBOX_OBJECT_CONVERT(ComponentInstance, OSAObj_Convert);
	*/


	m = Py_InitModule("_OSA", OSA_methods);
	d = PyModule_GetDict(m);
	OSA_Error = PyMac_GetOSErrException();
	if (OSA_Error == NULL ||
	    PyDict_SetItemString(d, "Error", OSA_Error) != 0)
		return;
	OSAComponentInstance_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&OSAComponentInstance_Type) < 0) return;
	Py_INCREF(&OSAComponentInstance_Type);
	PyModule_AddObject(m, "OSAComponentInstance", (PyObject *)&OSAComponentInstance_Type);
	/* Backward-compatible name */
	Py_INCREF(&OSAComponentInstance_Type);
	PyModule_AddObject(m, "OSAComponentInstanceType", (PyObject *)&OSAComponentInstance_Type);
}

/* ======================== End module _OSA ========================= */

