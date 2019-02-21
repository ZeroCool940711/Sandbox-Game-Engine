/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef CELL_VIEWER_SERVER_HPP
#define CELL_VIEWER_SERVER_HPP

#include "network/mercury.hpp"
#include "network/endpoint.hpp"

#include "cell_viewer_connection.hpp"


class Cell;
class CellApp;

/**
 *	An object of this type is responsible for maintaining the instances of
 *	SpaceViewerServer.
 */
class CellViewerServer : public Mercury::InputNotificationHandler
{
public:
	CellViewerServer( const CellApp & cellApp );
	virtual ~CellViewerServer();

	bool startup( Mercury::Nub& nub, uint16 port = 0  );
	void shutDown();
	void deleteConnection( CellViewerConnection* pConnection );
	uint16 port() const;

private:
	virtual	int	handleInputNotification(int fd);

	std::vector<CellViewerConnection*> connections_;

	Endpoint listener_;
	Mercury::Nub* pNub_;
	const CellApp & cellApp_;
};

#endif // VIEWER_CELL_SERVER_HPP
