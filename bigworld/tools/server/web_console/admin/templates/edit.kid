<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

<head>
	<title>Add A New User</title>
</head>

<body><div py:def="moduleContent()">

	<?python
	  import cherrypy
	?>
	
	<script type="text/javascript">
		PAGE_TITLE = "${cherrypy.request.params[ 'action' ].capitalize()} User Account";
	</script>
	
	<form action="" method="post">

		<table class="bordered">
			
			<tr py:if="cherrypy.request.params[ 'action' ] == 'add'"
				py:replace="tableHeader( 'Add A New User' )"/>

			<tr py:if="cherrypy.request.params[ 'action' ] == 'edit'"
				py:replace="tableHeader( 'Edit User Details' )"/>
			
			<tr>
				<td class="colheader">Username</td>
				<td>
					<input py:if="not user" name="username"/>
					<input py:if="user" name="username"
						   value="${user.user_name}"/>
				</td>
			</tr>
			
			<tr>
				<td class="colheader">Password</td>
				<td><input type="password" name="pass1"/></td>
			</tr>
			
			<tr>
				<td class="colheader">Confirm Password</td>
				<td><input type="password" name="pass2"/></td>
			</tr>
			
			<tr>
				<td class="colheader">Server User</td>
				<td>
					<input py:if="not user" name="serveruser"/>
					<input py:if="user" name="serveruser"
						   value="${user.serveruser}"/>
				</td>
			</tr>

			<tr><td colspan="2">
				<input py:if="user" type="hidden" name="id" value="${user.id}"/>
				<input py:if="not user" type="submit" value="Add User"/>
				<input py:if="user" type="submit" value="Commit Changes"/>
			</td></tr>

		</table>
	</form>
	
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
