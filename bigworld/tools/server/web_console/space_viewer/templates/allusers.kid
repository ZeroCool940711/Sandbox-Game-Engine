<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "pageHeader" ] = "All Users"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">
	
<div py:def="moduleContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'All Users';
	</script>

	<!--
		Display a table of users.
	-->
	<script type="text/javascript" src="/static/js/table.js"/>
	<div py:def="displayUsers( users, header )">
		<table class="sortable">
			<tr py:replace="tableHeader( header )"/>
			<tr class="sortrow">
				<td class="colheader"> Username </td>
				<td class="colheader"> UID </td>
			</tr>
			
			<tr py:for="u in users">
				<td>
					<a href="${tg.url( 'sv_main', user=u.name)}"
					   py:content="u.name"/>
				</td>
				<td py:content="u.uid" class="numeric"/>
			</tr>
		</table>
	</div>

	<table style="border: none">
	<tr valign="top">
	<td style="border: none"><table py:replace="displayUsers( activeUsers, 'All Users' )"/></td>
	</tr>
	<tr>
	<td colspan="2" style="border: none">
	<a href="${tg.url( 'usersFlush' )}">Refresh User List</a>
	</td>
	</tr>
	</table>

</div>

</html>
