<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

	
	<div py:def="scriptinfo(script, targets=None)">

		<script type="text/javascript">
			PAGE_TITLE = 'View script source';
		</script>

		<?python
		def printBool(x):
			if x: return "Yes"
			else: return "No"
		?>
		
		<table>
			<tr py:replace="tableHeader( 'Script info for: \'%s\'' % (script.name) )"></tr>
			<tr>
				<td>Name</td>
				<td py:content="script.name"/>
			</tr>
			<tr>
				<td>Description</td>
				<td py:content="script.description"/>
			</tr>
			<tr py:if="script.filename">
				<td>File path</td>
				<td py:content="script.filename"/>
			</tr>
			<tr>
				<td>Owner</td>
				<td py:content="script.user.user_name"/>
			</tr>
			<tr>
				<td>World readable</td>
				<td py:content="printBool(script.worldReadable)"/>
			</tr>
			<tr>
				<td>Lock Cells</td>
				<td py:content="printBool(script.lockCells)"/>
			</tr>
			<tr>
				<td>Targets</td>
				<td py:if="targets and isinstance(targets, list)" py:content="','.join(t.label() for t in targets)"/>
				<td py:if="targets and not isinstance(targets, list)">Via watcher: <code>${targets}</code></td>
				<td py:if="not targets">${script.target}</td>
			</tr>
			<tr>
				<td>Code</td>
				<td>
					<pre style="background-color: lightgrey; padding: 5px; border: 1px dotted black;">${script.code}</pre>
				</td>
			</tr>
		</table>
	</div>

	<div py:def="moduleContent()">
		<div py:if="triedEdit" class="error" style="color: red; margin-bottom: 5px;">
			You cannot edit scripts which don't belong to you.
		</div>
		<div py:content="scriptinfo(script)"/>
	</div>
</html>
