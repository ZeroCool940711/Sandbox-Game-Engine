<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "pageHeader" ] = "Watchers"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'layout.kid'"
	  py:extends="'common.kid'">
	
<div py:def="pageContent()">
	
	<script type="text/javascript">
		PAGE_TITLE = 'Watcher Listing';
	</script>

	<?python
	  import os
	  from web_console.common.util import alterParams

	  user = m.cluster.getUser( p.uid )
	  pathSplit = wd.path.split( "/" )
	?>

	
	<h1 class="watcherheader">Watcher Values for 
		<span py:if="not wd.path" py:content="p.label()" py:strip="True"/>
		<span py:if="wd.path" py:strip="True">
			<a href="${alterParams( path='' )}">${p.label()}</a>
			<span py:for="i in xrange( len( pathSplit ) - 1 )" py:strip="True">
				/ <a href="${alterParams( path='/'.join( pathSplit[:i+1] ) )}">${pathSplit[i]}</a>
			</span>
			/ ${pathSplit[-1]}
		</span>
	</h1>

	<div py:if="wd.isDir()">

		<?python
		  colspan = 4
		?>

		<script type="text/javascript" src="static/js/watchers.js"/>
		<form name="menuForm">
		<table class="watcher">
			<tr py:if="wd.path">
				<td colspan="${colspan}">
				<a href="${alterParams( path=os.path.dirname( wd.path ) )}">..</a>
				</td>
			</tr>
	
			<tr py:for="dir in subdirs" class="watcherrow">
				<td colspan="${colspan}">
					<a href="${alterParams( path=dir.path )}">
						${os.path.basename( dir.path )}
					</a>
				</td>
			</tr>
			<tr py:for="(w, menu) in watchers" class="watcherrow">
				<td>${os.path.basename( w.path )} </td>

				<!-- Check if the watcher is a function or not -->
				<td py:if="w.mode == 4">Callable function</td>
				<td py:if="w.mode != 4">
					<div py:if="w.mode == 1" id="read_only">${w.value}</div>
					<div py:if="w.mode != 1">${w.value}</div>
				</td>

				<!-- The advanced mode options -->
				<td class="customMenu" style="display: none">
					<select name="actionMenu" 
						py:replace="actionsMenu( menu, help = 'Add the menu now' )"/>
				</td>
			</tr>
		</table>
		</form>

		<!-- Checkbox to toggle display of custom watcher menu -->
		<form name="pageOptions" py:if="len(wd.getChildren()) > 0">
			Advanced Options: <input onclick="toggleCustomWatchers()" type="checkbox" name="showCustom"/> 
		</form>
	</div>

	<div py:if="not wd.isDir()">

		<form action="watcher" method="get">
		<table class="watcher">
			<tr><th class="heading" colspan="2">${wd.name}</th></tr>
			<tr>
				<td class="colheader">Existing Value</td>
				<td>${wd.value}</td>
			</tr>
			<tr>
				<td class="colheader">New Value</td>
				<td>
					<input type="hidden" name="machine" value="${m.name}"/>
					<input type="hidden" name="pid" value="${p.pid}"/>
					<input type="hidden" name="path" value="${wd.path}"/>
					<input type="hidden" name="dataType" value="${wd.type}"/>
					<!-- If it's a boolean, show a dropdown selection -->
					<div py:if="wd.type == 4">
						<select name="newval">
							<option value="true">True</option>
							<option value="false">False</option>
						</select>
					</div>
					<div py:if="wd.type != 4">
						<input name="newval" value="${wd.value}" type="text"/>
					</div>
				</td>
			</tr>
			<tr>
				<td colspan="2" style="text-align:right;">
					<input type="submit" value="Modify"/>
				</td>
			</tr>
		</table>
		
		</form>

	</div>

	<script type="text/javascript">
		if ("${status}" == "False")
		{
			Util.error( "Failed to set value" );
		}
	</script>
	
</div>

</html>
