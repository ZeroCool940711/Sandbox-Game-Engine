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

#include "interface_element.hpp"
#include "bundle.hpp"

#include "cstdmf/config.hpp"

DECLARE_DEBUG_COMPONENT2( "Network", 0 )

namespace Mercury
{

/**
 * 	@internal
 *	This structure contains the standard interface definition for a reply
 *	message. It is used internally by Mercury.
 */
const InterfaceElement InterfaceElement::REPLY( "Reply",
	REPLY_MESSAGE_IDENTIFIER, VARIABLE_LENGTH_MESSAGE, 4 );


const float InterfaceElementWithStats::AVERAGE_BIAS = -2.f / (5 + 1);

// -----------------------------------------------------------------------------
// Section: InterfaceElement
// -----------------------------------------------------------------------------

/**
 * 	This method returns the number of bytes occupied by a header
 * 	for this type of message.
 *
 * 	@return Number of bytes needed for this header.
 */
int InterfaceElement::headerSize() const
{
	int headerLen = sizeof( MessageID );
	switch (lengthStyle_)
	{
		case FIXED_LENGTH_MESSAGE:
			break;
		case VARIABLE_LENGTH_MESSAGE:
			headerLen += lengthParam_;
			break;
		default:
			headerLen = -1;
			break;
	}
	return headerLen;
}

/**
 * 	This method returns the number of bytes nominally occupied by the body
 * 	of this type of message.
 *
 * 	@return Number of bytes.
 */
int InterfaceElement::nominalBodySize() const
{
	// never guesses for variable-length messages
	return (lengthStyle_ == FIXED_LENGTH_MESSAGE) ? lengthParam_ : 0;
}


/**
 *	This class is used as a type of iterator on a bundle. It is used to keep a
 *	pointer to the body part of a bundle and move this pointer forward. This
 *	class will take care of moving through the packets in a bundle.
 */
class BundleDataPos
{
public:
	BundleDataPos( Packet * pPacket, void * pCurr ) :
		pPacket_( pPacket ),
		pCurr_( (uint8*)pCurr )
	{
		MF_ASSERT( pPacket->body() <= pCurr && pCurr < pPacket->back() );
	}

	Packet * pPacket() const { return pPacket_; }
	uint8 * pData() const	{ return pCurr_; }

