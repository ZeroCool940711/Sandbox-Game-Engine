<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "moduleHeader" ] = "Developer Viewer"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

<div py:def="moduleContent()">
	<script type="text/javascript">
		PAGE_TITLE = 'Service Status';
	</script>

		<table class="sortable">
			<tr py:replace="tableHeader( 'Service Status' )"> <td/> </tr>

			<tr class="sortrow">
				<td class="colheader" align="center" width="400"> Service Name </td>
				<td class="colheader" align="center"> Status </td>
			</tr>

			<tr py:for="l in links">
				<td>
				<a py:attrs="'href':tg.url('%s' % l.url)" py:content="'%s' % l.name"/>
				</td>
				<td align="center">
				<img py:attrs="{'src':tg.url('static/images/%s.png' % l.status)}"/>
				</td>
			</tr>
		</table>
</div>
</html>