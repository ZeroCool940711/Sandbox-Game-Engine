/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef NETMASK_HPP
#define NETMASK_HPP

/**
 *	This class is used to represent a network mask.
 */
class NetMask
{
public:
	NetMask();

	bool			parse( const char* str );
	bool			containsAddress( unsigned long addr ) const;

	void			clear();

private:
	unsigned long	mask_;
	int				bits_;
};

#endif // NETMASK_HPP
