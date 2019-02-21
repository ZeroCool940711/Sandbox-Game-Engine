function createWatcherPage( username )
{
	var params = new Object();
	params.name = window.prompt( "New custom watcher name:" );
	params.user = username;

	// Reload the page when we've successfully added the new watcher
	// page so that the list of custom watcher pages is updated.
	function onSuccess( )
	{
		window.location = "/cc/customWatcherList";
	}

	if (params.name)
	{
		Ajax.call( "/cc/createCustomWatcher", params, onSuccess )
	}
}



function saveWatcherToLayout( customWatcher, component, watcher )
{
	var params = new Object();
	params.customWatcher = customWatcher;
	params.component = component;
	params.watcher = watcher;


	// TODO: Might be worth adding an onSuccess() to this ajax call
	//       to refresh the page similar to createWatcherPage()
	//       above. Would involve passing in the page that called
	//       us.

	Ajax.call( "saveValueToCustomWatcher", params );
}


function deleteCustomWatcher( customWatcher )
{
	var params = new Object();
	params.customWatcher = customWatcher;

	// Reload the page when we've successfully added the new watcher
	// page so that the list of custom watcher pages is updated.
	function onSuccess( )
	{
		window.location = "/cc/customWatcherList";
	}

	if (confirm("About to delete custom watcher " + customWatcher)) { 
		Ajax.call( "/cc/deleteCustomWatcher", params, onSuccess );
	}
}


function deleteCustomWatcherEntry( customWatcherPage, component, watcherPath )
{
	var params = new Object();
	params.customWatcherPage = customWatcherPage;
	params.component = component;
	params.watcherPath = watcherPath;

	// Remove the '#' put on by the onClick call so we reload the page
	var tmploc = new String( window.location );
	tmploc = tmploc.substring(0, tmploc.length)

	// Reload the page when we've successfully delete the watcher path
	// so that we are displaying an up to date list of items.
	function onSuccess( )
	{
		window.location = tmploc;
	}

	if (confirm("Delete watcher path " + watcherPath + "?")) { 
		Ajax.call( "/cc/deleteCustomWatcherEntry", params, onSuccess );
	}
}


/*
 * Toggle the visibility of table cells with the custom watcher action menu
 */
function toggleCustomWatchers()
{
	arr = getElementsByTagAndClassName("td", "customMenu");
	for (i=0; i<arr.length; i++) {
		if (document.pageOptions.showCustom.checked) {
			arr[i].style.display = 'table-cell';
		} else {
			arr[i].style.display = 'none';
		}
	} 
}
