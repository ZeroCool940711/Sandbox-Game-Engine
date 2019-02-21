<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

<head>
	<title>Web Console User Listing</title>
</head>

<body><div py:def="moduleContent()">
	
	<script type="text/javascript">
		PAGE_TITLE = 'User Accounts';
	</script>
	
	<table class="sortable">

		<tr py:replace="tableHeader( 'Users' )"/>
		<tr py:replace="colHeaders(
						('Username', 'Server User', 'Action') )"/>

		<tr py:for="user in users">

			<td>${user.user_name}</td>
			<td>${user.serveruser}</td>
			<td><div py:replace="actionsMenu( options[ user ] )"/></td>
			
		</tr>

	</table>

	<br/>
	
	<h3>NOTE</h3>

	<ul>
		<li>The <b>Username</b> field refers to the login name for the Web
		Console account.</li>
	
		<li>The <b>Server User</b> field refers to the UNIX user associated with
		this Web Console account (i.e. for running the server, querying logs etc).</li>
		
	</ul>
	
</div></body>

</html>
