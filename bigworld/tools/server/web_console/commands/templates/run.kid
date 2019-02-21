<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
layout_params['page_specific_css'] = [ "/commands/static/css/runscript.css" ]
layout_params['page_specific_js']  = [ "/commands/static/js/runscript.js", "/commands/static/js/argtypes.js" ]
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

<div py:def="moduleContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Run Scripts';
	</script>

<div py:if="serverRunning" class="content" id="content">

	<table id="scriptLibrary">
	<tr py:replace="tableHeader( 'Command Library' )"/>

	<!-- Main command area contents -->
	<tr>
		<td>
			<select size="12" id="scriptSelect"/>

			<div id="scriptInfoPane">
				<!-- right aligned script information -->
				<div id="scriptTitlePane"/><br/>
				<div id="scriptParamPane"/><br/>
				<form name="executeForm"
						action="javascript:RunScript.executeScriptFromForm()">
					<div id="scriptExecutePane"/>
				</form>
			</div>
		</td>
	</tr>

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
