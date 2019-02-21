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
		PAGE_TITLE = 'FD Server Status';
	</script>
	<table py:if="procs" class="sortable">
		<tr py:replace="tableHeader( 'Procceses on FD Server' )"> <td/> </tr>
		<tr class="sortrow">
			<td class="colheader"> Process Name </td>
			<td class="colheader"> CPU Load </td>
			<td class="colheader"> Memory Usage </td>
			<td class="colheader"> PID </td>
			<td class="colheader"> Status </td>
		</tr>
		<tr py:for='p in procs'>
			<td py:content="'%s' % p.label()" class="string"/>
			<td py:content="'%.1f%%' % (p.load * 100)" class="numeric"/>
			<td py:content="'%.1f%%' % (p.mem * 100)" class="numeric"/>
			<td py:content="p.pid" class="numeric"/>
			<td align="center">
			<img py:attrs="{'src':tg.url('static/images/%s.png' % status)}"/>
			</td>
		</tr>
	</table>
	<div py:if="not procs">
		No processes running!
	</div>
</div>
</html>
