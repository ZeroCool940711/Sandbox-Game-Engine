<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<?python
  layout_params[ "moduleHeader" ] = "Log Viewer"
?>

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

<div py:def="moduleContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Search Logs';
	</script>

	<form action="javascript:staticFetch()" method="post" name="filters"
		  class="logviewer">
		
		<table id="filterstable" class="hideable.0.0.1.0">

		<tr py:replace="tableHeader( 'Log Filters' )"/>

		<tr><td class="group"><table class="innertable">
		<tr class="first">
			<td>
				<span class="field">Time</span>
				<span class="show">
					<input type="checkbox" name="show_date"
							title="Toggle whether the date is included in the output"/>
					Date
					<input type="checkbox" name="show_time"
							title="Toggle whether the time is included in the output"/>
					Time
				</span>
			</td>
			
			<td>
				<input type="text" size="21" name="time"
				   CLASS="calendardatepicker requiredfield" id="time"/>

				<img src="static/images/backwards.png"
					 onclick="document.filters.time.value = '(beginning of log)'"
					 title="Search from the beginning of the log"/>
				
				<img src="static/images/server.png"
					 onclick="document.filters.time.value = '(server startup)'"
					 title="Search from last server startup"/>
				
				<img src="static/images/calendar.png" id="time_trigger"
					 class="time_button"
					 title="Specify a time to search from using a calendar"/>

				<img src="static/images/search.png"
					 onclick="searchFromTime();"
					 title="Search from the last occurrence of a particular message"/>
				
				<img src="static/images/forwards.png"
					 onclick="document.filters.time.value = '(now)'"
					 title="Search from the current time"/>
			</td>
			
		    <script type="text/javascript">
		    Calendar.setup(
		    {
		        inputField : "time",
		        ifFormat : "%a %d %b %Y %H:%M:%S",
		        button : "time_trigger",
		        showsTime : true
		    }
		    );
			</script>
			
		</tr>
		
		<tr>
			
			<td class="colheader">
				<span class="field">Period</span>
			</td>
			<td>
				<input type="text" size="2" name="periodCount"/>
				<select name="periodUnits">
					<option value="seconds">seconds</option>
					<option value="minutes">minutes</option>
					<option value="hours">hours</option>
					<option value="days">days</option>
				</select>

				<select name="periodDirection">
					<option value="backwards" onclick="Query.setPeriodDisabled()">
						backwards
					</option>
					<option value="forwards" onclick="Query.setPeriodDisabled()">
						forwards
					</option>
					<option value="either side" onclick="Query.setPeriodDisabled()">
						either side
					</option>
					<option value="to beginning" onclick="Query.setPeriodDisabled()">
						to beginning
					</option>
					<option value="to present" onclick="Query.setPeriodDisabled()">
						to present
					</option>
				</select>
			</td>
			
		</tr>
		
		<tr>
			<td>
				<span class="field">Machine</span>
				<span class="show">
					<input type="checkbox" name="show_host"
							title="Toggle whether the hostname is included in the output"/>Show
				</span>
			</td>
			<td>
				<select name="host">
					<option value="">(all machines)</option>
					<option py:for="hostname in hostnames"
							value="${hostname}">${hostname}</option>
				</select>
			</td>
		</tr>
		
		<tr>
			
			<td>
				<span class="field">Username</span>
				<span class="show">
					<input type="checkbox" name="show_serveruser"
							title="Toggle whether the username is included in the output"/>Show
				</span>
			</td>

			<td>
				<select name="serveruser">
					<option py:for="user in users"
							value="${user}">${user}</option>
				</select>
			</td>
			
		</tr>
		
		<tr>
			<td>
				<span class="field">PID</span>
				<span class="show">
					<input type="checkbox" name="show_pid"
							title="Toggle whether the PID is included in the output"/>Show
				</span>
			</td>
			<td>
				<input type="text" size="22" name="pid"
					   title="The process ID to filter by, or leave blank for any PID"/>
			</td>
		</tr>

		<tr>
			<td>
				<span class="field">App ID</span>
				<span class="show">
					<input type="checkbox" name="show_appid"
							title="Toggle whether the App ID is included in the output"/>Show
				</span>
			</td>
			<td>
				<input type="text" size="22" name="appid"
					   title="The AppID to filter by, (e.g. '1' for cellapp01) or leave blank for any AppID"/>
			</td>
		</tr>

		</table></td>

		<td class="group"><table class="innertable2">

		<tbody><tr>
			<td class="first">
				<span class="field">Process</span>
				<span class="show">
					<input type="checkbox" name="show_procs"
							title="Toggle whether the process name is included in the output"/>Show
				</span>
			</td>
			<td>
				<span class="field">Severity</span>
				<span class="show">
					<input type="checkbox" name="show_severity"
							title="Toggle whether the message level is included in the output"/>Show
				</span>
			</td>
			<td>
				<span class="field">Message Filter</span>
			</td>
		</tr>
		<tr style="vertical-align: top">
			<td class="first">
				<select name="procs" multiple="multiple"
						title="Output is shown for selected processes"
						size="9">
					<option py:for="name in components"
							value="${name}">
						${name}
					</option>
				</select>
			</td>

			<td>
				<select name="severity" multiple="multiple"
						title="Output is shown for selected log levels"
						size="9">
					<option py:for="s in severities"
							value="${s}">
						${s}
					</option>
				</select>
			</td>
			<td>
				<code>+</code>
				<input type="text" size="16" name="message"
					   title="Inclusion pattern to match in log message, or blank for all messages"/>
				<p/>
				
				<code>-</code>
				<input type="text" size="16" name="exclude"
					   title="Exclusion pattern to match in log message, or blank for all messages"/>
				
				<p style="font-weight: normal">
					
					<input type="checkbox" name="caseSens"
						   title="Toggle case-sensitivity of the message filter"/>
					Case-sensitive<br/>
					
					<input type="checkbox" name="regex"
						   title="Treat the message filter as a regular expression"/>
					Regex match<br/>
					
					<input type="checkbox" name="interpolate"
						   title="Interpolate log messages before filtering; see help for more info"/>
					Pre-interpolate
				</p>
			</td>
		</tr>
		</tbody>
		</table></td></tr>
		
		<tr>
			<td colspan="2">

				<span style="float: left">
					
					<button name="fetch"
						   title="Search for log entries matching the filter criteria">
						  <b>Fetch</b>
					</button>
					
					<input type="button" name="live" value="Live Output"
						   onclick="toggleLive();" style="font-weight: bold"
						   title="Show continuously updating output for log entries matching the current filter criteria"/>

					<input type="text" size="1" name="context"
						   title="Lines of context to display around matching entries"/>
					Lines of context

					<input type="checkbox" name="autoHide"
						   title="Automatically hide the table of filters when 'Fetch' or 'Live Output' is clicked"/>
					Auto-hide settings
					
				</span>

				<span style="float: right">

				<select py:replace="actionsMenu( menuOptions,
									help = 'Save, load, and delete filter settings' )"/>
					
				</span>

			</td>
		</tr>

	</table></form>

	<p/>

	<input type="hidden" id="mlstatus" name="mlstatus" value="${mlstatus}" />
	<div id="outputGoesHere"/>

	<script type="text/javascript">
		var serverTimeOffset = ${now} - Date.now()/1000.0;
	</script>
	<script type="text/javascript" src="/log/static/js/server_time.js"/>
	<script type="text/javascript" src="/log/static/js/output_pane.js"/>
	<script type="text/javascript" src="/log/static/js/query.js"/>
	<script type="text/javascript" src="/log/static/js/log_viewer.js"/>
	<script type="text/javascript">
		if ("${mlnotify}" == "True")
		{
			Util.warning( "Message Logger is not running" );
		}

		gLastAnnotation = "${annotation}";
	</script>
	
</div>

</html>
