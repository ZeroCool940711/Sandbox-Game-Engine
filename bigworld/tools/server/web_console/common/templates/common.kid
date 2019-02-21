<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python import sitetemplate ?>
<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:extends="sitetemplate">

<head>
	<title> Library Functions </title>
</head>
	
<body>

	<!--
		Hidden form element for passing current page and params for doing
		redirects later on.
	-->
	<div py:def="hiddenRedirect()">
		<?python
		  import cherrypy
		?>
		<input type="hidden" name="redirectTo"
			   value="${tg.url( cherrypy.request.path, **cherrypy.request.params )}"/>
	</div>

	<!--
		Insert a header row into a table
	-->
	<table>
		<tr py:def="tableHeader( text )">
			<th colspan="100" class="heading">${text}</th>
		</tr>
	</table>

	<!--
		Insert column headers into a table
	-->
	<table>
		<tr py:def="colHeaders( hdrs )" class="sortrow">
			<td py:for="h in hdrs" py:content="h" class="colheader"/>
		</tr>
	</table>

	
	<!--
		A GMail-style actions menu with support for option groups.  Please see
		actionMenuAppend() and clearChildren() in dom.js for ways of
		dynamically modifying these menus at runtime.
		
		You must pass an instance of web_console.common.util.ActionMenuOptions
		as the first argument to this template.  Please see that module for more
		info about constructing the option list.
	-->
	<form action=""><p>

		<select py:def="actionsMenu( options, help = '' )"
				title='${help}'
				style="width: 100%">

			<div py:for="g in options.groupOrder" py:strip="True">

				<option py:if="g == options.groupOrder[0]"
						style="color: #666">
					${g.name}
				</option>

				<option py:if="g != options.groupOrder[0]"
						style="color: #666" disabled="disabled">
					${g.name}
				</option>
				
				<optgroup label="" id="${g.id}">

					<option py:for="label, script, help in g.options"
							onclick="${script};
									 this.parentNode.parentNode.selectedIndex=0"
							title="${help}">
						${label}
					</option>
					
				</optgroup>

			</div>

		</select>
		
	</p></form>
	

</body>
</html>
