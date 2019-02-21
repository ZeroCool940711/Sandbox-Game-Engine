var HelpTitle = 
{
	
	timerID: null,	// update timer in case top pane is not loaded
	
	/** 
	 *	Changes the Help title text from a content pane (right_pane). If the
	 *	top pane has not been loaded yet, then the update is deferred for a
	 *	second, repeating until the title element exists. 
	 */
	update : function ( titleText )
	{
		var topFrame = window.parent.document.getElementById( 'top_pane' );
		var titleElt = topFrame.contentDocument.getElementById( 'title' );
		
		// clear the timer, if any
		if (HelpTitle.timerID != null)
		{
			clearTimeout( HelpTitle.timerID );
			HelpTitle.timerID = null;
		}
		
		if (titleElt)
		{
			titleElt.innerHTML = titleText;
		}
		else
		{
			// top pane hasn't loaded yet
			// try again in one second
			HelpTitle.timerID = setTimeout( 
				"HelpTitle.update( '" + titleText + "' );", 1000 );
		}
	}
};
