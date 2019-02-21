/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef ENCRYPTION_FILTER_HPP
#define ENCRYPTION_FILTER_HPP

#if !defined(MF_SERVER) //defined( WIN32 ) && !defined( _XBOX360 )
#define USE_OPENSSL
#endif

#include "packet_filter.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/memory_stream.hpp"

#include "openssl/blowfish.h"
#include <string>

class ProfileVal;

namespace Mercury
{

#ifdef USE_OPENSSL

/**
 *  A PacketFilter that uses non-modal Blowfish encryption from OpenSSL.
 */
class EncryptionFilter : public PacketFilter
{
public:
	/// Blowfish is hardcoded to 64-bit blocks.
	static const int BLOCK_SIZE = 64 / 8;

	/// Minimum keylength we will accept for a valid filter (128-bit).
	static const int MIN_KEY_SIZE = 128 / 8;

	typedef std::string Key;

	EncryptionFilter( const Key & key );
	EncryptionFilter( int keySize = MIN_KEY_SIZE );
	~EncryptionFilter();

	virtual Reason send( Nub & nub, const Address & addr, Packet * pPacket );
	virtual Reason recv( Nub & nub, const Address & addr, Packet * pPacket );

	virtual int maxSpareSize();

	const Key & key() const { return key_; }
	const char * readableKey() const;
	bool isGood() const { return isGood_; }

	/// Convenience method to avoid having to do this cast all the time.
	BF_KEY * pBFKey() { return (BF_KEY*)pEncryptionObject_; }

	void encryptStream( MemoryOStream & clearStream,
		BinaryOStream & cipherStream );

	void decryptStream( BinaryIStream & cipherStream,
		BinaryOStream & clearStream );

private:
	int encrypt( const unsigned char * src, unsigned char * dest, int length );
	int decrypt( const unsigned char * src, unsigned char * dest, int length );

	bool initKey();

	Key key_;
	int keySize_;
	bool isGood_;

	// In the stock implementation, this is a pointer to a BF_KEY struct.  We
	// deliberately leave this as a void* to allow people to edit the cpp to use
	// a different encryption algorithm without needing to change this hpp.
	void * pEncryptionObject_;

	static ProfileVal * s_pProfileVal_;
};

#else // USE_OPENSSL

class EncryptionFilter : public PacketFilter
{
public:
	typedef std::string Key;
	const Key key() const { return std::string(""); }
};

#endif // USE_OPENSSL

typedef SmartPointer< EncryptionFilter > EncryptionFilterPtr;

} // namespace Mercury

#endif // ENCRYPTION_FILTER_HPP
