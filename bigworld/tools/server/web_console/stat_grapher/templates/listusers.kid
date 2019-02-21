<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
	import time
	layout_params['moduleHeader'] = "StatGrapher"
?>

<html py:layout="'../../common/templates/layout.kid'" xmlns="http://www.w3.org/1999/xhtml" xmlns:py="http://purl.org/kid/ns#">
	<body py:def="moduleContent()" py:strip="True">
		
	<script type="text/javascript">
		PAGE_TITLE = 'All Users';
	</script>
	
	<table class="sortable">
		<tr class="heading">
			<th colspan="2">Users available in ${log}</th>
		</tr>
		<tr>
			<td colspan="2" style="inlinehelp">
				Select which user to view processes for.
			</td>
		</tr>
		<tr class="sortrow">
			<td class="colheader">User name</td>
			<td class="colheader">User ID</td>
		</tr>
		<tr py:for="row in outputList">
			<td><a href="../../${action}/${log}/${row[0]}">${row[1]}</a></td>
			<td>${row[0]}</td>
		</tr>
	</table>

	</body>
</html>
