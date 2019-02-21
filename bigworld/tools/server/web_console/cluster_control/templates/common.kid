<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:extends="'../../common/templates/common.kid'">

<head>
	<title> Library Functions </title>
</head>
	
<body>

	<!--
		Display a table of processes.
	-->
	<div py:def="displayProcs( procs, flags, header = None )">

		<table py:if="procs" class="sortable">
			<?python
			  from web_console.common import caps
			  showMachine = "m" in flags
			  showUser = "u" in flags
			?>

			<tr py:if="header" py:replace="tableHeader( header )"> <td/> </tr>
			
			<tr class="sortrow">
				<td class="colheader"> Process Name </td>
				<td class="colheader" py:if="showMachine"> Machine </td>
				<td class="colheader" py:if="showUser"> <b> User </b> </td>
				<td class="colheader"> CPU Load </td>
				<td class="colheader"> Memory Usage </td>
				<td class="colheader"> PID </td>
				<td class="colheader"> Action </td>
			</tr>
			
			<tr py:for="p in procs">

				<!-- Get a reference to the User object for this proc -->
				<?python user = p.machine.cluster.getUser( p.uid ) ?>
				
				<td>
					<a href="${tg.url( 'watcher', machine=p.machine.name, pid=p.pid )}">
						${p.label()}
					</a>
				</td>
				
				<td py:if="showMachine">
					<a href="${tg.url( 'machine', machine=p.machine.name )}"
					   py:content="p.machine.name"/>
				</td>

				<td py:if="showUser">
					<a href="${tg.url( 'procs', user=user.name )}"
					   py:content="user.name"/>
				</td>
				
				<td py:content="'%.1f%%' % (p.load * 100)" class="numeric"/>
				<td py:content="'%.1f%%' % (p.mem * 100)" class="numeric"/>
				<td py:content="p.pid" class="numeric"/>
				<td py:content="actionsMenu( caps.get( p ) )"/>
				
			</tr>
		</table>

		<div py:if="not procs">
			No processes running!
		</div>
	</div>

	<!--
		Display a table of machines.
	-->
	<div py:def="displayMachines( machines, header=None )">

		<?python
		  from pycommon import messages
		  
		  if machines:
		    machines[0].cluster.queryTags()
			
		  maxVersion = max( [m.machinedVersion for m in machines] )
		?>
		
		<table class="sortable">

			<tr py:if="header" py:replace="tableHeader( header )"> <td/> </tr>
			
			<tr py:replace="colHeaders( [
				'Hostname', 'IP Address', 'CPU Load', 'Mem Usage', 'Speed (MHz)',
				'CPUs', 'Processes', 'Groups', 'Components', 'Machined Version'] )"/>
		
			<tr py:for="m in machines">

				<?python
				  if m.tags.has_key( "Groups" ):
				    groups = m.tags[ "Groups" ]
				  else:
				    groups = []

				  if m.tags.has_key( "Components" ):
				    components = " ".join( [ s.lower() for s in m.tags[ "Components" ] ] )
				    if not components:
				      components = "<none>"
				  else:
				    components = "<all>"
				?>
			
				<td>
					<a href="${tg.url( 'machine', machine=m.name )}"
					   py:content="m.name"/>
				</td>
			
				<td py:content="m.ip"/>
				
				<td py:content="', '.join( ['%.1f%%' % (l * 100) for l in m.loads] )"
					class="numeric"/>

				<td py:content="'%.1f%%' % (m.mem * 100)" class="numeric"/>
				<td py:content="m.mhz" class="numeric"/>
				<td py:content="m.ncpus" class="numeric"/>
				<td py:content="len( m.getProcs() )" class="numeric"/>
				<td>
					<span py:for="grp in groups">
						<a href="${tg.url( 'machines', group=grp )}">${grp}</a> 
					</span>
				</td>
				<td py:content="components"/>
				<td class="numeric">
					<span py:if="m.machinedVersion &lt; maxVersion" class="alert"
						  py:content="m.machinedVersion"/>
					<span py:if="m.machinedVersion == maxVersion"
						  py:content="m.machinedVersion"/>
				</td>
			</tr>
		</table>

		<p py:if="messages.MachineGuardMessage.MACHINED_VERSION &lt; maxVersion"
			 class="alert">
			WARNING: The Web Console thinks that the current bwmachined version
			is ${messages.MachineGuardMessage.MACHINED_VERSION}, yet there are
			machines on the network up to version ${maxVersion}.
		</p>
	</div>

</body>
</html>
