/**
 * Requires:
 *
 * server_time.js
 */

/* namespace */ var Query =
{
	// Toggle enabled state on period controls depending on selection
	setPeriodDisabled: function()
	{
		var disabled = (document.filters.periodDirection.value == "to beginning" ||
						document.filters.periodDirection.value == "to present");

		document.filters.periodCount.disabled = disabled;
		document.filters.periodUnits.disabled = disabled;
	},

	// Set text on the live output button according to given state
	setLiveButton: function( live )
	{
		document.filters.live.value = live ? "Stop" : "Live Output";
	},

	// Get live state from form
	getLiveButton: function()
	{
		return document.filters.live.value == "Stop";
	},


	/**
	 *  This class represents a query for log data.
	 */
	Query: function( live )
	{
		// The AsyncTask that is used to fetch the log data from the server.
		this.task = null;

		// The number of log messages that have been returned to this query.
		this.count = 0;

		// The number of log entries that have been searched by this query.
		// This is based on the first number in the pair returned by the
		// Query.getProgress() method on the server, and is used to determine
		// how much of the search has been completed.
		this.seen = 0;

		// The total number of log entries that might be searched by this
		// query.  This is the second number in the getProgress() pair.
		this.currTotal = -1;

		// Bool indicating whether or not this is a 'live' query.
		this.live = live || false;

		// These two are use when getting the next page of results for a
		// truncated query
		this.startAddr = null;
		this.endAddr = null;

		// This is a marker to say that this query was forcibly terminated by
		// the user.
		this.userTerminated = false;

		this.TOGGLABLE_FIELDS = ["date", "time", "host", "serveruser", "pid",
								 "appid", "procs", "severity"];

		// This is taken from bwlog.so
		this.SHOW_DATE = 1 << 0;
		this.SHOW_TIME = 1 << 1;
		this.SHOW_HOST = 1 << 2;
		this.SHOW_USER = 1 << 3;
		this.SHOW_PID = 1 << 4;
		this.SHOW_APPID = 1 << 5;
		this.SHOW_PROCS = 1 << 6;
		this.SHOW_SEVERITY = 1 << 7;
		this.SHOW_MESSAGE = 1 << 8;
		this.SHOW_ALL = 0x1FF;

		// Sensible defaults for when we want to hand-craft queries
		this.form = {time: "(server startup)",
					 periodCount: "",
					 periodUnits: "",
					 periodDirection: "to present",
					 host: "",
					 serveruser: "",
					 pid: "",
					 appid: "",
					 message: "",
					 exclude: "",
					 caseSens: true,
					 regex: false,
					 interpolate: true,
					 procs: "CellApp,BaseApp,CellAppMgr,BaseAppMgr,DBMgr,LoginApp",
					 severity: "TRACE,DEBUG,INFO,NOTICE,WARNING,ERROR,CRITICAL,HACK,SCRIPT",
					 live: false,
					 context: "",
					 autoHide: false,
					 showMask: this.SHOW_ALL};

		// Get the message logger flag for a field name
		this.getFlag = function( field )
		{
			// Due to the DB change to serveruser it is necessary to
			// special case this flag.
			if (field == "serveruser")
			{
				return this[ "SHOW_USER" ];
			}
			else
			{
				return this[ "SHOW_" + field.toUpperCase() ];
			}
		}

		// Populate this Query's dict from the form
		this.readForm = function()
		{
			this.form.time = document.filters.time.value;
			this.form.periodCount = document.filters.periodCount.value;
			this.form.periodUnits = document.filters.periodUnits.value;
			this.form.periodDirection = document.filters.periodDirection.value;
			this.form.host = document.filters.host.value;
			this.form.serveruser = document.filters.serveruser.value;
			this.form.pid = document.filters.pid.value;
			this.form.appid = document.filters.appid.value;
			this.form.message = document.filters.message.value;
			this.form.exclude = document.filters.exclude.value;
			this.form.caseSens = document.filters.caseSens.checked;
			this.form.regex = document.filters.regex.checked;
			this.form.interpolate = document.filters.interpolate.checked;
			this.form.procs = DOM.getSelected( document.filters.procs );
			this.form.severity = DOM.getSelected( document.filters.severity );
			this.form.live = Query.getLiveButton();
			this.form.context = document.filters.context.value;
			this.form.autoHide = document.filters.autoHide.checked;

			this.form.showMask = this.SHOW_MESSAGE;
			for (var i in this.TOGGLABLE_FIELDS)
			{
				var field = this.TOGGLABLE_FIELDS[i];
				var input = "show_" + field;
				if (document.filters[ input ].checked)
				{
					this.form.showMask |= this.getFlag( field );
				}
			}

			return this;
		}

		// Populate the form from this object's dict
		this.writeForm = function()
		{
			document.filters.time.value = this.form.time;
			document.filters.periodCount.value = this.form.periodCount;
			document.filters.periodUnits.value = this.form.periodUnits;
			document.filters.periodDirection.value = this.form.periodDirection;
			document.filters.host.value = this.form.host;
			document.filters.serveruser.value = this.form.serveruser;
			document.filters.pid.value = this.form.pid;
			document.filters.appid.value = this.form.appid;
			document.filters.message.value = this.form.message;
			document.filters.exclude.value = this.form.exclude;
			document.filters.caseSens.checked = this.form.caseSens;
			document.filters.regex.checked = this.form.regex;
			document.filters.interpolate.checked = this.form.interpolate;
			DOM.setSelected( document.filters.procs, this.form.procs );
			DOM.setSelected( document.filters.severity, this.form.severity );
			Query.setLiveButton( this.form.live );
			document.filters.context.value = this.form.context;
			document.filters.autoHide.checked = this.form.autoHide;

			for (var i in this.TOGGLABLE_FIELDS)
			{
				var field = this.TOGGLABLE_FIELDS[i];
				var input = "show_" + field;
				var flag = this.getFlag( field );
				document.filters[ input ].checked =
					this.form.showMask & flag ? true : false;
			}

			updateLive();
			Query.setPeriodDisabled();
			return this;
		}

		// Convert this query into a URL
		this.toURL = function( fetch )
		{
			var dict = new Object();
			dict.time = repr( this.form.time );
			dict.periodCount = repr( this.form.periodCount );
			dict.periodUnits = repr( this.form.periodUnits );
			dict.periodDirection = repr( this.form.periodDirection );
			dict.host = repr( this.form.host );
			dict.serveruser = repr( this.form.serveruser );
			dict.pid = repr( this.form.pid );
			dict.message = repr( this.form.message );
			dict.exclude = repr( this.form.exclude );
			dict.caseSens = repr( this.form.caseSens );
			dict.regex = repr( this.form.regex );
			dict.interpolate = repr( this.form.interpolate );
			dict.procs = repr( this.form.procs );
			dict.severity = repr( this.form.severity );
			dict.live = repr( this.form.live );
			dict.autoHide = repr( this.form.autoHide );
			dict.showMask = repr( this.form.showMask );

			var s = "?";

			for (var key in dict)
			{
				s += key + "=" + escape( dict[ key ] )+ "&";
			}

			s += fetch ? "fetch=true" : "fetch=false";
			return "/log/search" + s;
		}

		this.fromURL = function( url )
		{
			if (!url)
				url = window.location.search;

			var doFetch = false;

			var mappings = url.substr( 1 ).split( "&" );
			for (var i in mappings)
			{
				var parts = mappings[i].split( "=" );
				if (parts.length == 2)
				{
					if (parts[0] == "fetch" && eval( parts[1] ))
						doFetch = true;
					else
						this.form[ parts[0] ] = eval( unescape( parts[1] ) );
				}
			}

			if (doFetch)
			{
				var callback = function (q) { q.fetch(); }
				window.addEventListener( "load", partial( callback, this ), true );
			}
		}

		// Convert form values into query format server expects.  If an output
		// pane is passed, its max buffer size will be used.
		this.getServerArgs = function( outputPane )
		{
			if (!outputPane)
				outputPane = OutputPane.get();

			var time;

			// Work out time reference point
			if (/^\s*$/.test( this.form.time ) || this.form.time == "(now)")
			{
				time = ServerTime.get();
			}

			else if (this.form.time == "(server startup)")
			{
				time = "server startup";
			}

			else if (this.form.time == "(beginning of log)")
			{
				time = 0;
			}

			else if (this.form.time[0] == "#")
			{
				time = this.form.time;
			}

			else
			{
				var match = /\.(\d+)\s*$/.exec( this.form.time );
				var msTime = match ? new Number( "0." + match[1] ) : 0;
				var noMsTime = match ? this.form.time.replace( match[0], "" ) :
					this.form.time;

				time = (new Date( noMsTime )).getTime() / 1000.0 + msTime;

				if (isNaN( time ))
				{
					Util.error( "Bad time specification. " +
								"You must use the same format as appears " +
								"in the logs (including both date and time)" );
					return null;
				}
			}

			// Work out period
			var period, mult;
			if (this.form.periodDirection == "to beginning")
			{
				period = "to beginning";
			}
			else if (this.form.periodDirection == "to present")
			{
				period = "to present";
			}

			else if (/^\s*$/.test( this.form.periodCount ))
			{
				Util.error( "You must specify a number of " +
							this.form.periodUnits );
				return null;
			}

			else
			{
				if (this.form.periodDirection == "forwards")
				{
					period = "+";
				}
				else if (this.form.periodDirection == "backwards")
				{
					period = "-";
				}
				else
				{
					period = "";
				}

				switch (this.form.periodUnits)
				{
					case "seconds": mult = 1; break;
					case "minutes": mult = 60; break;
					case "hours": mult = 60 * 60; break;
					case "days": mult = 24 * 60 * 60; break;
				}

				period += new Number( this.form.periodCount ) * mult;
			}

			var args = new Object();
			args.time = time;
			args.period = period;
			args.host = this.form.host;
			args.serveruser = this.form.serveruser;
			args.pid = this.form.pid;
			args.appid = this.form.appid;
			args.message = this.form.message;
			args.exclude = this.form.exclude;
			args.caseSens = this.form.caseSens;
			args.regex = this.form.regex;
			args.interpolate = this.form.interpolate;
			args.procs = this.form.procs;
			args.severity = this.form.severity;
			args.showMask = this.form.showMask;
			args.context = this.form.context;

			// Limit size of static queries
			if (outputPane && !this.live)
				args.max = outputPane.bufSize;

			// Insert resume addresses if we have em
			if (this.startAddr && this.endAddr)
			{
				args.startAddr = this.startAddr;
				args.endAddr = this.endAddr;
			}

			return args;
		}

		// Executes this query, writing the results into the provided output pane.
		this.fetch = function( outputPane, serverParams, wait, callback )
		{
			// Auto args
			if (!outputPane)
				outputPane = OutputPane.get();

			if (!serverParams)
			{
				serverParams = this.getServerArgs();
				if (!serverParams)
					return;
			}

			// Clear some variables
			this.seen = 0;
			this.currTotal = -1;

			var This = this;

			var onID = function( id )
			{
				outputPane.displayLoading( true );
			};

			var onUpdate = function( state, data )
			{
				if (state == "results")
				{
					outputPane.insert( data.lines, data.reverse );
				}
				else if (state == "progress")
				{
					This.startAddr = data[0];
					This.count = data[1];
					This.seen = data[2];
					This.currTotal = data[3];
					outputPane.displayLoading( true );
				}
				else if (state == "truncated")
				{
					This.task.terminate();
					This.endAddr = data;
					outputPane.displayLoading( false );
				}
			}

			var onNoUpdate = function()
			{
				outputPane.displayLoading( true );
			}

			var onFinished = function()
			{
				outputPane.displayLoading( false );
				if (callback != undefined)
					callback();
			}

			outputPane.setQuery( this );

			this.task = new AsyncTask.AsyncTask(
				onID, onUpdate, onNoUpdate, onFinished, wait || 100, false );

			this.task.start( "/log/fetch", serverParams );
		}
	}
};
