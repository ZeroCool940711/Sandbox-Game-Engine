<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
	import time
	layout_params['moduleHeader'] = "StatGrapher"
?>

<html py:layout="'../../common/templates/layout.kid'" 
	xmlns="http://www.w3.org/1999/xhtml" 
	xmlns:py="http://purl.org/kid/ns#"
	py:extends="'../../common/templates/common.kid'">

	<body py:def="moduleContent()" py:strip="True">

	<script type="text/javascript">
		PAGE_TITLE = 'List Of Logs';
	</script>
		
	<table class="sortable">
		<tr py:replace="tableHeader( 'Logs available for viewing' )"></tr>

		<tr class="sortrow">
			<td class="colheader">Log name</td>
			<td class="colheader">Created</td>
			<td class="colheader">Last updated</td>
			<td class="colheader">Status</td>
			<td class="colheader">Action</td>
		</tr>
		<tr py:if="not outputList">
		<td colspan="5" class="nologdbs">No log databases found.</td>
		</tr>
		<tr py:for="row in outputList" style="${(row[3] and 'background-color: #BBFFBB' or '')}">
			<td>${row[0]}</td>
			<td>${tg.formatTime(row[1])}</td>
			<td>${tg.formatTime(row[2])}</td>
			<td>${row[3] and "Running" or "Stopped"}</td>
			<td py:content="actionsMenu( row[4] )" />
		</tr>
	</table>
	</body>
</html>
