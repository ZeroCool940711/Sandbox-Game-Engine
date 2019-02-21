<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

<div py:def="moduleContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Delete User Account';
	</script>
	
	<div>
		Do you really want to delete ${username}?
		
		<form action="" method="post"><p>
			
			<input type="submit" value="Yes"/>
			<input type="hidden" name="username" value="${username}"/>
			<input type="hidden" name="confirmed" value="true"/>

			<input type="button" value = "No"
				   onclick="window.location = 'list'"/>
			
		</p></form>

	</div>

</div>

</html>
