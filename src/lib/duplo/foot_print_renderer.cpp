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
#include "foot_print_renderer.hpp"
#include "moo/material.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/graphics_settings.hpp"
#include "romp/custom_mesh.hpp"
#include "resmgr/auto_config.hpp"
#include <deque>

namespace { // anonymous

const int MAX_FOOTPRINTS = 256;

struct FootprintBlock
{
	int spaceID;
	int beginIdx;
	int endIdx;
};

typedef std::deque<FootprintBlock> FootprintBlockVector;
FootprintBlockVector s_footprintBlocks;

const int       c_vertsPerPrint = 4;
Vector3			s_storage[MAX_FOOTPRINTS*c_vertsPerPrint];
bool            s_enabled       = true;
Moo::Material * s_footPrintMat  = NULL;

} // namespace anonymous


DECLARE_DEBUG_COMPONENT2( "duplo", 2 )

// -----------------------------------------------------------------------------
// Section: FootPrints
// -----------------------------------------------------------------------------

/**
 *	This global method specifies the resources required by this file
 */
static AutoConfigString s_mfmName( "environment/footprintMaterial" );

/**
 *	Constructor.
 *
 *	@param	id	id of the space owning this instance.
 */
FootPrintRenderer::FootPrintRenderer(ChunkSpaceID id) :
	spaceID_(id)
{}

/**
 *	Destructor.
 */
FootPrintRenderer::~FootPrintRenderer()
{
}

/**
 *	Initialises the FootPrintRenderer class.
 */
void FootPrintRenderer::init()
{
	BW_GUARD;
	FootprintBlock invalidBlock;
	invalidBlock.spaceID  = -1;
	invalidBlock.beginIdx = 0;
	invalidBlock.endIdx   = MAX_FOOTPRINTS;
	s_footprintBlocks.push_back(invalidBlock);
	
	s_footPrintMat = new Moo::Material();
	s_footPrintMat->load( s_mfmName );

	// foot prints on/off settings
	typedef Moo::GraphicsSetting::GraphicsSettingPtr GraphicsSettingPtr;
	GraphicsSettingPtr footPrintSettings =
		new Moo::StaticCallbackGraphicsSetting(
			"FOOT_PRINTS", "Foot Prints", &FootPrintRenderer::setFootPrintOption,
			-1, false, false);

	footPrintSettings->addOption("ON", "On", true);
	footPrintSettings->addOption("OFF", "Off", true);
	Moo::GraphicsSetting::add(footPrintSettings);
}

/**
 *	Finalizes the FootPrintRenderer class.
 */
void FootPrintRenderer::fini()
{
	BW_GUARD;
	if (s_footPrintMat != NULL)
	{
		delete s_footPrintMat;
		s_footPrintMat = NULL;
	}
}

/**
 *	Draw all foot prints for this space.
 */
void FootPrintRenderer::draw()
{
	BW_GUARD;
	if (!s_enabled)
	{
		return;
	}

	static CustomMesh<Moo::VertexXYZDUV> vxs;	
	vxs.clear();
	
	FootprintBlockVector::const_iterator blockIt  = s_footprintBlocks.begin();
	FootprintBlockVector::const_iterator blockEnd = s_footprintBlocks.end();
	while (blockIt != blockEnd)
	{
		if (blockIt->spaceID == this->spaceID_)
		{
			for (int i=blockIt->beginIdx; i<blockIt->endIdx; i++)
			{
				Vector3 * fp = &s_storage[ c_vertsPerPrint*i ];

				Moo::VertexXYZDUV	quad[c_vertsPerPrint];
				for (int j=0; j < c_vertsPerPrint; j++)
				{
					quad[j].pos_    = fp[j];
					quad[j].colour_ = 0xffffffff;
					quad[j].uv_.set( (j&1)?1.f:0.f, (j&2)?1.f:0.f );
				}

				vxs.push_back( quad[0] );
				vxs.push_back( quad[2] );
				vxs.push_back( quad[1] );

				vxs.push_back( quad[1] );
				vxs.push_back( quad[2] );
				vxs.push_back( quad[3] );
			}
		}
		++blockIt;
	}

	Moo::rc().setPixelShader( NULL );
	s_footPrintMat->set();

	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
	Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );

	vxs.draw();
}

/**
 *	Adds one foot print quad to this space.
 */
void FootPrintRenderer::addFootPrint(const Vector3 * vertices)
{
	BW_GUARD;
	FootprintBlock & backBlock = s_footprintBlocks.back();
	
	int nextIdx = backBlock.endIdx % MAX_FOOTPRINTS;		
	if (nextIdx == 0 || this->spaceID_ != backBlock.spaceID)
	{
		FootprintBlock block;
		block.spaceID  = this->spaceID_;
		block.beginIdx = nextIdx;
		block.endIdx   = nextIdx + 1;
		s_footprintBlocks.push_back(block);
	}
	else
	{
		backBlock.endIdx = nextIdx + 1;
	}

	FootprintBlock & frontBlock = s_footprintBlocks.front();
	++frontBlock.beginIdx;
	if (frontBlock.beginIdx == frontBlock.endIdx)
	{
		s_footprintBlocks.pop_front();
	}

	memcpy(&s_storage[
		c_vertsPerPrint*nextIdx], vertices, 
		c_vertsPerPrint*sizeof(Vector3));
}

/**
 *	Turns foot printing on/off. Implicitly called
 *	whenever the user changes the FOOT_PRINTS setting.
 */
void FootPrintRenderer::setFootPrintOption(int selectedOption)
{
	s_enabled = selectedOption == 0;
}

// foot_print_renderer.cpp
