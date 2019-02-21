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
 *	UalCallback: Contains all callback/functor related code
 */


#ifndef UAL_CALLBACK_HPP
#define UAL_CALLBACK_HPP

#include "asset_info.hpp"
#include "common/popup_menu.hpp"
#include "cstdmf/bw_functor.hpp"

// Forward
class UalDialog;
class UalManager;


// Callback Functors & Data
class UalItemInfo
{
public:
	// For now, it only accepts initialization when being constructed
	// or using the default assignment operator
	UalItemInfo() :
		dialog_( 0 ),
		x_( 0 ),
		y_( 0 ),
		isFolder_( false ),
		folderExtraData_( 0 ),
		next_( 0 )
	{};
	UalItemInfo(
		UalDialog* dialog,
		AssetInfo assetInfo,
		int x,
		int y,
		bool isFolder = false,
		void* folderData = 0 ) :
		dialog_( dialog ),
		assetInfo_( assetInfo ),
		x_( x ),
		y_( y ),
		isFolder_( isFolder ),
		folderExtraData_( folderData ),
		next_( 0 )
	{};
	~UalItemInfo() { if ( next_ ) delete next_; };

	const UalDialog* dialog() const { return dialog_; };
	const AssetInfo& assetInfo() const { return assetInfo_; };
	int x() const { return x_; };
	int y() const { return y_; };
	bool isFolder() const { return isFolder_; };
	const std::string& type() const { return assetInfo_.type(); };
	const std::string& text() const { return assetInfo_.text(); };
	const std::string& longText() const { return assetInfo_.longText(); };
	const std::string& thumbnail() const { return assetInfo_.thumbnail(); };
	const std::string& description() const { return assetInfo_.description(); };

	UalItemInfo* getNext() const { return next_; };

	UalItemInfo operator=( const UalItemInfo& other )
	{
		dialog_ = other.dialog_;
		assetInfo_ = other.assetInfo_;
		x_ = other.x_;
		y_ = other.y_;
		isFolder_ = other.isFolder_;
		folderExtraData_ = other.folderExtraData_;
		next_ = 0;
		return *this;
	};
	bool operator==( const UalItemInfo& other )
	{
		return dialog_ == other.dialog_ &&
			assetInfo_.equalTo( other.assetInfo_ ) &&
			isFolder_ == other.isFolder_ &&
			folderExtraData_ == other.folderExtraData_;
	};
private:
	friend UalDialog;
	friend UalManager;
	UalDialog* dialog_;
	AssetInfo assetInfo_;
	int x_;
	int y_;
	bool isFolder_;
	void* folderExtraData_;
	UalItemInfo* next_;

	void setNext( UalItemInfo* next ) { next_ = next; };
};


// Typedefs
// types for the right-click callback ( use static methods in PopupMenu to add
// items to the vector )
typedef PopupMenu::Item UalPopupMenuItem;
typedef PopupMenu::Items UalPopupMenuItems;

//	drag&drop and click on an item in the list ( item info )
typedef BWBaseFunctor1< UalItemInfo* > UalItemCallback;

//	right-click on an item in the list ( item info, map to be filled with data )
typedef BWBaseFunctor2< UalItemInfo*, UalPopupMenuItems& > UalStartPopupMenuCallback;
typedef BWBaseFunctor2< UalItemInfo*, int > UalEndPopupMenuCallback;

// send errors to app ( error message string )
typedef BWBaseFunctor1< const std::string& > UalErrorCallback;

// onSetFocus/KillFocus ( UalDialog ptr and focus state )
typedef BWBaseFunctor2< UalDialog*, bool > UalFocusCallback;

// Aliases to the BWFunctor classes.
#define UalCallback0	BWBaseFunctor0
#define UalCallback1	BWBaseFunctor1
#define UalCallback2	BWBaseFunctor2
#define UalFunctor0		BWFunctor0
#define UalFunctor1		BWFunctor1
#define UalFunctor2		BWFunctor2


#endif // UAL_CALLBACK_HPP
