<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "pageHeader" ] = "Custom Watcher"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'layout.kid'"
	  py:extends="'common.kid'">

<body>

<!-- Document body -->
<div py:def="pageContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Custom Watcher: ${name}';
	</script>

	<table class="watcher">
		<tr class="heading">
			<th>Custom Watcher: ${name}</th>
		</tr>
	</table>

	<script type="text/javascript" src="static/js/watchers.js"/>
	<div py:for="comp in results">
	<p>
	<table>
	<tr class="heading">
		<th colspan="${len(results[ comp ][0]) + 1}">${comp}</th>
	</tr>
	<tr>
		<th style="background-color: #DDD"></th>
		<th py:for="watcher in results[ comp ][0]">
			${"/".join( watcher.split('/')[-2:] )}
			<a href="#" onClick="deleteCustomWatcherEntry('${name}','${comp}','${watcher}');"
			   title="Delete watcher '${watcher}' from this page">
				[x]
			</a>
		</th>
	</tr>
	<tr py:for="proc in results[ comp ][1]">
		<th align="right">${proc}</th>
		<td py:for="value in results[ comp ][1][ proc ]" align="left">${value}</td>
	</tr>
	</table>
	</p>
	</div>

</div> <!-- pageContent() -->
</body>
</html>
