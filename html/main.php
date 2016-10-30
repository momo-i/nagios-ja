<?php
include_once(dirname(__FILE__).'/includes/utils.inc.php');

$this_version = '4.2.2';
$this_year = '2016';
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">

<html>

<head>
<meta http-equiv="content-type" content="text/html;charset=UTF-8">
<meta name="ROBOTS" content="NOINDEX, NOFOLLOW" />
<title>Nagios Core 日本語化</title>
<link rel="stylesheet" type="text/css" href="stylesheets/common.css?<?php echo $this_version; ?>" />
<script type="text/javascript" src="js/jquery-1.7.1.min.js"></script>

<script type='text/javascript'>
	$(document).ready(function() {
		loadRemoteFeed( // Our top banner splash.
			'#splashbox0-contents', 'corebanner', 1,
			'', processBannerItem, ''
		);

		loadRemoteFeed( // "Latest News"
			'#splashbox4-contents', 'frontpage', 3,
			'<ul>', processNewsItem, '<li><a href="https://www.nagios.org/news" target="_blank">新着情報...</a></li></ul>'
		);

		loadRemoteFeed( // "Don't Miss..."
			'#splashbox5-contents', 'corepromo', 3,
			'<ul>', processPromoItem, '</ul>'
		);

		getCoreStatus();
	});

	// Fetch an RSS feed and format HTML for the first n items.
	function loadRemoteFeed(id, name, n, prefix, formatter, suffix) {
		$.ajax({
			type: 'GET',
			url: 'https://www.nagios.org/backend/feeds/' + name + '/',
			crossDomain: true,
			success: function(d, status, jqXHR) {
				// We should have Internet access, set the playlist HTML.
				initializePlaylist();

				var text = ''; // Start with empty text by default.

				$(d).find('channel').find('item').each(function(index) {
					var itemText = formatter($(this)); // Format the item's HTML.
					if (itemText) text += itemText; // Append if non-empty.
					return index+1 < n; // Only process n items.
				});

				// Only set the HTML if we have item text.
				if (text) $(id).html(prefix + text + suffix);
			}
		});
	}

	function processBannerItem(item) {
		return item.find('description').text();
	}

	function processNewsItem(item) {
		var link = item.find('link').text();
		var title = item.find('title').text();
		return link && title
			? '<li><a href="' + link + '" target="_blank">' + title + '</a></li>'
			: '';
	}

	function processPromoItem(item) {
		var description = item.find('description').text();
		return description
			? '<li>' + description + '</li>'
			: '';
	}


	// Set our playlist HTML when we know we have Internet access.
	var playlistInitialized = false;
	function initializePlaylist() {
		if (!playlistInitialized) {
			playlistInitialized = true;
			$('#splashbox3')
				.addClass('splashbox3-full')
				.removeClass('splashbox3-empty')
				.html('<iframe width="100%" height="100%" src="//www.youtube.com/embed/videoseries?list=PLN-ryIrpC_mCUW1DFwZpxpAk00i60lSkE&iv_load_policy=3&rel=0" frameborder="0" allowfullscreen></iframe>');
		}
	}

	// Get the daemon status JSON.
	function getCoreStatus() {
		setCoreStatusHTML('passiveonly', 'プロセスのステータスをチェック中...');

		$.get('<?php echo $cfg["cgi_base_url"];?>/statusjson.cgi?query=programstatus', function(d) {
			d = d && d.data && d.data.programstatus || false;
			if (d && d.nagios_pid) {
				var pid = d.nagios_pid;
				var daemon = d.daemon_mode ? 'デーモン' : 'プロセス';
				setCoreStatusHTML('enabled', daemon + 'はプロセスID' + pid + 'で動作中');
			} else {
				setCoreStatusHTML('disabled', '動作していません');
			}
		}).fail(function() {
			setCoreStatusHTML('disabled', 'プロセスのステータスを取得できませんでした');
		});
	}

	function setCoreStatusHTML(image, text) {
		$('#core-status').html('<img src="images/' + image + '.gif" /> ' + text);
	}
</script>

</head>


<body id="splashpage">


<div id="mainbrandsplash">
	<div id="mainlogo"><a href="https://www.nagios.org/" target="_blank"><img src="images/logofullsize.png" border="0" alt="Nagios" title="Nagios"></a></div>
	<div><span id="core-status"></span></div>
</div>


<div id="currentversioninfo">
	<div class="product">Nagios<sup><span style="font-size: small;">&reg;</span></sup> Core<sup><span style="font-size: small;">&trade;</span></sup> 日本語化</div>
	<div class="version">バージョン <?php echo $this_version; ?></div>
	<div class="releasedate">2016年10月24日</div>
	<div class="checkforupdates"><a href="https://www.nagios.org/checkforupdates/?version=<?php echo $this_version; ?>&amp;product=nagioscore" target="_blank">更新のチェック</a></div>
