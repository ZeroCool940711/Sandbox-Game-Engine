/**
 *  This module provides implementations of javascript methods referenced in
 *  common/caps.py.
 */

/* namespace */ var Caps =
{
	gotoProcessLog: function( user, pname, mname, pid )
	{
		Util.include( "/log/static/js/query.js" );

		var q = new Query.Query();
		q.form.serveruser = user;
		q.form.pid = pid;
		q.form.host = mname;
		q.form.procs = [pname];
		q.form.showMask = (q.SHOW_DATE | q.SHOW_TIME | q.SHOW_SEVERITY |
						   q.SHOW_MESSAGE);
		window.location = q.toURL( true );
	}
};
