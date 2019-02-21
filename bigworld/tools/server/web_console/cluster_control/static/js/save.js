function saveServerLayout( username )
{
	var params = new Object();
	params.name = window.prompt( "Please enter a name to save this layout as" );
	params.user = username;

	if (params.name)
	{
		Ajax.call( "/cc/saveLayout", params )
	}
}
