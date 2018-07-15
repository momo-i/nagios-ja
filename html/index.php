<?php
// Allow specifying main window URL for permalinks, etc.
$url = 'main.php';

if (isset($_GET['corewindow'])) {

	// The default window url may have been overridden with a permalink...
	// Parse the URL and remove permalink option from base.
	$a = parse_url($_GET['corewindow']);

	// Build the base url.
	$url = htmlentities($a['path']).'?';
	$url = (isset($a['host'])) ? $a['scheme'].'://'.$a['host'].$url : '/'.$url;

	$query = isset($a['query']) ? $a['query'] : '';
	$pairs = explode('&', $query);
	foreach ($pairs as $pair) {
		$v = explode('=', $pair);
		if (is_array($v)) {
			$key = urlencode($v[0]);
			$val = urlencode(isset($v[1]) ? $v[1] : '');
			$url .= "&$key=$val";
		}
	}
	if (preg_match("/^http:\/\/|^https:\/\/|^\//", $url) != 1)
		$url = "main.php";
}

$this_year = '2016';
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Frameset//EN" "http://www.w3.org/TR/html4/frameset.dtd">

<html>
<head>
	<meta http-equiv="content-type" content="text/html;charset=UTF-8">
	<meta name="ROBOTS" content="NOINDEX, NOFOLLOW">
<script LANGUAGE="javascript">
	var n = Math.round(Math.random() * 10000000000);
	document.write("<title>Nagios Core 日本語化 on " + window.location.hostname + "</title>");
	document.cookie = "NagFormId=" + n.toString(16);
</script>
	<link rel="shortcut icon" href="images/favicon.ico" type="image/ico">
</head>

<frameset cols="205,*" style="border: 0px;">
	<frame src="side.php" name="side" frameborder="0" style="">
	<frame src="<?php echo $url; ?>" name="main" frameborder="0" style="">

	<noframes>
		<!-- このページを表示するには、フレームをサポートしているブラウザが必要です。 -->
		<h2>Nagios Core 日本語化</h2>
		<p align="center">
			<a href="https://www.nagios.org/">www.nagios.org</a><br>
			Copyright &copy; 2010-<?php echo $this_year; ?> Nagios Core Development Team and Community Contributors.
			Copyright &copy; 1999-2010 Ethan Galstad<br>
		</p>
		<p>
			<i>注: このページを表示するには、フレームをサポートしているブラウザが必要です。</i>
		</p>
	</noframes>
</frameset>

</html>