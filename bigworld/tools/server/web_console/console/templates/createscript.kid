<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
layout_params['page_specific_js']  = \
	[ "/console/static/js/createscript.js",
	  "/console/static/js/argtypes.js"]
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">


<div py:def="moduleContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Create Script';
	</script>

	<div py:if="error" style="color:red;">
	<h4>Errors</h4>
	<ul py:if="isinstance(error, list)">
		<li py:for="msg in error">${msg}</li>
	</ul>
	<span py:strip="True" py:if="not isinstance(error, list)">${error}</span>
	</div>

	<form action="$dest" method="POST">
		<input type="hidden" name="mode" value="submit"/>
		<table class="bordered">
			<tr py:if="action == 'add'" py:replace="tableHeader('Add a new script')"/>
			<tr py:if="action != 'add'" py:replace="tableHeader('Edit script: %s' % (title))"/>
			<tr>
				<td class="colheader">Script name</td>
				<td><input name="title" length="40" value="$title"/></td>
			</tr>
			<tr>
				<td class="colheader">Description</td>
				<td>
					<textarea name="desc" cols="80" rows="1">$desc</textarea>
				</td>
			</tr>
			<tr>
				<td class="colheader">Arguments</td>
				<td>
					<div id="args">
					</div>
					<input id="addArg" type="button" value="Add new argument"/>
					<input id="removeArg" type="button" value="Remove argument"/>
				</td>
			</tr>
			<tr>
				<td class="colheader">World Readable</td>
				<td><input type="checkbox" name="worldReadable" value="True" checked="${worldReadable or None}"/></td>
			</tr>
			<tr>
				<td class="colheader">Lock cells</td>
				<td><input type="checkbox" name="lockCells" value="True" checked="${lockCells or None}"/></td>
			</tr>
			<tr>
				<td class="colheader">Process type</td>
				<td>
					<input type="radio" name="procType" 
						py:for="validProc in validProcTypes" 
						value="$validProc" checked="${(procType == validProc or None)}">
						$validProc
					</input>
				</td>
			</tr>
			<tr>
				<td class="colheader">Run method</td>
				<td>
					<input type="radio" name="runType" 
						py:for="validRun in validRunTypes" 
						value="$validRun" checked="${(runType == validRun or None)}">
						$validRun
					</input>
				</td>
			</tr>
			<tr>
				<td class="colheader">Code</td>
				<td> 
					<textarea name="code" cols="80" rows="30">$code</textarea>
				</td>
			</tr>
			<tr>
				<td class="colheader" colspan="2">
					<input type="submit" value="Submit"/>
				</td>
			</tr>
		</table>
	</form>

	<script type="text/javascript">
		CreateScript.setArgList( $jsArgs );
	</script>

</div>

</html>
