<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
	import time
	from stat_logger import constants
	layout_params['moduleHeader'] = "StatGrapher"
?>

<html py:layout="'../../common/templates/layout.kid'" 
	xmlns="http://www.w3.org/1999/xhtml" 
	xmlns:py="http://purl.org/kid/ns#"
	py:extends="'../../common/templates/common.kid'">

	<body py:def="moduleContent()" py:strip="True">

	<script type="text/javascript">
		PAGE_TITLE = 'Preferences for log "${log}"';
	</script>

	<h2>Preferences for log "${log}"</h2>

	<div style="width: 70%">
	These preferences are read-only, and were generated from stat_logger's preference file. 
	See the "StatLogger" chapter in the Server Operations Guide for information in configuring 
	the preferences for StatLogger, which affects what can be viewed here in StatGrapher.
	<p/>
	The last two columns, "Show" and "Colour", refer to current user settings, which can 
	be changed from the actual graph page (no changes can be made on this page).
	<p/>
	</div>

	<table class="sortable" py:for="proc in prefTree.iterProcPrefs()" style="width: 70%; margin-bottom: 2em;">
		<th py:replace="tableHeader( 'Process: %s' % proc.name )"></th>
		<tr class="sortrow">
			<td class="colheader">Display name</td>
			<td class="colheader">Value</td>
			<!--td class="colheader">Type</td-->
			<td class="colheader">Max</td>
			<td class="colheader">Consolidate</td>
			<td class="colheader">Show</td>
			<td class="colheader">Colour</td>
		</tr>
		<tr py:for="stat in proc.iterAllStatPrefs()">
			<?python
			displayPref = displayPrefs["procPrefs"][proc.name][str( stat.dbId )]
			?>
			<td>${stat.name}</td>
			<td>${stat.valueAt}</td>
			<!--td>${stat.type}</td-->
			<td>${stat.maxAt}</td>
			<td>${constants.CONSOLIDATE_NAMES[stat.consolidate]}</td>
			<td>${str(stat.dbId) in displayPrefs["enabledProcStatOrder"][proc.name] and "Yes" or "No"}</td>
			<td><div style="width: 10px; height: 10px; background-color: #${displayPref['colour']}" /></td>
		</tr>
	</table>

	<table class="sortable" style="width: 70%; margin-bottom: 2em;">
		<th py:replace="tableHeader( 'Machines' )"></th>
		<tr class="sortrow">
			<td class="colheader">Display name</td>
			<td class="colheader">Value</td>
			<!--td class="colheader">Type</td-->
			<td class="colheader">Max</td>
			<td class="colheader">Consolidate</td>
			<td class="colheader">Show</td>
			<td class="colheader">Colour</td>
		</tr>
		<tr py:for="stat in prefTree.iterMachineStatPrefs()">
			<?python
			displayPref = displayPrefs["machineStatPrefs"][str( stat.dbId )]
			?>
			<td>${stat.name}</td>
			<td>${stat.valueAt}</td>
			<!--td>${stat.type}</td-->
			<td>${stat.maxAt}</td>
			<td>${constants.CONSOLIDATE_NAMES[stat.consolidate]}</td>
			<td>${str(stat.dbId) in displayPrefs["enabledMachineStatOrder"] and "Yes" or "No"}</td>
			<td><div style="width: 10px; height: 10px; background-color: #${displayPref['colour']}" /></td>
		</tr>
	</table>
	</body>
</html>
