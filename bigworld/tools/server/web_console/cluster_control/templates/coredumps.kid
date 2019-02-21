<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "pageHeader" ] = "Machine Info"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'layout.kid'"
	  py:extends="'common.kid'">
	
<div py:def="pageContent()">

	<?python
	  import time
	?>

	<script type="text/javascript">
		PAGE_TITLE = 'Coredumps';
	</script>

	<table class="sortable">
		<tr py:replace="tableHeader( 'Coredumps for %s' % user.name )"/>
		<tr py:replace="colHeaders( ('Filename', 'Time', 'Assertion') )"/>

		<tr py:for="fname, assrt, t in coredumps">
			
			<td valign="top">
				<pre class="thinpre" py:content="fname"/>
			</td>
			
			<td valign="top">
				<pre class="thinpre" py:content="time.ctime( t )"/>
			</td>
			
			<td valign="top">
				<pre class="thinpre" py:content="assrt or '(none)'"/>
			</td>
		</tr>
	</table>
	
</div>

</html>
