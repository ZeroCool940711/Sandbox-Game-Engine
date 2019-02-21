<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python import cherrypy ?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#">

<head>
	<link rel="stylesheet" type="text/css" href="/static/css/top.css"/>
	<link rel="stylesheet" type="text/css" href="/static/css/navmenu.css"/>
	<link rel="stylesheet" type="text/css" href="/static/css/content.css"/>
	<link rel="stylesheet" type="text/css" href="/static/css/dojo.css"/>
	<link rel="shortcut icon" type="image/x-icon"
		  href="/static/images/favicon.ico"/>
	
	<title>Loading...</title>

	<!-- Standard JS includes we want everywhere -->
	<script type="text/javascript" src="${tg.tg_js}/MochiKit.js"/>
	<script type="text/javascript" src="/static/js/util.js"/>
	<script type="text/javascript" src="/static/js/ajax.js"/>
	<script type="text/javascript" src="/static/js/dom.js"/>
	<script type="text/javascript" src="/static/js/table.js"/>
	<script type="text/javascript" src="/static/js/async_task.js"/>
	<script type="text/javascript" src="/static/js/caps.js"/>

	<!-- Note: this isn't the IE method of controlling cache -->
	<meta http-equiv="cache-control" content="no-cache"/>

	<!-- Page specific CSS -->
	<?python
		try:
			page_specific_css
		except:
			page_specific_css = [ ]
	?>
	<link py:for="file in page_specific_css" href="${file}" type="text/css" rel="stylesheet" />

	<!-- Page specific Javascript -->
	<?python
		try:
			page_specific_js
		except:
			page_specific_js = [ ]
	?>
	<script py:for="file in page_specific_js" src="${file}" type="text/javascript"></script>
</head>

<body>

	<!--
		Simple login stuff.
	-->
	<?python
	  from turbogears import identity
	  from web_console.common import util, module
	  import web_console.root.controllers
	  import cherrypy

	  username = util.getSessionUsername()
	?>
	<table width="100%" class="logoheader">
		<tr>
			<td>
				<img src="/static/images/bigworld.png"
					 alt="BigWorld Web Console"/>
			</td>
			
			<td	align="right" valign="bottom">
				
				<div py:if="username" class="login">
					You are logged in as
					<a href="/cc/procs">${username}</a>
					(<a href="/logout">logout</a>)
				</div>

				<div py:if="not username" class="login">
					<script type="text/javascript">
						PAGE_TITLE = 'Login';
					</script>
					<form action="/login" method="post"><div>

						<!-- A quick note about these param names.  They have to
							 sync up with those given in dev.cfg:identity.form.*
							 otherwise the identity module won't intercept these
							 params and do the login properly -->
						<span id="loginbox">
						<label>Username:</label>
						<input name="login_username" size="13" class="text"/> 
						<label>Password:</label>
						<input name="login_password" type="password" size="13" class="text"/>
						<input type="submit" name="login_submit" value="Log In" class="login-button"/></span>
					</div></form>
				</div>
				
			</td>
		</tr>
	</table>

	<table class="mainpanel" width="100%" cellspacing="0" cellpadding="0"
		   py:if="username or cherrypy.request.path == '/login'"><tr>

		<!-- Navigation panel on the left -->
		<td py:if="username" valign="top" class="spaced">
			<div id="navigation" class="navmenu">
				<ul class="level-one">

					<li py:for="mod in module.Module.all()"
						py:if="mod.auth()"
						py:attrs="mod.attrs()">
						<a href="/${mod.path}"><img src="${mod.icon}" alt=""/>
						${mod.name}</a>
						<ul py:if="mod.isCurrent()" class="level-two">
							<li py:for="page in mod.pages"
								py:attrs="page.attrs()">
								<a py:if="not page.popup"
								   href="${page.url()}">${page.name}</a>
								<a py:if="page.popup"
								   href="#"
								   onclick="window.open( '${page.url()}' )">
									${page.name}</a>
							</li>
						</ul>
						
					</li>
				</ul>
			</div>
		</td>

		
		<!-- Main panel -->
		<td width="100%" height="100%" valign="top" class="main">

			<!-- Actual page content -->
			<div id="content" class="content" py:content="moduleContent()"/>

		</td>
	</tr>
	</table>

	<!-- Login page contents -->
	<div py:if="not username" py:strip="not username">
	
	<!-- Browser detection and warning -->
	<div id="bad_browser" style="display: none;">
		WARNING: This browser is not supported (<span id="browserVersion"></span>).<br/>
		Recommended browser is <a href="http://www.mozilla.org/products/firefox">
		Mozilla FireFox</a> 1.5 - 2.0.
	</div>
	
	<script py:if="not username" type="text/javascript" src="/static/js/browser_detect.js"/>
	<script py:if="not username" type="text/javascript"><![CDATA[
		// Detect whether a supported browser is being used
		if (!((BrowserDetect.browser == "Firefox" || 
					BrowserDetect.browser == "Mozilla") &&
				BrowserDetect.version >= 1.5))
		{
			browserVersionNode = document.getElementById( 'browserVersion' );
			browserVersionNode.innerHTML = BrowserDetect.browser + " " + 
				BrowserDetect.version;
			badBrowserNode = document.getElementById( 'bad_browser' );
			badBrowserNode.style.display = "block";
		}
		]]>
	</script>

	<!-- Introductory text -->
	<div id="login_intro">
	
	<h1 class="heading">BigWorld WebConsole</h1>
	
	<p>Welcome to the BigWorld WebConsole: the new web-based interface to
	BigWorld cluster management. </p>

	<h2>User Quickstart</h2>
	<p>If you do not already have an account, ask the system administrator to
	set one up for you; this must be done by the <b>admin</b> user.</p>
	
	<p>If you already have a user account, login through the <a href="#" 
		onClick="return false"
		onMouseOver="var loginBox = document.getElementById( 'loginbox' ); 
			loginBox.style.border = '3px solid #FAA61A'; 
			loginBox.style.backgroundColor = '#104D8C';
			loginBox.style.padding = '2em';
			" 
		onMouseOut="var loginBox = document.getElementById( 'loginbox' ); 
			loginBox.style.border = 'none'; 
			loginBox.style.background = 'transparent';
			loginBox.style.padding = '0';">login box</a> in the
	top-right corner of the screen.</p>

	<p>Once you are logged in, you will see the <i>ClusterControl</i> module by
	default. This module allows you to view the state of the cluster, and start
	and stop BigWorld server processes.  </p>
	
	<p>The navigation menu on the left will allow you to move between modules.
	Online help is available: look for the <i>Help</i> links in the navigation
	menu.</p>
	
	<?python
		#TODO: make this sysadmin quickstart section only appear if no other users 
		#other than admin exist
	?>
	<h2>System Administration Quickstart</h2>

	<p> If you are the system administrator and WebConsole has only just been
	installed, you will need to set up user accounts through the <b>admin</b>
	account. The default password for the admin account is
	'<code>admin</code>'. After logging in, make sure you change the
	administrator password. </p>
	
	<p>Create user accounts for BigWorld users to administrate their BigWorld
	server processes. </p>
	
	<p>For more details, refer to the <i>Server Tools Installation
	Guide</i>.</p>
	
	</div>

	</div>

	<!-- Call init methods of any modules that need it -->
	<script type="text/javascript">
		Table.init();
		document.title = PAGE_TITLE;
	</script>

</body>

</html>
