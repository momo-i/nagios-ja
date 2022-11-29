<?php
include_once(dirname(__FILE__).'/includes/utils.inc.php');

$this_version = '4.4.9';
$link_target = 'main';
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">

<html>

<head>
<meta http-equiv="content-type" content="text/html;charset=UTF-8">
<meta name="ROBOTS" content="NOINDEX, NOFOLLOW">
<title>Nagios Core 日本語化</title>
<link href="stylesheets/common.css?<?php echo $this_version; ?>" type="text/css" rel="stylesheet">
</head>


<body class='navbar'>

<div class="navbarlogo">
	<a href="https://www.nagios.org" target="_blank"><img src="images/sblogo.png" height="39" width="140" border="0" alt="Nagios" /></a>
</div>

<div class="navsection">
	<div class="navsectiontitle">全般</div>
	<div class="navsectionlinks">
		<ul class="navsectionlinks">
			<li><a href="main.php" target="<?php echo $link_target;?>">ホーム</a></li>
			<li><a href="https://go.nagios.com/nagioscore/docs" target="_blank">ドキュメント</a></li>
		</ul>
	</div>
</div>

<div class="navsection">
	<div class="navsectiontitle">現在の状態</div>
	<div class="navsectionlinks">
		<ul class="navsectionlinks">
			<li><a href="<?php echo $cfg["cgi_base_url"];?>/tac.cgi" target="<?php echo $link_target;?>">総合監視状態</a></li>
			<li>
				<a href="map.php?host=all" target="<?php echo $link_target;?>">マップ</a>
				<a href="<?php echo $cfg["cgi_base_url"];?>/statusmap.cgi?host=all" target="<?php echo $link_target;?>">(旧)</a>
			</li>
			<li><a href="<?php echo $cfg["cgi_base_url"];?>/status.cgi?hostgroup=all&amp;style=hostdetail" target="<?php echo $link_target;?>">ホスト</a></li>
			<li><a href="<?php echo $cfg["cgi_base_url"];?>/status.cgi?host=all" target="<?php echo $link_target;?>">サービス</a></li>
			<li>
				<a href="<?php echo $cfg["cgi_base_url"];?>/status.cgi?hostgroup=all&amp;style=overview" target="<?php echo $link_target;?>">ホストグループ</a>
				<ul>
					<li><a href="<?php echo $cfg["cgi_base_url"];?>/status.cgi?hostgroup=all&amp;style=summary" target="<?php echo $link_target;?>">サマリー</a></li>
					<li><a href="<?php echo $cfg["cgi_base_url"];?>/status.cgi?hostgroup=all&amp;style=grid" target="<?php echo $link_target;?>">グリッド</a></li>
				</ul>
			</li>
			<li>
				<a href="<?php echo $cfg["cgi_base_url"];?>/status.cgi?servicegroup=all&amp;style=overview" target="<?php echo $link_target;?>">サービスグループ</a>
				<ul>
					<li><a href="<?php echo $cfg["cgi_base_url"];?>/status.cgi?servicegroup=all&amp;style=summary" target="<?php echo $link_target;?>">サマリー</a></li>
					<li><a href="<?php echo $cfg["cgi_base_url"];?>/status.cgi?servicegroup=all&amp;style=grid" target="<?php echo $link_target;?>">グリッド</a></li>
				</ul>
			</li>
		</ul>
	</div>
	<div class="navsectionheader">
		<ul>
			<li>障害
				<ul>
					<li><a href="<?php echo $cfg["cgi_base_url"];?>/status.cgi?host=all&amp;servicestatustypes=28" target="<?php echo $link_target;?>">サービス</a> (<a href="<?php echo $cfg["cgi_base_url"];?>/status.cgi?host=all&amp;type=detail&amp;hoststatustypes=3&amp;serviceprops=10&amp;servicestatustypes=28" target="<?php echo $link_target;?>">未処理の障害</a>)</li>
					<li><a href="<?php echo $cfg["cgi_base_url"];?>/status.cgi?hostgroup=all&amp;style=hostdetail&amp;hoststatustypes=12" target="<?php echo $link_target;?>">ホスト</a> (<a href="<?php echo $cfg["cgi_base_url"];?>/status.cgi?hostgroup=all&amp;style=hostdetail&amp;hoststatustypes=12&amp;hostprops=42" target="<?php echo $link_target;?>">未処理の障害</a>)</li>
					<li><a href="<?php echo $cfg["cgi_base_url"];?>/outages.cgi" target="<?php echo $link_target;?>">ネットワーク障害</a></li>
				</ul>
			</li>
		</ul>
	</div>
	<div class="navbarsearch">
		<form method="get" action="<?php echo $cfg["cgi_base_url"];?>/status.cgi" target="<?php echo $link_target;?>">
			<fieldset>
				<legend>クイック検索:</legend>
				<input type='hidden' name='navbarsearch' value='1'>
				<input type='text' name='host' size='15' class="NavBarSearchItem">
			</fieldset>
		</form>
	</div>
