<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "pageHeader" ] = "Layouts"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'layout.kid'"
	  py:extends="'common.kid'">
	
<head>
	<title> Layouts </title>
</head>

<body><div py:def="pageContent()">

	<?python
	  from pycommon import cluster
	?>

	<script type="text/javascript">
		PAGE_TITLE = 'Saved Layouts';
	</script>

	<script type="text/javascript" src="/static/js/table.js"/>
	<table py:if="layouts" class="sortable">
		<tr py:replace="tableHeader( 'Saved Layouts' )"/>

		<tr class="sortrow">
			<td class="colheader"> Name </td>
			<td class="colheader"> Server User </td>
			<td py:for="pname in pnames" class="colheader">
				${cluster.Process.getPlural( pname )}
			</td>
			<td/>
		</tr>
		
		<tr py:for="i in xrange( len( layouts ) )">
			<td py:content="recs[i].name" style="font-weight: bold"/>
			<td py:content="recs[i].serveruser"/>
			
			<td py:for="pname in pnames" py:content="layouts[i][ pname ]"
				class="numeric"/>
			
			<td><a href="${tg.url( 'deleteLayout', name = recs[i].name )}">
				delete
			</a></td>
		</tr>
	</table>

	<div py:if="not layouts">
		No saved layouts!
	</div>

</div></body>

</html>
