<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "pageHeader" ] = "Start The Server"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'layout.kid'"
	  py:extends="'common.kid'">
	
<body>
	<!-- Macro definitions -->
	<select py:def="selectList( name, options, selected )"
			name="${name}" onchange="onSelectMachines( this )">
			<loop py:for="o in options">
				<option py:if="o in selected"
						selected="selected">${o}</option>
				
				<option py:if="o not in selected">${o}</option>
			</loop>
	</select>

	<!-- Document body -->
	<div py:def="pageContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Start The Server';
	</script>

	<?python
	  groups = c.getGroups().keys()
	  groups.append( "(use all machines)" )
	  
	  machines = c.getMachines()
	  machines.sort()
	?>

	<table class="bordered">
		<tr py:replace="tableHeader( 'Server Environment' )"/>
		<tr><td class="colheader">User</td><td>${user.name}</td></tr>
		<tr><td class="colheader">UID</td><td>${user.uid}</td></tr>
		<tr><td class="colheader">MF_ROOT</td><td><code id="ccStartMFRoot"> </code></td></tr>
		<tr><td class="colheader">BW_RES_PATH</td><td><code id="ccStartBWResPath"> </code></td></tr>
		
		<tr py:if="user.coredumps">
			<td class="colheader alert">Core Dumps</td>
			<td><a href="${tg.url( 'coredumps', user=user.name )}">
					${len( user.coredumps )}
			</a></td>
		</tr>
	</table>

	<p/>

	<form action="${tg.url( 'doStart' )}" name="startForm">
		<div><input name="user" type="hidden" value="${user.name}"/></div>
		<table class="bordered">
			<tr py:replace="tableHeader( 'Start' )"/>
			<tr>
				<td>
					<input type="radio" name="mode" value="single"
						   id="ccStartMachineMode" checked="checked"
						   onchange="onSelectMachines( this.form.machine )"/>
				</td>
				<td>
					On a single machine:
				</td>
				<td>
					<select py:replace="selectList( 'machine',
						[m.name for m in machines], prefs.arg )"/>
				</td>
			</tr>

			<tr>
				<td>
					<input type="radio" name="mode" value="layout"
						   id="ccStartLayoutMode"
						   onchange="onSelectMachines( this.form.layout )"/>
				</td>
				<td>
					From a saved layout:
				</td>
				<td>
					<select py:replace="selectList( 'layout',
										savedLayouts, prefs.arg )"/>
				</td>
			</tr>

			<tr>
				<td>
					<input type="radio" name="mode" value="group"
						   id="ccStartGroupMode"
						   onchange="onSelectMachines( this.form.group )"/>
				</td>
				<td>
					On a group of machines:
				</td>
				<td>
					<select py:replace="selectList( 'group',
										groups, prefs.arg )"/>
				</td>
			</tr>
			<tr>
				<td/>
				<td>
					<label>Restrict components by tags:</label>
				</td>
				<td>
					<input type="checkbox" name="restrict" value="true"
						   id="ccRestrictFlag" checked="checked" disabled="true"/>
				</td>
			</tr>

			<tr>
				<td/>
				<td colspan="2">
					<input type="submit" id="ccStartSubmit" value="Go!"/>
				</td>
			</tr>			
		</table>
	</form>

	<!-- JavaScript stuff -->
	<script type="text/javascript" src="static/js/start.js"/>
	<script type="text/javascript">
		var username = "${user.name}";

		// Disable modes that don't make sense
		if (${len( groups )} == 0)
		{
			$("ccStartGroupMode").disabled = true;
			document.startForm.group.disabled = true;
		}
		
		if (${len( savedLayouts )} == 0)
		{
			$("ccStartLayoutMode").disabled = true;
			document.startForm.layout.disabled = true;
		}
		
		if ("${prefs.mode}" == "single")
			$("ccStartMachineMode").checked = true;
		else if ("${prefs.mode}" == "group" &amp;&amp; !$("ccStartGroupMode").disabled)
			$("ccStartGroupMode").checked = true;
		else if (!$("ccStartLayoutMode").disabled)
			$("ccStartLayoutMode").checked = true;
			
		onSelectMachines( null );
	</script>

</div></body>
</html>
