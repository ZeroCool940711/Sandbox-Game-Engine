<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html xmlns="http://www.w3.org/1999/xhtml"
	  xmlns:py="http://purl.org/kid/ns#"
	  py:extends="'common.kid'">
<head>
	<title>${PAGE_TITLE}</title>
</head>

<frameset rows="50px,*"  framespacing="0" frameborder="no">
	<frame id="top_pane" name="top_pane" src="/static/html/help_top_pane.html"/>
	<frameset cols="200px,*" framespacing="0">
		<frame name="left_pane"  src="helpLeftPane"  noresize="noresize"/>
		<frame name="right_pane" src="static/html/help_right_pane.html" noresize="noresize"/>
	</frameset>
	<noframes>
		This browser window does not support frames.
	</noframes>
</frameset>

</html>

