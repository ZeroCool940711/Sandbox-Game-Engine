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

	<script type="text/javascript">
		PAGE_TITLE = 'Machine Info';
	</script>
	
	<p py:content="displayMachines( [m], 'Machine Info' )"/>
	<p py:content="displayProcs( ps, 'u', 'Processes running on %s' % m.name )"/>
	<p><a href="${tg.url( 'machines' )}">Show all machines</a></p>

</div>

</html>
