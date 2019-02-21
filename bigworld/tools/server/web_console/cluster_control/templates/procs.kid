<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'layout.kid'"
	  py:extends="'common.kid'">

<div py:def="pageContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Manage Servers';
	</script>

	<div py:replace="displayProcs( procs, 'm', 'Processes for %s' % user.name )"/>

	<!-- Set up log link -->
	<script type="text/javascript">

		function gotoLog()
		{
			var q = new Query.Query();
			q.form.serveruser = "${user.name}";
			q.form.showMask = q.SHOW_ALL &amp; ~q.SHOW_USER &amp; ~q.SHOW_APPID;
			window.location = q.toURL( true );
		}
		
	</script>

	<p>
		<span py:if="user.getServerProcs() ">
			<a href="${tg.url( 'startproc', user=user.name)}">
				Start more processes</a>
			|
			<a href="${tg.url( 'restart', user=user.name )}">
				Restart the server</a>
			|
			<a py:if="areWorldProcessesRunning"
			   href="${tg.url( 'stop', user=user.name )}">
				Stop the server</a>

			<a py:if="not areWorldProcessesRunning" 
			   href="${tg.url( 'kill', user=user.name )}">
				Kill the server</a>
			|
			<script type="text/javascript" src="static/js/save.js"/>
			<a href="#" onclick="saveServerLayout( '${user.name}' )">
				Save this server layout</a>
			|
			<a href="javascript:gotoLog()">
				View log</a>
		</span>

		<span py:if="not user.getServerProcs()">
			<a href="${tg.url( 'start', user=user.name )}">
				Start the server
			</a>
		</span>
	</p>
	<span py:if="layoutErrors != None and len( layoutErrors )" class="alert">
	Server Layout Warnings:
	</span>
	<ul>
		<li py:for="error in layoutErrors" class="alert">${error}</li>
		
	</ul>
	
</div>

</html>
