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
 *	ListMultiProvider: Inherits from SmartListProvider to implement a list
 *	provider that manages one or more sub-providers, allowing multiple asset
 *	sources to be shown under one UAL folder
 */

#ifndef LIST_MULTI_PROVIDER_HPP
#define LIST_MULTI_PROVIDER_HPP

#include "list_cache.hpp"
#include "filter_spec.hpp"
#include "smart_list_ctrl.hpp"
#include "atlimage.h"


// SmartList file provider
class ListMultiProvider : public ListProvider
{
public:
	ListMultiProvider();
	virtual ~ListMultiProvider();

	virtual void refresh();

	virtual bool finished();

	virtual int getNumItems();

	virtual	const AssetInfo getAssetInfo( int index );
	virtual void getThumbnail( ThumbnailManager& manager,
								int index, CImage& img, int w, int h,
								ThumbnailUpdater* updater );

	virtual void filterItems();

	// additional interface
	void addProvider( ListProviderPtr provider );

private:
	typedef std::vector<ListProviderPtr> ProvVec;
	ProvVec providers_;
	class ListItem
	{
	public:
		ListItem( ListProviderPtr provider, int index );

		ListProviderPtr provider() const { return provider_; }
		int index() const { return index_; }

		const char* text() const;

	private:
		ListProviderPtr provider_;
		int index_;
		mutable std::string text_;
		mutable bool inited_;
	};
	std::vector<ListItem> items_;
	int lastNumItems_;

	static bool s_comparator( const ListItem& a, const ListItem& b );

	void updateItems();
	void fillItems();
};

#endif // LIST_MULTI_PROVIDER_HPP
