/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef SIGNAL_SET_HPP
#define SIGNAL_SET_HPP

#include "cstdmf/debug.hpp"

#include <signal.h>

namespace Signal
{

/**
 * 	This class is a thin wrapper around sigset_t. Has dodgy isClear() feature
 * 	because sigisemptyset() is non-standard function.
 */
class Set
{
public:
	enum InitState
	{
		EMPTY, FULL
	};

	Set( InitState state = EMPTY ) : isClear_( state == EMPTY )
	{
		if (state == EMPTY)
		{
			sigemptyset( &signals_ );
		}
		else
		{
			sigfillset( &signals_ );
		}
	}

	void set( int signal )
	{
		MF_VERIFY( sigaddset( &signals_, signal ) == 0 );
		isClear_ = false;
	}
	void clear()
	{
		MF_VERIFY( sigemptyset( &signals_ ) == 0 );
		isClear_ = true;
	}

	bool isClear() const			{ return isClear_; }
	bool isSet( int signal ) const
	{
		return sigismember( &signals_, signal ) ? true : false;
	}

	operator const sigset_t*() const 	{ return &signals_; }

private:
	sigset_t	signals_;
	bool		isClear_;
};

/**
 * 	This class blocks and unblocks the specified signals when it is
 * 	created and destroyed.
 */
class Blocker
{
public:
	Blocker( const Set& blockSignals )
	{
		MF_VERIFY( pthread_sigmask( SIG_SETMASK, blockSignals, &oldSignals_ )
				== 0 );
	}
	~Blocker()
	{
		MF_VERIFY( pthread_sigmask( SIG_SETMASK, &oldSignals_, NULL ) == 0 );
	}

private:
	sigset_t	oldSignals_;
};

}	// end namespace Signal


#endif // SIGNAL_SET_HPP
