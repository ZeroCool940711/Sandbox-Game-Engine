/* namespace */ var ServerTime =
{
	offset: null,

	setOffset: function( offset )
	{
		ServerTime.offset = offset;
	},

	get: function()
	{
		return Date.now()/1000.0 + ServerTime.offset;
	},

	// Convert a server time (expressed in seconds) to a JavaScript Date object
	toDate: function( t )
	{
		return new Date( t * 1000 );
	}
};
