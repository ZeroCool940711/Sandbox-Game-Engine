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
		PAGE_TITLE = 'Automatic Documents Builds';
	</script>
		<table class="sortable" width="300">
			<tr py:replace="tableHeader( 'Automatic Documents Builds' )"> <td/> </tr>
			<tr class="sortrow">
				<td class="colheader" align="center" width="80%"> Module Name </td>
				<td class="colheader" align="center"> Status </td>
			</tr>
			<tr py:for='m in mods'>
				<td>
				<a href="${tg.url( 'doclog', module=m )}"> Auto Document Builds - $m </a>
				</td>
				<td align="center">
				<img py:attrs="{'src':tg.url( 'static/images/%s.png' % mods[m])}"/>
				</td>
			</tr>
		</table>
		<p py:content="'Build Date: %s' % date"/>
</div>
</html>
