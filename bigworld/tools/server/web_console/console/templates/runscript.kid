<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
layout_params['page_specific_css'] = [ "/console/static/css/runscript.css" ]
layout_params['page_specific_js']  = [ "/console/static/js/runscript.js", "/console/static/js/argtypes.js" ]
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

<div py:def="moduleContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Run Scripts';
	</script>

<div py:if="serverRunning">

	<table id="scriptLibrary">
	<tr py:replace="tableHeader( 'Commands' )"/>

	<!-- Main command area contents -->
	<form name="executeForm"
		action="javascript:RunScript.executeScriptFromForm()">
	<tr>
		<td>
			<!-- <span id="interfaceContainer"> -->
			<!-- <div id="scriptPane"> -->
			<select size="10" id="scriptSelect"/>
			<!-- </div> -->
			<!-- </span> -->

			<div id="scriptInfoPane">
				<!-- right aligned script information -->
				<div id="scriptTitlePane"/><br/>
				<div id="scriptParamPane"/><br/>
				<div id="scriptExecutePane"/>
			</div>
		</td>
	</tr>
	</form>

	</table>

	<div id="outputContainer" class="pyscript">
		<!--div id="outputContainerActions">
			<a href="javascript:RunScript.clearOutputPane()">Clear</a>
			</div-->
		<span class="heading">Return Value</span>
		<div id="resultPane"/>
		<br/>
		<span class="heading">Console Output</span>
		<div id="outputPane"/>
	</div>

	<script type="text/javascript">
		<p py:if="runNow" py:strip="True">
			MochiKit.DOM.addLoadEvent( MochiKit.Base.bind( RunScript.executeScript, RunScript, ${runNow.id} ) );
		</p>

		<?python import simplejson ?>
		RunScript.initCategories( ${simplejson.dumps( categories )} );

		function initLoad()
		{
			RunScript.switchCategory( "watcher", "" );
		}
		MochiKit.DOM.addLoadEvent( initLoad );
	</script>

</div> <!-- py:if="serverRunning" -->

<div py:if="not serverRunning">
	No server running.
</div> <!-- py:if="not serverRunning" -->

</div>

</html>
