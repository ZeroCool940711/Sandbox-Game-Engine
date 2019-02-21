<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "moduleHeader" ] = "Log Viewer"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

<div py:def="moduleContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Live Output';
	</script>

	<input type="hidden" id="mlstatus" name="mlstatus" value="${mlstatus}" />
	<div id="outputGoesHere"/>
	
	<script type="text/javascript">
		var serverTimeOffset = ${now} - Date.now()/1000.0;
		var serveruser = "${serveruser}";
	</script>
	<script type="text/javascript" src="/log/static/js/server_time.js"/>
	<script type="text/javascript" src="/log/static/js/output_pane.js"/>
	<script type="text/javascript" src="/log/static/js/query.js"/>
	<script type="text/javascript" src="/log/static/js/live.js"/>
	<script type="text/javascript">
		if ("${mlnotify}" == "True")
		{
			Util.warning( "Message Logger is not running" );
		}
	</script>

</div>
</html>