</div>

<div class="navsection">
	<div class="navsectiontitle">レポート</div>
	<div class="navsectionlinks">
		<ul class="navsectionlinks">
			<li><a href="<?php echo $cfg["cgi_base_url"];?>/avail.cgi" target="<?php echo $link_target;?>">稼動状況</a></li>
			<li>
				<a href="trends.html" target="<?php echo $link_target;?>">傾向</a>
				<a href="<?php echo $cfg["cgi_base_url"];?>/trends.cgi" target="<?php echo $link_target;?>">(旧)</a>
			</li>
			<li><a href="<?php echo $cfg["cgi_base_url"];?>/history.cgi?host=all" target="<?php echo $link_target;?>">警報</a>
			<ul>
				<li><a href="<?php echo $cfg["cgi_base_url"];?>/history.cgi?host=all" target="<?php echo $link_target;?>">履歴</a></li>
				<li><a href="<?php echo $cfg["cgi_base_url"];?>/summary.cgi" target="<?php echo $link_target;?>">サマリー</a></li>
				<li>
					<a href="histogram.html" target="<?php echo $link_target;?>">ヒストグラム</a>
					<a href="<?php echo $cfg["cgi_base_url"];?>/histogram.cgi" target="<?php echo $link_target;?>">(旧)</a>
				</li>
			</ul>
			</li>
			<li><a href="<?php echo $cfg["cgi_base_url"];?>/notifications.cgi?contact=all" target="<?php echo $link_target;?>">通知</a></li>
			<li><a href="<?php echo $cfg["cgi_base_url"];?>/showlog.cgi" target="<?php echo $link_target;?>">イベントログ</a></li>
		</ul>
	</div>
</div>

<div class="navsection">
	<div class="navsectiontitle">システム</div>
	<div class="navsectionlinks">
		<ul class="navsectionlinks">
			<li><a href="<?php echo $cfg["cgi_base_url"];?>/extinfo.cgi?type=3" target="<?php echo $link_target;?>">コメント</a></li>
			<li><a href="<?php echo $cfg["cgi_base_url"];?>/extinfo.cgi?type=6" target="<?php echo $link_target;?>">ダウンタイム</a></li>
			<li><a href="<?php echo $cfg["cgi_base_url"];?>/extinfo.cgi?type=0" target="<?php echo $link_target;?>">プロセス情報</a></li>
			<li><a href="<?php echo $cfg["cgi_base_url"];?>/extinfo.cgi?type=4" target="<?php echo $link_target;?>">パフォーマンス情報</a></li>
			<li><a href="<?php echo $cfg["cgi_base_url"];?>/extinfo.cgi?type=7" target="<?php echo $link_target;?>">スケジュール</a></li>
			<li><a href="<?php echo $cfg["cgi_base_url"];?>/config.cgi" target="<?php echo $link_target;?>">設定内容の確認</a></li>
		</ul>
	</div>
</div>

</body>
</html>
