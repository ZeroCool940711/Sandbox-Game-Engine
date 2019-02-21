var outpane = OutputPane.create( $("outputGoesHere"), {} );

function goLive()
{
	var query = new Query.Query( true );
	query.form.periodDirection = "to present";
	query.form.serveruser = serveruser;
	query.form.showMask = (query.SHOW_TIME | query.SHOW_HOST |
						   query.SHOW_PROCS | query.SHOW_SEVERITY | query.SHOW_MESSAGE);

	var args = query.getServerArgs( outpane );
	args.live = true;
	args.time = ServerTime.get();

	query.fetch( outpane, args, 1000 );
}

window.addEventListener( "load", goLive, false );
