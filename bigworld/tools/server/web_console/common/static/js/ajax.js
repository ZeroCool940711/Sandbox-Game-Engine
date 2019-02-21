/**
 *  This is the client-side implementation of the AJAX mechanisms in
 *  web_console/common/ajax.py.
 */

/* namespace */ var Ajax =
{
	OK: 0,
	ERROR: 1,
	EXCEPTION: 2,

	/**
	 *  Perform an AJAX callback to a method on the server that has been exposed
	 *  with @ajax.expose()
	 */
	call: function( url, params, onSuccess, onError, promptArgs, silent )
	{
		if (!params)
			params = {};

		// If promptArgs is given, ask the user for input and pass the arguments
		// through to the server after prefixing them with a "__" to avoid any
		// conflicts with parameters already in 'params'.
		if (promptArgs)
		{
			for (var i in promptArgs)
			{
				params[ "__" + promptArgs[i][0] ] =
					prompt( "Enter " + promptArgs[i][1] + ": " );
				if (params[ "__" + promptArgs[i][0] ] == undefined)
				{
					return;
				}
			}
		}

		var d = doSimpleXMLHttpRequest( url, params );
		d.addCallback( partial( Ajax.recv, onSuccess, onError, silent ) );
	},


	/**
	 *  This method is called each time the browser receives some data from the
	 *  server.
	 */
	recv: function( onSuccess, onError, silent, req )
	{
		var dict = evalJSONRequest( req );

		if (dict._status == Ajax.OK)
		{
			// This is an if/if, instead of an if/else if because I don't think
			// these are necessarily mutually exclusive
			if (onSuccess)
			{
				onSuccess( dict );
			}

			if (dict._message && !silent)
			{
				Util.info( dict._message );
			}

		}
		else if (dict._status == Ajax.ERROR)
		{
			if (onError)
			{
				onError( dict._error, dict._details );
			}
			else if (dict._error)
			{
				Util.error( dict._error, dict._details );
			}
		}
		else if (dict._status == Ajax.EXCEPTION)
		{
			window.open( dict._redirect );
		}
	},
};
