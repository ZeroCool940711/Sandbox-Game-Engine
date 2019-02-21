/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef LOD_BAR_HPP
#define LOD_BAR_HPP

#include "mutant.hpp"

class CLodBar: public CWnd
{
public:
	
	static const int numPatterns = 3;
	
	CLodBar( CWnd* parent );

	virtual ~CLodBar();
	
	float maxRange() { return maxRange_; }

	void setWidth( int w );

	void locked( bool locked ) { locked_ = locked; }
	
	void redraw();
	
// Implementation
protected:
	DECLARE_MESSAGE_MAP()
	
private:
	int mover( int pos );
		
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
	afx_msg void OnPaint();
		
	CWnd* parent_;
		
	int width_, height_;
	float maxRange_;

	float posToDist_;
	float posToDist( int pos ) { return (pos < 0) ? 0 : floorf(0.5f + pos * posToDist_); }
	
	float distToPos_;
	int distToPos( float dist ) { return (int)(0.5f + dist * distToPos_); }

	LODList* lodList_;

	CBitmap lodBmp[numPatterns];
	CBrush* lodPat[numPatterns];

	bool mouseInBar_;
	int mover_;

	bool locked_;
};

#endif // LOD_BAR_HPP