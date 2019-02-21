/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef PYSCRIPT_PCH_HPP
#define PYSCRIPT_PCH_HPP

#ifdef _WIN32

#include "cstdmf/pch.hpp"

#ifndef MF_SERVER

#include "pickler.hpp"
#include "pyobject_plus.hpp"
#include "pywatcher.hpp"
#include "script.hpp"
#include "script_math.hpp"

#endif // ndef MF_SERVER
#endif // _WIN32

#endif // PYSCRIPT_PCH_HPP
