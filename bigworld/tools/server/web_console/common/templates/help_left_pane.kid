<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:extends="'common.kid'">

<head>
	<link href="/static/css/top.css" rel="stylesheet" type="text/css" />
	<link href="/static/css/help.css" rel="stylesheet" type="text/css" />
</head>
<body>
	<table cellpadding="0" cellspacing="0" class="helpmainpanel">
	<tr>
	<td class="spaced" valign="top">
	<div id="navigation" class="helpnavmenu">
		<ul class="level-one-help">

		<li py:if="section != 'cluster_control'" class="top-level current">
		<!-- CLUSTER CONTROL COLLAPSED -->
			<a href="" onclick="window.open('/cc/help','_parent');"><img src="/static/images/cluster_control.png" alt=""/>Cluster Control Help</a>
		</li>

		<li py:if="section == 'cluster_control'" class="top-level current">
		<!-- CLUSTER CONTROL EXPANDED -->
			<a href="/cc/static/html/help_right_pane.html#Top"
				target="right_pane"><img src="/static/images/cluster_control.png" alt=""/>Cluster Control Help</a>
			<ul class="level-two-help">
			<li class="not-current">
				<a href="/cc/static/html/help_right_pane.html#ManagingServers"
				   target="right_pane">Managing servers</a>
				<ul class="level-three-help">
				<li class="not-current">
					<a href="/cc/static/html/help_right_pane.html#StartingTheServer"
						target="right_pane">Starting the server</a>
				</li>
				<li class="not-current">
					<a href="/cc/static/html/help_right_pane.html#RestartingTheServer"
						target="right_pane">Restarting the server</a>
				</li>
				</ul>
			</li>
			<li class="not-current">
				<a href="/cc/static/html/help_right_pane.html#BrowsingMachines"
					target="right_pane">Browsing machines</a>
			</li>
			<li class="not-current">
				<a href="/cc/static/html/help_right_pane.html#BrowsingUsers"
					target="right_pane">Browsing users</a>
			</li>
			<li class="not-current">
				<a href="/cc/static/html/help_right_pane.html#BrowsingWatcherValues"
					target="right_pane">Browsing watcher values</a>
			</li>
			<li class="not-current">
				<a href="/cc/static/html/help_right_pane.html#SeeAlso"
					target="right_pane">See also</a>
			</li>
			<li class="not-current">
				<a href="/cc/static/html/help_right_pane.html#Caveats"
					target="right_pane">Caveats</a>
			</li>
			</ul>
		</li>

		<li py:if="section != 'log_viewer'" class="top-level current">
		<!-- LOG VIEWER COLLAPSED -->
			<a href="" onclick="window.open( '/log/help', '_parent' );"><img src="/static/images/log_viewer.png" alt=""/>Log Viewer Help</a>
		</li>

		<li py:if="section == 'log_viewer'" class="top-level current">
		<!-- LOG VIEWER EXPANDED -->
			<a href="/log/static/html/help_right_pane.html#Top" target="right_pane"><img src="/static/images/log_viewer.png" alt=""/>Log Viewer Help</a>
			<ul class="level-two-help">
				<li class="not-current">
					<a href="/log/static/html/help_right_pane.html#DisplayingOutput"
						target="right_pane">Displaying output</a>
				</li>
				<li class="not-current">
					<a href="/log/static/html/help_right_pane.html#Searching"
						target="right_pane">Searching</a>

					<ul class="level-three-help">
					<li class="not-current">
						<a href="/log/static/html/help_right_pane.html#LogFilters"
							target="right_pane">Log filters</a>
					</li>
					<li class="not-current">
						<a href="/log/static/html/help_right_pane.html#QueryModes"
							target="right_pane">Query modes</a>
					</li>
					<li class="not-current">
						<a href="/log/static/html/help_right_pane.html#LinesOfContext"
							target="right_pane">Lines of context</a>
					</li>
					<li class="not-current">
						<a href="/log/static/html/help_right_pane.html#SavedFilterSettings"
							target="right_pane">Saved filter settings</a>
					</li>
					<li class="not-current">
						<a href="/log/static/html/help_right_pane.html#ConservingScreenRealEstate"
							target="right_pane">Conserving screen real estate</a>
					</li>
					</ul>
				</li>
				<li class="not-current">
					<a href="/log/static/html/help_right_pane.html#LiveOutputPage"
						target="right_pane">Live Output page</a>
				</li>
				<li class="not-current">
					<a href="/log/static/html/help_right_pane.html#UsageSummaryPage"
						target="right_pane">Usage Summary page</a>
				</li>
				<li class="not-current">
					<a href="/log/static/html/help_right_pane.html#SeeAlso"
						target="right_pane">See also</a>
				</li>
			</ul>
		</li>

		<li py:if="section != 'stat_grapher'" class="top-level current">
		<!-- STATGRAPHER COLLAPSED -->
			<a href="" onclick="window.open( '/statg/help', '_parent' );"><img src="/static/images/stat_grapher.png" alt=""/>Stat Grapher Help</a>
		</li>

		<li py:if="section == 'stat_grapher'" class="top-level current">
		<!-- STATGRAPHER EXPANDED -->
			<a href="/statg/static/html/help_right_pane.html#Top" target="right_pane"><img src="/static/images/stat_grapher.png" alt=""/>Stat Grapher Help</a>
			<ul class="level-two-help">
			<li class="not-current">
				<a href="/statg/static/html/help_right_pane.html#StatLoggerLogDatabases"
					target="right_pane">StatLogger log databases</a>
			</li>
			<li class="not-current">
				<a href="/statg/static/html/help_right_pane.html#ChoosingGraphs"
					target="right_pane">Choosing graphs</a>
			</li>
			<li class="not-current">
				<a href="/statg/static/html/help_right_pane.html#BasicOperation"
					target="right_pane">Basic operation</a>
				<ul class="level-three-help">
				<li class="not-current">
					<a href="/statg/static/html/help_right_pane.html#TheGraphDisplay"
						target="right_pane">The Graph display</a>
				</li>
				<li class="not-current">
					<a href="/statg/static/html/help_right_pane.html#Panning"
						target="right_pane">Panning</a>
				</li>
				<li class="not-current">
					<a href="/statg/static/html/help_right_pane.html#ViewRange"
						target="right_pane">View Range</a>
				</li>
				<li class="not-current">
					<a href="/statg/static/html/help_right_pane.html#SnappingToLogEnd"
						target="right_pane">Snapping to log end</a>
				</li>
				<li class="not-current">
					<a href="/statg/static/html/help_right_pane.html#TheVelocityScroller"
						target="right_pane">The Velocity Scroller</a>
				</li>
				<li class="not-current">
					<a href="/statg/static/html/help_right_pane.html#TheLegend"
						target="right_pane">The Legend</a>
					<ul class="level-four-help">
						<li class="not-current">
							<a href="/statg/static/html/help_right_pane.html#StatisticNumericalValues"
								target="right_pane">Statistic numerical values</a>
						</li>
						<li class="not-current">
							<a href="/statg/static/html/help_right_pane.html#HighlightingStatistics"
								target="right_pane">Highlighting statistics</a>
						</li>
						<li class="not-current">
							<a href="/statg/static/html/help_right_pane.html#ChangingTheColourOfAStatistic"
								target="right_pane">Changing the colour of a
							statistic</a>
						</li>
						<li class="not-current">
							<a href="/statg/static/html/help_right_pane.html#StatisticsPreferences"
								target="right_pane">Statistics preferences</a>
						</li>
					</ul>
				</li>
				</ul>
			</li>
			</ul>
		</li>

		<li py:if="section != 'console'" class="top-level current">
		<!-- PYTHON CONSOLE COLLAPSED -->
			<a href="" onclick="window.open( '/console/help', '_parent' );"><img src="/static/images/console.png" alt=""/>Python Console Help</a>
		</li>
		<li py:if="section == 'console'" class="top-level current">
		<!-- PYTHON CONSOLE EXPANDED -->
			<a href="/console/static/html/help_right_pane.html#Top"
				target="right_pane"><img src="/static/images/console.png" alt=""/>Python Console Help</a>
			<ul class="level-two-help">
				<li class="not-current">
				<a href="/console/static/html/help_right_pane.html#pyConsole"
					target="right_pane">Python console</a>
				<ul class="level-three-help">
				<li class="not-current">
					<a href="/console/static/html/help_right_pane.html#Usage"
						target="right_pane">Usage</a>
				</li>
				</ul>
				</li>
			</ul>
		</li>

		<li py:if="section != 'commands'" class="top-level current">
		<!-- COMMANDS COLLAPSED -->
			<a href="" onclick="window.open( '/commands/help', '_parent' );"><img src="/static/images/console.png" alt=""/>Commands Help</a>
		</li>
		<li py:if="section == 'commands'" class="top-level current">
		<!-- COMMANDS EXPANDED -->
			<a href="/commands/static/html/help_right_pane.html#Top"
				target="right_pane"><img src="/static/images/console.png" alt=""/>Commands Help</a>
			<ul class="level-two-help">
				<li class="not-current">
				<a href="/commands/static/html/help_right_pane.html#Commands"
					target="right_pane">My commands</a>
				<ul class="level-three-help">
				<li class="not-current">
					<a href="/commands/static/html/help_right_pane.html#Usage"
						target="right_pane">Usage</a>
				</li>
				</ul>
				</li>
			</ul>
		</li>

		</ul>
	</div>
	</td>
	</tr>
	</table>
</body>
</html>
