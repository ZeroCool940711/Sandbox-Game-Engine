/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef PORTMAP_HPP
#define PORTMAP_HPP

/**
 * This port is used by the login server.
 */
#define PORT_LOGIN				20013

/**
 * This port is used by the machined process.
 */
#define PORT_MACHINED_OLD		20014
#define PORT_MACHINED			20018

/**
 * Port used by machined during the initial interface discovery.
 */
#define PORT_BROADCAST_DISCOVERY	20019

/**
 * This port is used by the watcher process.
 */
#define PORT_WATCHER			20017

/**
 * Cell python servers are at PORT_PYTHON_PROXY + proxyid (TCP).
 */
#define PORT_PYTHON_PROXY		40000

/**
 * Cell python servers are at PORT_PYTHON_CELL + cellid (TCP).
 */
#define PORT_PYTHON_CELL		50000

#endif // PORTMAP_HPP
