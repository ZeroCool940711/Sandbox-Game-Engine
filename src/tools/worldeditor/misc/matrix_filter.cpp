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
#include "worldeditor/misc/matrix_filter.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/string_provider.hpp"
#include "common/string_utils.hpp"


//---------------------------------------------------------
//  Section: MatrixFilter::FilterDef
//---------------------------------------------------------
MatrixFilter::FilterDef::FilterDef()
{
	clear();
}


void MatrixFilter::FilterDef::clear()
{
	constant_ = 0;
	noise_ = false;
	noiseSizeX_ = 0;
	noiseSizeY_ = 0;
	kernel_.clear();
	kernelWidth_ = 0;
	kernelHeight_ = 0;
	strengthRatio_ = 0;
	kernelSum_ = 0;
	included_ = false;
}



//---------------------------------------------------------
//  Section: MatrixFilter
//---------------------------------------------------------

/**
 *	Constructor
 */
MatrixFilter::MatrixFilter() :
	inited_( false )
{
}


/**
 *	Init the singleton by reading the filter descriptions from the xml file.
 *
 *  @return		true if successful, false otherwise
 */
bool MatrixFilter::init()
{
	DataSectionPtr pSection = BWResource::openSection( "resources/data/filters.xml" );
	if ( !pSection )
	{
		ERROR_MSG( "TerrainHeightFilterFunctor::TerrainHeightFilterFunctor()"
			" - Could not open resources/data/filters.xml\n" );
		return false;
	}

	std::vector<DataSectionPtr>	pSections;
	pSection->openSections( "filter", pSections );

	std::vector<DataSectionPtr>::iterator it = pSections.begin();
	std::vector<DataSectionPtr>::iterator end = pSections.end();

	while( it != end )
	{
		DataSectionPtr pFilter = *it++;

		FilterDef filter;
		filter.included_ = pFilter->readBool( "included", true );
		filter.constant_ = pFilter->readFloat( "constant", 0.f );
		filter.noise_ = pFilter->readBool( "noise", false );
		filter.noiseSizeX_ = pFilter->readInt( "noiseSizeX", 1 );
		filter.noiseSizeY_ = pFilter->readInt( "noiseSizeY", 1 );

		std::vector<DataSectionPtr>	kernels;
		pFilter->openSections( "kernel", kernels );

		filter.name_ = pFilter->readString( "name", L("WORLDEDITOR/GUI/PAGE_TERRAIN_FILTER/UNKNOWN_FILTER") );
		bool error = false;
		float sum = 0;
		if ( filter.noise_ )
		{
			// set a base kernel, to be later overriden by the noise function
			filter.kernelWidth_ = 0;
			filter.kernelHeight_ = 0;
			filter.kernelSum_ = 0;
		}
		else
		{
			filter.kernelWidth_ = 0;
			filter.kernelHeight_ = kernels.size();
			if ( (filter.kernelHeight_ % 2) == 0 || filter.kernelHeight_ < 3 )
			{
				// invalid filter, so skip!!!
				ERROR_MSG( "Terrain Height filter %s has an even height. Must be odd and bigger than 2.\n", filter.name_.c_str() );	
				continue;
			}
			for( std::vector<DataSectionPtr>::iterator y = kernels.begin();
				y != kernels.end(); ++y )
			{
				std::string row = (*y)->asString();
				std::vector<std::string> cols;
				StringUtils::vectorFromString( row, cols, " \t" );
				int width = cols.size();
				if ( (width % 2) == 0 || width < 3 )
				{
					// invalid filter, so skip!!!
					ERROR_MSG( "Terrain Height filter %s has an even width. Must be odd and bigger than 2.\n", filter.name_.c_str() );	
					error = true;
					break;
				}
				if ( !filter.kernelWidth_ )
				{
					filter.kernelWidth_ = width;
				}
				else if ( filter.kernelWidth_ != width )
				{
					// invalid filter, so skip!!!
					ERROR_MSG( "Terrain Height filter %s has rows with different widths\n", filter.name_.c_str() );	
					error = true;
					break;
				}
				for( std::vector<std::string>::iterator x = cols.begin();
					x != cols.end(); ++x )
				{
					float val = float( atof( (*x).c_str() ) );
					sum += val;
					filter.kernel_.push_back( val );
				}
			}
		}

		if ( !error )
		{
			filter.strengthRatio_ = pFilter->readFloat( "strengthRatio" );
			if ( !filter.noise_ )
			{
				std::string kernelSumStr = pFilter->readString( "kernelSum", "auto" );
				StringUtils::toLowerCase( kernelSumStr );
				if ( kernelSumStr == "auto" )
				{
					filter.kernelSum_ = sum;
				}
				else
				{
					filter.kernelSum_ = pFilter->readFloat( "kernelSum" );
				}
				if ( filter.kernelSum_ == 0 )
					filter.kernelSum_ = 0.001f;
			}

			filters_.push_back( filter );
		}
	}
	inited_ = true;
	return true;
}


/**
 *	Static method to return the Singleton instance
 *
 *  @return		instance to the matrix filter object
 */
/*static*/ MatrixFilter& MatrixFilter::instance()
{
	static MatrixFilter s_instance;
	if ( !s_instance.inited_ )
		s_instance.init();
	return s_instance;
}


/**
 *	Returns the number of filters read from the xml file.
 *
 *  @return		number of filters
 */
size_t MatrixFilter::size() const
{
	return filters_.size();
}


/**
 *	Returns a filter by index.
 *
 *  @param index	Index to the filter
 *  @return			Filter at the index-th position
 */
const MatrixFilter::FilterDef& MatrixFilter::filter( size_t index ) const
{
	MF_ASSERT( index < filters_.size() );
	return filters_[ index ];
}
