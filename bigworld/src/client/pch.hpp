/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef BWCLIENT_PCH_HPP
#define BWCLIENT_PCH_HPP

// identifier was truncated to '255' characters in the browser information
#pragma warning(disable: 4786)

#include "cstdmf/pch.hpp"
#include "resmgr/pch.hpp"
#include <Python.h>

#include <vector>
#include <set>

#include "moo/moo_math.hpp"

#include "..\common\doc_watcher.hpp"

#include "network/basictypes.hpp"
#include "cstdmf/binary_stream.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "duplo/pymodel.hpp"
#include "duplo/py_attachment.hpp"
#include "duplo/chunk_attachment.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#endif // BWCLIENT_PCH_HPP
