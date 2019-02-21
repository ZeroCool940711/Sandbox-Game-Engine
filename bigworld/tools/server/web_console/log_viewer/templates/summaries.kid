<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "moduleHeader" ] = "Log Viewer"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

<div py:def="moduleContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Log Usage Summary';
	</script>

	<table class="sortable">

		<tr py:replace="tableHeader( 'Log Summary For All Users' )"/>

		<tr py:replace="colHeaders( ('Username', 'Size', 'Entries', 'Segments',
									 'Start', 'End', 'Duration') )"/>

		<tr py:for="i in info">

			<td><a href="${tg.url( 'summary', user=i[0] )}">${i[0]}</a></td>
			
			<td py:for="x in i[1:]" class="numeric">${x}</td>
			
		</tr>

	</table>
	
</div>

</html>