	bool advance( unsigned int distance )
	{
		if ((pPacket_ == NULL) || (pCurr_ == NULL))
			return false;

		int remainingLength = distance;
		uint8 * pEnd = (uint8*)pPacket_->back();

		while (remainingLength >= (pEnd - pCurr_))
		{
			remainingLength -= (pEnd - pCurr_);
			pPacket_ = pPacket_->next();
			if (pPacket_ == NULL)
			{
				ERROR_MSG( "BundleDataPos::advancedBy: "
								"Ran out of packets.\n" );
				return false;
			}
			pCurr_ = (uint8*)pPacket_->body();
			pEnd = (uint8*)pPacket_->back();
		}
		pCurr_ += remainingLength;

		return true;
	}

private:
	Packet * pPacket_;
	uint8 * pCurr_;
};


/**
 *	This method is called by InterfaceElement::compressLength when the amount of
 *	data added to the stream for the message is more than the message's size
 *	field can handle. For example, if lengthParam is 1 and there is at least
 *	255 bytes worth of data added for the message (or 65535 for 2 bytes etc).
 *
 *	To handle this, a 4-byte size is placed at the start of the message
 *	displacing the first four bytes of the message. These are appended to the
 *	end of the message. The original length field is filled with 0xff to
 *	indicate this special situation.
 */
int InterfaceElement::specialCompressLength( void * data, int length,
		Bundle & bundle ) const
{
	// The message is longer that the length field can handle.
	WARNING_MSG( "Mercury::InterfaceElement::compressLength( %s ): "
		"length %d exceeds maximum of length format %d\n",
		this->c_str(), length, (1 << (lengthParam_*8)) - 1 );

	// Fill the original length field with ones to indicate the special
	// situation.
	const int IDENTIFIER_SIZE = sizeof(uint8);
	for (int i = IDENTIFIER_SIZE; i <= lengthParam_; ++i)
	{
		((uint8*)data)[i] = 0xff;
	}

	// Find the packet that the data pointer is in.
	Packet * pPacket = bundle.firstPacket_.getObject();
	while (pPacket &&
			!(pPacket->body() <= data && data < pPacket->back()))
	{
		pPacket = pPacket->next();
	}

	IF_NOT_MF_ASSERT_DEV (pPacket != NULL)
	{
		ERROR_MSG( "Mercury::InterfaceElement::compressLength( %s ): "
			"data not in any packets\n",
			this->c_str() );

		return -1;
	}

	// Move to the start of the message body.
	BundleDataPos head( pPacket, data );
	head.advance( IDENTIFIER_SIZE + lengthParam_ );

	// Get four bytes to place the displaced start of the message
	uint32 localLength = length;
	uint8 * pTail = (uint8*)bundle.reserve( sizeof( int32 ) );

	// Copy the length to the head of the message and the displaced start of
	// the message to the tail. This needs to be done a byte at a time as the
	// head may span packets.
	for (int i = 0; i < int(sizeof( int32 )); ++i)
	{
		// Copy displaced head of the message to the tail of the message.
		*pTail = *head.pData();

		// Copy the message size to the start of the message
		*head.pData() = (uint8)localLength;
		localLength >>= 8;

		++pTail;

		bool advanceSuccessful = head.advance( 1 );

		IF_NOT_MF_ASSERT_DEV (advanceSuccessful)
		{
			ERROR_MSG( "Mercury::InterfaceElement::compressLength( %s ): "
				"head not in packets.\n",
				this->c_str() );

			return -1;
		}
	}

	return 0;
}


/**
 * 	This method compresses a length into the given header.
 *
 *	@param data		Pointer to the header
 *	@param length	Length to compress
 *
 *	@return 0 if successful.
 */
int InterfaceElement::compressLength( void * data, int length,
		Bundle & bundle ) const
{
	switch (lengthStyle_)
	{
	case FIXED_LENGTH_MESSAGE:
		if (length != lengthParam_)
		{
			CRITICAL_MSG( "Mercury::InterfaceElement::compressLength( %s ): "
				"Fixed length message has wrong length (%d instead of %d)\n",
				this->c_str(), length, lengthParam_ );
		}
		break;

	case VARIABLE_LENGTH_MESSAGE:
	{
		// Beware of overflow in length, not sure if this actually happens in
		// practice, but it definately happens on the expandLength() side of
		// things so it's probably worth having this check here
		if (length < 0)
		{
			ERROR_MSG( "Mercury::InterfaceElement::compressLength( %s ): "
				"Possible overflow in length (%d bytes) for "
				"variable length message\n",
				this->c_str(), length );

			return -1;
		}

		char *pLen = ((char*)data) + 1;
		int len = length;
		bool oversize = false;

		switch (lengthParam_)
		{
			case 1:
				*(uint8*)pLen = (uint8)len;
				if (len >= 0xff) oversize = true;
				break;

			case 2:
				*(uint16*)pLen = BW_HTONS( (uint16)len );
				if (len >= 0xffff) oversize = true;
				break;

			case 3:
				BW_PACK3( pLen, len );
				if (len >= 0xffffff) oversize = true;
				break;

			case 4:
				*(uint32*)pLen = BW_HTONL( (uint32)len );
				break;

			default:
				CRITICAL_MSG( "InterfaceElement::compressLength( %s ): "
					"Unsupported variable length width: %d\n",
					this->c_str(), lengthParam_ );
		}

		// If the message length could not fit into a standard length field, we
		// need to handle this as a special case.
		if (oversize)
		{
			return this->specialCompressLength( data, length, bundle );
		}
		break;
	}
	default:
		ERROR_MSG( "Mercury::InterfaceElement::compressLength( %s ): "
			"Unrecognised length format %d\n",
			this->c_str(), (int)lengthStyle_ );

		return -1;
		break;
	}

	return 0;
}


/**
 *	This method handles the case where a message has had its length field added
 *	by InterfaceElement::specialCompressLength. See that method for more
 *	details.
 */
int InterfaceElement::specialExpandLength( void * data, Packet * pPacket ) const
{
	const int IDENTIFIER_SIZE = sizeof( uint8 );

	WARNING_MSG( "InterfaceElement::expandLength( %s ): "
		"Received a message longer than normal length\n",
		this->c_str() );

	// Read the length out of the first four bytes of the message.
	int len = 0;
	{
		BundleDataPos curr( pPacket, data );
		curr.advance( IDENTIFIER_SIZE + lengthParam_ );
		for (int i = 0; i < (int)sizeof(int32); ++i)
		{
			len |= uint32(*curr.pData()) << 8*i;
			if (!curr.advance( 1 ))
			{
				ERROR_MSG( "InterfaceElement::expandLength( %s ): "
					"Ran out of packets.\n",
					this->c_str() );

				return -1;
			}
		}
	}

	// Now need to move the last four bytes of the message to the first.
	BundleDataPos head( pPacket, data );
	head.advance( IDENTIFIER_SIZE + lengthParam_ );
	BundleDataPos tail = head;

	bool tailAdvanceSuccess = tail.advance( len );
	IF_NOT_MF_ASSERT_DEV (tailAdvanceSuccess)
	{
		ERROR_MSG( "InterfaceElement::expandLength( %s ): "
			"Could not find tail.\n",
			this->c_str() );

		return -1;
	}

	for (int i = (int)sizeof( int32 ) - 1; i >= 0; --i)
	{
		*head.pData() = *tail.pData();

		// Do not advance on the last iteration as it is likely to fail.
		if (i > 0)
		{
			head.advance( 1 );
			tailAdvanceSuccess = tail.advance( 1 );
			IF_NOT_MF_ASSERT_DEV (tailAdvanceSuccess)
			{
				ERROR_MSG( "InterfaceElement::expandLength( %s ): "
					"Ran out of tail.\n",
					this->c_str() );

				return -1;
			}
		}
	}

	return len;
}


/**
 * 	This method expands a length from the given header.
 *
 * 	@param data	This is a pointer to a message header.
 *
 * 	@return Expanded length.
 */
int InterfaceElement::expandLength( void * data, Packet * pPacket ) const
{
	switch(lengthStyle_)
	{
	case FIXED_LENGTH_MESSAGE:
		return lengthParam_;
		break;
	case VARIABLE_LENGTH_MESSAGE:
	{
		uint8 *pLen = ((uint8*)data) + sizeof( MessageID );
		uint32 len = 0;

		switch (lengthParam_)
		{
			case 1: len = *(uint8*)pLen; break;
			case 2: len = BW_NTOHS( *(uint16*)pLen ); break;
			case 3: len = BW_UNPACK3( (const char*)pLen ); break;
			case 4: len = BW_NTOHL( *(uint32*)pLen ); break;
			default:
				CRITICAL_MSG( "InterfaceElement::expandLength( %s ): "
					"Unhandled variable message length: %d\n",
					this->c_str(), lengthParam_ );
		}

		// If lengthParam_ is 4, a length > 0x80000000 will cause an overflow and
		// a negative value will be returned from this method.
		if ((int)len < 0)
		{
			ERROR_MSG( "Mercury::InterfaceElement::expandLength( %s ): "
				"Overflow in calculating length of variable message!\n",
				this->c_str() );

			return -1;
		}

		// The special case is indicated with the length field set to maximum.
		// i.e. All bits set to 1.
		if (!this->canHandleLength( len ))
		{
			return this->specialExpandLength( data, pPacket );
		}

		return len;
		break;
	}
	default:
		ERROR_MSG( "Mercury::InterfaceElement::expandLength( %s ): "
			"unrecognised length format %d\n",
			this->c_str(), (int)lengthStyle_ );

		break;
	}
	return -1;
}


/**
 *  Returns the string representation of this interface element, useful for
 *  debugging.
 */
const char * InterfaceElement::c_str() const
{
	static char buf[ 256 ];
	bw_snprintf( buf, sizeof( buf ), "%s/%d", name_, id_ );
	return buf;
}


// -----------------------------------------------------------------------------
// Section: InterfaceElementWithStats
// -----------------------------------------------------------------------------

#if ENABLE_WATCHERS
/**
 *	This static method returns a generic watcher for this class.
 */
WatcherPtr InterfaceElementWithStats::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		InterfaceElementWithStats * pNULL = NULL;

		pWatcher = new DirectoryWatcher();

		pWatcher->addChild( "name",
				makeWatcher( pNULL->name_ ) );

		pWatcher->addChild( "id",
				makeWatcher( *pNULL, &InterfaceElementWithStats::idAsInt ) );

		pWatcher->addChild( "maxBytesReceived",
				makeWatcher( pNULL->maxBytesReceived_ ) );

		pWatcher->addChild( "bytesReceived",
				makeWatcher( pNULL->numBytesReceived_ ) );

		pWatcher->addChild( "messagesReceived",
				makeWatcher( pNULL->numMessagesReceived_ ) );

		pWatcher->addChild( "avgMessageLength",
				makeWatcher( *pNULL,
					&InterfaceElementWithStats::avgMessageLength ) );

		pWatcher->addChild( "avgBytesPerSecond",
				makeWatcher( *pNULL,
					&InterfaceElementWithStats::avgBytesReceivedPerSecond ) );
		pWatcher->addChild( "avgMessagesPerSecond",
				makeWatcher( *pNULL,
					&InterfaceElementWithStats::
						avgMessagesReceivedPerSecond ) );
	}

	return pWatcher;
}
#endif

} // namespace Mercury

// interface_element.cpp