</div>


<div id="updateversioninfo">
<?php
	$updateinfo = get_update_information();
	if (!$updateinfo['update_checks_enabled']) {
?>
		<div class="updatechecksdisabled">
			<div class="warningmessage">警告: 自動更新チェックは無効です。</div>
			<div class="submessage">アップデートチェックを無効にすると潜在的なセキュリティリスクがあります。Nagiosの設定ファイルで更新チェックを有効にするか、手動で更新を確認するために<a href="https://www.nagios.org/" target="_blank">nagios.org</a>を訪れてください。</a></div>
		</div>
<?php
	} else if (
		$updateinfo['update_available'] && $this_version < $updateinfo['update_version']
	) {
?>
		<div class="updateavailable">
			<div class="updatemessage">Nagios Coreの最新バージョンが利用可能です</div>
			<div class="submessage">Nagios <?php echo $updateinfo['update_version'];?>をダウンロードするために<a href="https://www.nagios.org/download/" target="_blank">nagios.org</a>を訪れてください。</div>
		</div>
<?php
	}
?>
</div>


<div id='splashrow0'>
	<div id="splashbox0" class="splashbox-media"><!-- info banner feed -->
		<div id="splashbox0-contents"></div>
	</div>
</div>


<div id="splashboxes">

	<div id='splashrow1'>

		<div id="splashbox1" class="splashbox splashbox-clear">
			<h2>始めに</h2>
			<ul>
				<li><a href="https://go.nagios.com/nagioscore/startmonitoring" target="_blank">インフラストラクチャの監視を開始</a></li>
				<li><a href="https://go.nagios.com/nagioscore/changelook" target="_blank">Nagiosの外観を変更</a></li>
				<li><a href="https://go.nagios.com/nagioscore/extend" target="_blank">数百ものアドオンでNagiosを拡張</a></li>
				<!--<li><a href="https://go.nagios.com/nagioscore/docs" target="_blank">Read the Nagios documentation</a></li>-->
				<li><a href="https://go.nagios.com/nagioscore/support" target="_blank">サポート</a></li>
				<li><a href="https://go.nagios.com/nagioscore/training" target="_blank">トレーニング</a></li>
				<li><a href="https://go.nagios.com/nagioscore/certification" target="_blank">認定</a></li>
			</ul>
		</div>

		<div id="splashbox2" class="splashbox">
			<h2>クイックリンク</h2>
			<ul>
				<li><a href="https://library.nagios.com" target="_blank">Nagios Library</a> (チュートリアルとドキュメント)</li>
				<li><a href="https://labs.nagios.com" target="_blank">Nagios Labs</a> (開発ブログ)</li>
				<li><a href="https://exchange.nagios.org" target="_blank">Nagios Exchange</a> (プラグインとアドオン)</li>
				<li><a href="https://support.nagios.com" target="_blank">Nagios Support</a> (テクニカルサポート)</li>
				<li><a href="https://www.nagios.com" target="_blank">Nagios.com</a> (会社)</li>
				<li><a href="https://www.nagios.org" target="_blank">Nagios.org</a> (プロジェクト)</li>
			</ul>
		</div>

	</div><!-- end splashrow1 -->
	
	<div id="splashrow2">

		<div id="splashbox4" class="splashbox splashbox-clear"><!-- latest news feed -->
			<h2>新着ニュース</h2>
			<div id="splashbox4-contents">
			</div>
		</div>

		<div id="splashbox5" class="splashbox"><!-- core promo feed -->
			<h2>お見逃しなく</h2>
			<div id="splashbox5-contents">
			</div>
		</div>

		<div id="splashbox3" class="splashbox3-empty"><!-- youtube playlist -->
		</div>

	</div><!-- end splashrow2 -->

</div><!-- end splashboxes-->


<div id="mainfooter">
	<div id="maincopy">
		Copyright &copy; 2010-<?php echo $this_year; ?> Nagios Core Development Team and Community Contributors. Copyright &copy; 1999-2009 Ethan Galstad. See the THANKS file for more information on contributors.
	</div>
	<div CLASS="disclaimer">
		Nagios Core is licensed under the GNU General Public License and is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.  Nagios, Nagios Core and the Nagios logo are trademarks, servicemarks, registered trademarks or registered servicemarks owned by Nagios Enterprises, LLC.  Use of the Nagios marks is governed by the <A href="https://www.nagios.com/legal/trademarks/">trademark use restrictions</a>.
	</div>
	<div class="logos">
		<a href="https://www.nagios.org/" target="_blank"><img src="images/weblogo1.png" width="102" height="47" border="0" style="padding: 0 40px 0 40px;" title="Nagios.org" /></a>
		<a href="http://sourceforge.net/projects/nagios" target="_blank"><img src="images/sflogo.png" width="88" height="31" border="0" alt="SourceForge.net Logo" /></a>
	</div>
</div> 


</body>
</html>
