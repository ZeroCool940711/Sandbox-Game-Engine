<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
layout_params['page_specific_css'] = [ "/console/static/css/runscript.css" ]
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

<div py:def="moduleContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Edit Scripts';
	</script>

	
	<div py:if="myScripts">
		<table class="sortable">
			<tr py:replace="tableHeader( 'My scripts' )"> <td/> </tr>
			<tr class="sortrow">
				<td class="colheader"> Script name </td>
				<td class="colheader"> Description </td>
				<td class="colheader"> Action </td>
			</tr>
			<?python 
				from web_console.common import util
			?>
			<tr py:for="s, actions in myScripts">
				<td>${s.title}</td>
				<td>${s.desc}</td>
				<td py:content="actionsMenu( actions )"/>
			</tr>
		</table>
	</div>
		
	<div py:if="otherScripts">
		<table class="sortable">
			<tr py:replace="tableHeader( 'My scripts' )"> <td/> </tr>
			<tr class="sortrow">
				<td class="colheader"> Script name </td>
				<td class="colheader"> Description </td>
				<td class="colheader"> Owner </td>
				<td class="colheader"> Action </td>
			</tr>
			<?python 
				from web_console.common import util
			?>
			<tr py:for="s, actions in otherScripts">
				<td>${s.title}</td>
				<td>${s.desc}</td>
				<td>${s.user.user_name}</td>
				<td py:content="actionsMenu( actions )"/>
			</tr>
		</table>
	</div>

	<p>
		<a href="createscript">Create a script</a>
	</p>

</div>

</html>
