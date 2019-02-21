<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "pageHeader" ] = "Start Processes"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'layout.kid'"
	  py:extends="'common.kid'">
	
<div py:def="pageContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Start More Processes';
	</script>

	<?python
	  from pycommon import cluster
	?>
	
	<select py:def="selectList( name, options, selected )"
			name="${name}" style="width: 100%">
			<loop py:for="o in options">
				<option py:if="o == selected"
						selected="selected">${o}</option>
				
				<option py:if="o != selected">${o}</option>
			</loop>
	</select>
	
	<table class="bordered">
		<tr py:replace="tableHeader( 'Starting as' )"/>
		<tr><td>User</td><td>${user.name}</td></tr>
		<tr><td>UID</td><td>${user.uid}</td></tr>
	</table>

	<p/>

	<form action="${tg.url( 'startproc' )}">
		<div><input name="user" type="hidden" value="${user.name}"/></div>
		<table class="bordered">
			<tr py:replace="tableHeader( 'Start' )"/>
			<tr>
				<td>
					Server component:
				</td>
				<td py:content="selectList( 'pname',
								cluster.Process.ALL_PROCS, prefs[ 'proc' ] )"/>
			</tr>
			<tr>
				<td>
					Machine:
				</td>
				<td py:content="selectList( 'machine',
								[m.name for m in machines], prefs[ 'machine' ] )"/>
			</tr>

			<tr>
				<td>
					<label>Number of processes:</label>
				</td>
				<td>
					<input type="text" name="count" value="${prefs[ 'count' ]}"/>
				</td>
			</tr>

			<tr>
				<td/>
				<td colspan="2">
					<input type="submit" value="Go!"/>
				</td>
			</tr>			
		</table>
	</form>
	
</div>
</html>
