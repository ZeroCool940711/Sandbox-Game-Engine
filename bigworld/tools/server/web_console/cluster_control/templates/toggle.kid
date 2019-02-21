<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "pageHeader" ] = "Starting"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'layout.kid'"
	  py:extends="'common.kid'">
	
<body><div py:def="pageContent()">

	<?python
	  from turbojson import jsonify
	  if action == "start":
	    header = "Starting"
	  elif action == "stop":
	    header = "Stopping"
	  elif action == "restart":
	    header = "Restarting"
	?>

	<script type="text/javascript">
		PAGE_TITLE = "${header} The Server";
	</script>
	
	<table class="bordered">
		<tr py:replace="tableHeader( '%s The Server' % header )"/>
		<tr style="text-align: left;"> <!-- TODO: move this into CSS -->
			<th>Component</th>
			<th style="color: red">Dead</th>
			<th style="color: orange">Running</th>
			<th style="color: green">Registered</th>
			<th style="color: blue">Details</th>
		</tr>
		<tr py:for="pname in pnames">
			<td py:content="pname" id="${pname}_header"/>
			<td id="${pname}_dead"/>
			<td id="${pname}_running"/>
			<td id="${pname}_registered"/>
			<td id="${pname}_details"/>
		</tr>
	</table>

	<script type="text/javascript" src="static/js/toggle.js"/>
	<script type="text/javascript" src="/log/static/js/query.js"/>
	<script type="text/javascript">
		Toggle.layout = ${jsonify.encode( layout )};
		Toggle.id = ${id};
		Toggle.pnames = ${jsonify.encode( pnames )};
		Toggle.user = "${user.name}";
		Toggle.action = "${action}";
		Toggle.display();
		Toggle.follow();
	</script>

</div></body>

</html>
