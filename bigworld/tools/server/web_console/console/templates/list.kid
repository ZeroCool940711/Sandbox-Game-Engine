<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:layout="'../../common/templates/layout.kid'"
	  py:extends="'../../common/templates/common.kid'">

<div py:def="moduleContent()">

	<script type="text/javascript">
		PAGE_TITLE = 'Python Console';
	</script>

	<table class="sortable">
		
		<tr py:replace="tableHeader( 'Console-Enabled Processes For %s' % user.name )"/>
		<tr py:replace="colHeaders( ('Process Name', 'Machine', 'Action') )"/>

		<tr py:for="p in procs">

			<td py:content="p.label()"/>
			<td py:content="p.machine.name"/>
			<td>
				<a href="${tg.url( 'console',
						 host = p.machine.ip,
						 port = ports[ p ],
						 process = labels[ p ])}">Connect</a>
			</td>
		</tr>
	</table>

</div>

</html>
