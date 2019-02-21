<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "moduleHeader" ] = "SpaceViewer"
  layout_params[ "page_specific_css"] = ["/statg/static/css/stat_grapher.css"]

  
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

<body>
		<div py:def="moduleContent()">

		<script type="text/javascript" src="/static/js/dojo/dojo.js"/>
			
		<script type="text/javascript">
			PAGE_TITLE = 'SpaceViewer';
			djConfig.baseRelativePath = "/static/js/dojo/";
			dojo.require( "dojo.widget.Dialog" );
		</script>

		<script type="text/javascript" src="/sv/static/js/exception.js"></script>
		<script type="text/javascript" src="/sv/static/js/flashtag.js"></script>
		<script type="text/javascript" src="/sv/static/js/flashserializer.js"></script>
		<script type="text/javascript" src="/sv/static/js/flashproxy.js"></script>
		<script type="text/javascript" src="/sv/static/js/legend.js"></script>
		<script type="text/vbscript" src="/sv/static/js/vbcallback.vbs"></script>
		<script type="text/javascript" src="/sv/static/js/swfobject.js"></script>

		<table  id="graphtable">
		<tr>
			<td id="flashCell" >
				<div id="flashDiv">
				Flash Player 9 or higher required. Go
				<a href="http://www.adobe.com/go/getflashplayer">here</a>
				to get it!

				<script type="text/javascript">
					var flashId = new Date().getTime();
					var flashProxy = new FlashProxy( flashId, 
							'/sv/static/swf/javascriptflashgateway.swf' );
					var so = new SWFObject("/sv/static/swf/DeleteImages.swf","DeleteImages",
							'100%', '100%', "9", "#FFFFFF" );
					 so.addVariable('UserIn', "$user");
					so.addVariable( 'flashId', flashId );
					so.addVariable( 'serviceURL', "$serviceURL" );
					so.addParam( 'align', 'middle' );
					so.addParam( 'width', '100%' );
					so.addParam( 'height', '100%' );
					so.addParam( 'id', 'DeleteImages' );
					so.addParam('flashId', flashId);					
					so.write("flashDiv");
				</script>
			</div>
			</td>
				<td id="legendCell" class = "legend" >
				<div id="legendDiv">
				</div>
			</td>
			
		</tr>
		</table>
			
		</div>

</body>

</html>
