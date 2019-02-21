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
		PAGE_TITLE = 'Detailed Log Summary';
	</script>

	<?python
	  import time
	  from pycommon import util
	?>

	<table class="sortable">

		<tr py:replace="tableHeader( 'Log Summary for ' + user )"/>

		<tr py:replace="colHeaders( ('Segment Name', 'Start', 'End', 'Duration',
									 'Entries', 'Size') )"/>

		<tr py:for="s in segments">

			<td>${s.suffix}</td>
			<td>${time.ctime( s.start )}</td>
			<td>${time.ctime( s.end )}</td>
			<td class="numeric">${util.fmtSecondsLong( int( s.end - s.start ) )}</td>
			<td class="numeric">${s.nEntries}</td>
			<td class="numeric">${util.fmtBytes( s.entriesSize + s.argsSize, True )}</td>
			
		</tr>

	</table>

	<p><a href="summaries">Show summary for all users</a></p>

</div>

</html>
