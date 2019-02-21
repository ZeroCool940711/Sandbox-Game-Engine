<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "pageHeader" ] = "Exception Raised"
  layout_params[ "moduleHeader" ] = "Exception Raised"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">
	
<head>
	<title> Exception </title>
</head>

<body>
<div py:def="moduleContent()">

	<h1>Exception Raised</h1>
	<h3>${time}</h3>
	<code>${exception}</code>

	<div id="notrace">
		<h2><a href="#" onClick="Util.toggleVisibility('notrace','stacktrace');">View stack trace</a></h2>
	</div>

	<!--
	explicitly setting the style for hiding here as there doesn't
	appear to be a generic "hide element" style
	-->
	<div id="stacktrace" class="hidden" style="display: none">
		<h2><a href="#" onClick="Util.toggleVisibility('notrace','stacktrace');">Hide stack trace</a></h2>
		<font size="-1">
			<pre>${traceback}</pre>
		</font>
	</div>
</div>
</body>
</html>
