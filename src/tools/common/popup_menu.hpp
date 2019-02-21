/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

/*
 *  Generic popup menu helper class
 */

#ifndef POPUP_MENU_HPP
#define POPUP_MENU_HPP

#include <vector>

class PopupMenu
{
public:
	typedef std::pair<std::string,int> Item;
	typedef std::vector<Item> Items;
	
	static const int Separator = 0;
	static const int StartSubmenu = -1;
	static const int EndSubmenu = -2;

	PopupMenu();
	PopupMenu( Items& items );

	// Menu Item display name and ID:
	// name: empty for separators and exit menu commands, string for items, start with ## to set checked
	// ID values:
	//     0 = separator
	//    -1 = Create a submenu and add subsequent items to it
	//    -2 = Finish submenu, so add subsequent items to the menu existing before -1
	//	  >0 = your ID, which will be returned by doModal if the item is clicked
	void addItem( const std::string& name, int id ) { addItem( items_, name, id ); };

	// adds all items in the Items vector
	void addItems( Items& items ) { addItems( items_, items ); };

	// start/end submenues (can be nested)
	void startSubmenu( const std::string& name ) { addItem( name, StartSubmenu ); };
	void endSubmenu() { addItem( "", EndSubmenu ); };
	// adds a separator
	void addSeparator() { addItem( "", Separator ); };

	// show and execute the menu.
	int doModal( HWND parent );

// STATICS versions that work directly on an Items vector
	// add an item
	static void addItem( Items& in, const std::string& name, int id );

	// adds all items in the Items vector
	static void addItems( Items& in, Items& items );

	// start/end submenues (can be nested)
	static void startSubmenu( Items& in, const std::string& name ) { addItem( in, name, StartSubmenu ); };
	static void endSubmenu( Items& in ) { addItem( in, "", EndSubmenu ); };
	// adds a separator
	static void addSeparator( Items& in ) { addItem( in, "", Separator ); };

private:
	Items items_;
};

#endif // POPUP_MENU_HPP
