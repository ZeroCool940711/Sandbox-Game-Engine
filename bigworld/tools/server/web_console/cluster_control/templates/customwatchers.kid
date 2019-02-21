<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "pageHeader" ] = "Custom Watchers"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'layout.kid'"
	  py:extends="'common.kid'">
	
<body>

	<!-- Document body -->
	<div py:def="pageContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Custom Watchers';
	</script>

	<table py:if="customWatchers">
		<tr class="heading">
			<th>Watcher Page Name</th>
			<th>Actions</th>
		</tr>
		<tr py:for="(custom, menu, count) in customWatchers">
			<td>
			<div py:if="count!=0">
			<a href="${tg.url( 'viewCustomWatcher', name=custom.pageName )}">
				${custom.pageName}
			</a>
			</div>
			<div py:if="count==0"> ${custom.pageName} </div>
			</td>
			<td align="center">
					<select py:replace="actionsMenu(menu)"/>
			</td>
		</tr>
	</table>

	<div py:if="not customWatchers">
		No custom watcher views defined!
	</div>

	<script type="text/javascript" src="static/js/watchers.js"/>
	<p>
		<a href="#" onclick="createWatcherPage( '${user.name}' )">
			New custom watcher page
		</a>
	</p>

	</div>
</body>
</html>
