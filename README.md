Nagios 4.x 日本語化
==========

![Nagios!](https://www.nagios.com/wp-content/uploads/2015/05/Nagios-Black-500x124.png)

[![Build Status](https://travis-ci.org/momo-i/nagios-ja.svg?branch=master)](https://travis-ci.org/momo-i/nagios-ja)


NagiosはC言語で書かれたホスト/サービス/ネットワークプログラムで、
GNU一般公衆利用許諾契約書、バージョン2の下でリリースされています。
CGIプログラムは現在のステータス、履歴、などをWebインターフェースを
通じてみることができるようになっています。

ドキュメント、新規リリース、バグレポート、掲示板の情報などは
Nagiosのホームページ https://www.nagios.org を訪れてください。


[機能](https://www.nagios.org/about/features/)
-----------------------------------------------
* SMTP、POP3、HTTP、PINGなどを経由したネットワークサービスの監視
* CPU負荷、ディスr九使用率などのホストリソースの監視
* ユーザが開発したサービス監視方法を許可するインターフェースプラグイン
* Ability to define network host hierarchy using "parent" hosts,
  allowing detection of and distinction between hosts that are down
  and those that are unreachable.
* Notifications when problems occur and get resolved (via email,
  pager, or user-defined method).
* Ability to define event handlers for proactive problem resolution.
* Automatic log file rotation/archiving.
* Optional web interface for viewing current network status,
  notification and problem history, log file, etc.


変更履歴
-------
See the
[Changelog](https://raw.githubusercontent.com/NagiosEnterprises/nagioscore/master/Changelog)
for a summary of important changes and fixes, or the
[commit history](https://github.com/NagiosEnterprises/nagioscore/commits/master)
for more detail.


ダウンロード
--------
最新版は https://www.nagios.org/download/ からダウンロードできます。日本語化パッチについては https://ftp.momo-i.org/pub/security/nagios/patches/ からダウンロードできます。


インストール方法
------------
[クイックスタートインストールガイド](http://nagios.sourceforge.net/docs/nagioscore/4/en/quickstart.html)
はNagiosの起動や監視に役立ちます。


Documentation & Support
-----------------------
* [User Guide](http://nagios.sourceforge.net/docs/nagioscore/4/en/)
* [Nagios Core Documentation Library](https://library.nagios.com/library/products/nagioscore/)
* [Support Forums](https://support.nagios.com/forum/viewforum.php?f=7)
* [Additional Support Resources](https://www.nagios.org/support/)


Contributing
------------
The Nagios source code is hosted on GitHub:
https://github.com/NagiosEnterprises/nagioscore

Do you have an idea or feature request to make Nagios better? Join or start a
discussion on the [Nagios Core Development forum](https://support.nagios.com/forum/viewforum.php?f=34).
Bugs can be reported by [opening an issue on GitHub](https://github.com/NagiosEnterprises/nagioscore/issues/new).
If you have identified a security related issue in Nagios, please contact
security@nagios.com.

Patches and GitHub pull requests are welcome. Pull requests on GitHub
link commits in version control to review and discussion of the
changes, helping to show how and why changes were made, in addition to
who was involved.

Created by Ethan Galstad, the success of Nagios has been due to the
fantastic community members that support it and provide bug reports,
patches, and great ideas. See the
[THANKS file](https://raw.githubusercontent.com/NagiosEnterprises/nagioscore/master/THANKS)
for some of the many who have contributed since 1999.
