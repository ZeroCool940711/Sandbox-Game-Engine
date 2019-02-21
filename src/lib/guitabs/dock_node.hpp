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
 *	GUI Tearoff panel framework - DockNode class
 */

#ifndef GUITABS_DOCK_NODE_HPP
#define GUITABS_DOCK_NODE_HPP


namespace GUITABS
{


/**
 *  This class is the base class that all node objects inherit from. It
 *  includes default implementation for some methods, giving the a default
 *  behaviour of a tree leaf for classes that don't override these methods.
 */
class DockNode : public ReferenceCount
{
public:
	virtual void setLeftChild( DockNodePtr child );
	virtual void setRightChild( DockNodePtr child );

	virtual DockNodePtr getLeftChild();
	virtual DockNodePtr getRightChild();

	virtual bool isLeaf();
	virtual bool hitTest( int x, int y );
	virtual Orientation getSplitOrientation();

	virtual CWnd* getCWnd() = 0;
	virtual bool isVisible();
	virtual bool isExpanded();
	virtual void setParentWnd( CWnd* parent );
	virtual bool adjustSizeToNode( DockNodePtr newNode, bool nodeIsNew = true );
	virtual void recalcLayout();
	virtual void getPreferredSize( int& w, int& h );

	virtual bool getNodeByWnd( CWnd* ptr, DockNodePtr& childNode, DockNodePtr& parentNode );
	virtual DockNodePtr getNodeByPoint( int x, int y );

	virtual void destroy();

	virtual bool load( DataSectionPtr section, CWnd* parent, int wndID ) = 0;
	virtual bool save( DataSectionPtr section ) = 0;

};


} // namespace

#endif // GUITABS_DOCK_NODE_HPP