<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "pageHeader" ] = "${error}"
  layout_params[ "moduleHeader" ] = "${error}"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">
	
<head>
	<title> ${error} </title>
</head>

<body>
<div py:def="moduleContent()">
	<h1>${error}</h1>
	<p>${message}</p>
</div>
</body>
</html>
