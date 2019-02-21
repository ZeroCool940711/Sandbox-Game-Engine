<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "moduleHeader" ] = "Error"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

<head>
	<title> Error </title>
</head>
	
<body><div py:def="moduleContent()">

	<?python
	  from web_console.common import util
	?>
	
	<h2 style="color: red">
		<span py:if="msg">ERROR: ${msg}</span>
		<span py:if="not msg">ERROR</span>
	</h2>
	
	<div id="errors">
		<div py:for="d in debug">
			<code style="color: red">${d}</code>
		</div>
	</div>
	
</div></body>
</html>
