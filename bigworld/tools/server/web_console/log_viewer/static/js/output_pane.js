/**
 * Requires:
 *
 * server_time.js
 */

/* namespace */ var OutputPane =
{
	sInstance: null,

	OutputPane: function( anchorElt, params )
	{
		// DOM stuff
		this.root = DIV( {class: "logviewer"} );
		this.table = TABLE( {class: "outputpane"} );
		this.header = DIV( {class: "heading"}, "Results" );

		this.status = SPAN();
		if ($("mlstatus").value == "False")
		{
			this.statusimg = IMG( {class: "statuspix",
				title: "MessageLogger offline",
				src: "static/images/offline.png"} );
		} else {
			this.statusimg = IMG( {class: "statuspix",
				title: "MessageLogger online",
				src: "static/images/online.png"} );
		}
		this.spinner = IMG( {class: "spinner",
			src: "static/images/throbber.png"} );

		this.stop = IMG( {class: "stop", src: "static/images/stop-disabled.png",
						  title: "Stop loading results", id: "stop"} );

		this.frame = createDOM( "iframe", {} );
		this.results = PRE();

		appendChildNodes( anchorElt, this.root );
		appendChildNodes( this.root, this.table );
		appendChildNodes( this.table,
						  TR( {style: "vertical-align: top"},
							  TD( {}, this.header ),
							  TD( {class: "status"}, this.status ),
							  TD( {width: "90px", align: "right"},
								  this.statusimg, A( {}, this.stop ), this.spinner ) ),
						  TR( {}, TD( {colspan: "3"}, this.frame ) ) );

		// This is the query this output pane is currently operating on
		this.query = null;

		// This is the number of lines of output currently being displayed
		this.recCount = 0;

		// This is the number of lines that have already been displayed for the
		// current query
		this.alreadyDisplayed = 0;

		this.domInitted = false;
		this.width = params.width;
		this.height = params.height;
		this.bufSize = params.bufsize || 10000;
		this.form = params.form;
		this.followOutput = 0;
		this.loading = false;

		// For some reason lots of DOM manipulation doesn't work if you try to do it
		// during the construction of an OutputPane.  We call this init method from
		// the beginning of insert() if it hasn't been called yet.
		this.domInit = function()
		{
			if (!this.domInitted)
			{
				this.window = this.frame.contentWindow;
				this.body = this.frame.contentDocument.body;
				this.body.appendChild( this.results );
				this.domInitted = true;
			}
		}

		// Resize the output pane to fill the predefined dimensions, or the entire
		// window in any unspecified direction
		this.fill = function()
		{
			var opos = elementPosition( this.frame );
			var odim = elementDimensions( this.frame );
			var wdim = getViewportDimensions();

			odim.w = this.width ? this.width : wdim.w - opos.x - 40;
			odim.h = this.height ? this.height : wdim.h - opos.y - 40;
			setElementDimensions( this.frame, odim );
		}

		// Associate this output pane with a Query
		this.setQuery = function( query )
		{
			// Stop current query if it's still in progress
			if (this.query != null && this.query != query)
				this.stopLoading();

			if (this.query != query)
			{
				this.query = query;
				this.alreadyDisplayed = 0;
				this.clear();
			}
		}

		// This method is responsible for correctly setting the state of the
		// text and images that are at the top right of the output pane,
		// i.e. the status text, the stop button, and the spinner.  It should
		// not update any internal state.
		this.displayLoading = function( isLoading )
		{
			// While loading, we display some kind of progress report.  For live
			// queries, this is the most recent time we have results for.  For
			// static queries, this is the number of results loaded.
			if (isLoading)
			{
				if (!this.loading)
				{
					this.spinner.src = "static/images/throbber.gif";
					this.stop.src = "static/images/stop-enabled.png";
				}

				this.stop.parentNode.href =
					"javascript:OutputPane.get().stopLoading()";

				// Show best guess of how long it is until we're done displaying
				var pcComplete = 0;
				if (this.query && (this.query.currTotal > 0))
				{
					pcComplete = 
						Math.round( 100 * 
							Math.max( this.query.seen / this.query.currTotal,
							this.recCount / this.bufSize ) )
				}

				if (this.query.live)
				{
					this.status.innerHTML = "Displaying results to " +
						OutputPane.niceDate( ServerTime.get() );
				}
				else
				{
					this.status.innerHTML = "(" + pcComplete + "% complete)";
				}
			}

			else
			{
				if (!this.loading)
					return;

				this.spinner.src = "static/images/throbber.png";
				this.stop.src = "static/images/stop-disabled.png";
				this.stop.parentNode.href = "#";

				// Show number of results displayed
				var display;

				if (!this.query)
				{
					display = "";
				}

				else if (this.query.count == 0)
				{
					display = "No results to display";
				}

				else if (this.query.seen < this.query.currTotal &&
						 !this.query.userTerminated)
				{
					// Truncated search, more results pending
					var linesRemaining = this.query.currTotal - this.query.seen;
					display = "Displaying results " +
						(this.alreadyDisplayed - this.recCount + 1) + " to " +
						this.alreadyDisplayed + " (" + linesRemaining +
						" possible results remain ... " +
						"<a href='javascript:OutputPane.get().resume()'>more</a>)";
				}

				else if ((this.query.seen == this.query.currTotal &&
						  this.alreadyDisplayed == this.recCount) ||
						 this.query.live ||
						 this.query.userTerminated)
				{
					// Completed search and all results fit on a single page
					display = "Displaying " + this.recCount + " results";
				}

				else
				{
					// Completed a multi-page search
					display = "Displaying results " +
						(this.alreadyDisplayed - this.recCount + 1) + " to " +
						this.alreadyDisplayed + " (search complete)";
				}

				this.status.innerHTML = display;
			}

			this.loading = isLoading;
		}

		this.resume = function()
		{
			var serverParams = this.query.getServerArgs( this );
			if (!serverParams)
				return;

			serverParams.startAddr = this.query.startAddr;
			serverParams.endAddr = this.query.endAddr;
			this.clear();
			this.query.fetch( this, serverParams );
		}

		this.stopLoading = function( dontUpdateLive )
		{
			// We have to set this before going into displayLoading() so that it
			// knows not to display the 'more' link.
			if (this.query)
				this.query.userTerminated = true;

			if (this.query && this.query.live && this.form && !dontUpdateLive)
				updateLive( false );
			else
				this.displayLoading( false );

			// We need to check for query here because we may have recursed in
			// here for a second time from updateLive() and query may be null.
			if (this.query)
			{
				this.query.task.terminate();
				this.query = null;
			}
		}

		this.insert = function( lines, reverse )
		{
			this.domInit();

			// -1 means never, ever follow output, and is used in static fetch mode
			if (this.followOutput != -1)
			{
				// If the user has scrolled to the bottom of the output window,
				// start following again.
				this.followOutput =
					this.window.pageYOffset == this.window.scrollMaxY;
			}

			for (var i=0; i < lines.length; i++)
			{
				var line = document.createTextNode( lines[i] );
				if (!reverse)
				{
					this.results.appendChild( line );
				}
				else
				{
					this.results.insertBefore(
						line, this.results.childNodes[0] );
				}
				this.recCount++;
				this.alreadyDisplayed++;
			}

			while (this.recCount > this.bufSize)
			{
				if (!reverse)
				{
					this.results.removeChild(
						this.results.childNodes[0] );
				}
				else
				{
					this.results.removeChild(
						this.results.childNodes[
							this.results.childNodes.length-1 ] );
				}
				this.recCount--;
			}

			if (this.followOutput == 1)
			{
				this.window.scrollTo( this.window.pageXOffset,
									  this.window.scrollMaxY );
			}

			this.displayLoading( true );
		}

		this.clear = function ()
		{
			this.domInit();
			this.body.removeChild( this.results );
			this.results = PRE();
			this.body.appendChild( this.results );
			this.recCount = 0;
		}

		this.fill();
	},

	// Create an output pane, passing the parameters through to the constructor
	create: function( anchorElt, params )
	{
		if (OutputPane.sInstance != null)
		{
			Util.error( "You can't create two OutputPanes" );
		}
		else
		{
			OutputPane.sInstance = new OutputPane.OutputPane( anchorElt, params );
			window.addEventListener( "resize",
									 function(){ OutputPane.get().fill(); },
									 false );
		}

		return OutputPane.sInstance;
	},

	// Return a reference to an already created logging pane
	get: function()
	{
		if (OutputPane.sInstance == null)
		{
			Util.error( "No OutputPane exists yet" );
			return null;
		}
		else
			return OutputPane.sInstance;
	},

	// A nice date for displaying
	niceDate: function( timeInSeconds )
	{
		var s = new String( new Date( timeInSeconds * 1000 ) );

		// Remove the timezone information from the end of the string
		return s.replace( / \w+[+-]\d+.*/, "" );
	}
};
