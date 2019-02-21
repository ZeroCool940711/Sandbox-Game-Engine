/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

/**
 *	XML File Thumbnail Provider (particles, lights, etc)
 */


#ifndef XML_THUMB_PROV_HPP
#define XML_THUMB_PROV_HPP

#include "thumbnail_manager.hpp"


// XML File Provider
class XmlThumbProv : public ThumbnailProvider
{
public:
	XmlThumbProv() {};
	virtual bool isValid( const ThumbnailManager& manager, const std::string& file );
	virtual bool needsCreate( const ThumbnailManager& manager, const std::string& file, std::string& thumb, int& size );
	virtual bool prepare( const ThumbnailManager& manager, const std::string& file );
	virtual bool render( const ThumbnailManager& manager, const std::string& file, Moo::RenderTarget* rt  );

private:
	DECLARE_THUMBNAIL_PROVIDER()

	bool isParticleSystem( const std::string& file );
	bool isLight( const std::string& file );
	std::string particleImageFile();
	std::string lightImageFile();
};

#endif // XML_THUMB_PROV_HPP
