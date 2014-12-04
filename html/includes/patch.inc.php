<?php
/**
 *
 * Patch checker Class
 *
 * Patch Class
 *
 * PHP version 5.5
 *
 * LICENSE: The MIT License
 * Copyright (c) momo-i www.momo-i.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @category	patch
 * @package		Patch
 * @author		momo-i <webmaster@momo-i.org>
 * @copyright	2014 www.momo-i.org
 * @license		http://opensource.org/licenses/mit-license.php MIT License
 * @version		1.0
 * @link		http://www.momo-i.org/
 * @since		Class available since Release 1.0
 */

/**
 * Patch Class
 *
 * This is Patch class for patch checker.
 *
 * @category    patch
 * @package		Patch
 * @author		momo-i <webmaster@momo-i.org>
 * @copyright	2014 www.momo-i.org
 * @license		http://opensource.org/licenses/mit-license.php MIT License
 * @version		1.0
 */
class patch {

	// {{{ property

	/**
	 * Base URL
	 *
	 * @var		string
	 * @access	private
	 */
	private $_baseurl = 'http://%s/pub/security/nagios/patches';

	/**
	 * Base Host
	 *
	 * @var		string
	 * @access	private
	 */
	private $_host = 'ftp.momo-i.org';

	/**
	 * UserAgent
	 *
	 * @var		string
	 * @access	private
	 */
	private $_UA = "Mozilla/5.0 Nagios Japanese Patch Checker/1.0";

	/**
	 * tmp file
	 *
	 * @var		string
	 * @access	private
	 */
	private $_tmpfile = "/tmp/nagios-patch.check";

	// }}}

	// {{{ constructor
	/**
	 * Constructor
	 *
	 * Nothing to do...
	 *
	 * @access	private
	 */
	private function __construct() {}
	// }}}

	/**
	 * Main function
	 *
	 * @param	string	Version
	 * @access	public
	 * @return	string	Check Result
	 */
	static public function get_patch($version, $period = 3600)
	{
		$self = new self();
		if($self->_check_period($period))
			return $self->_get_newversion($version);
		else
			return $self->_get_newversion($version, false);
	}

	/**
	 * Check Nagios Japanese patch
	 *
	 * @param	string	Version
	 * @access	private
	 * @return	string	Check Result
	 */
	private function _get_newversion($version, $check = true)
	{
		$msg = "利用可能な最新の日本語化パッチは存在しません。";
		if($check == false)
		{
			return $msg;
		}
		$baseurl = sprintf($this->_baseurl, $this->_host);
		$url = sprintf('%s/nagios-jp-%s.patch.gz', $baseurl, $version);
		if(extension_loaded('curl'))
		{
			$ch = curl_init($url);
			curl_setopt($ch, CURLOPT_FAILONERROR, true);
			curl_setopt($ch, CURLOPT_USERAGENT, $this->_UA);
			curl_setopt($ch, CURLOPT_NOBODY, true);
			curl_exec($ch);
			$info = curl_getinfo($ch);
			if($info['http_code'] != 200)
			{
				return $msg;
			}
			$msg = sprintf('日本語化パッチの<a href="%s">ダウンロード</a>が可能です。', $url);
			return $msg;
		}
		elseif(function_exists('file_get_contents') && ini_get('allow_url_fopen') == true)
		{
			$header = "User-Agent: $UA\r\n";
			$opts = array(
				'http' => array(
					'method' => 'HEAD',
					'header' => $header
				)
			);
			$cn = stream_context_create($opts);
			if(@file_get_contents($url, false, $cn) !== false)
			{
				$msg = sprintf('日本語化パッチの<a href="%s">ダウンロード</a>が可能です。', $url);
				return $msg;
			}
			return $msg;
		}
		return "日本語化パッチ確認のための機能が利用できません。<a href=\"http://ftp.momo-i.org/pub/security/nagios/patches/\">こちら</a>より確認して下さい。";
	}

	/**
	 * Check Period
	 */
	private function _check_period($period)
	{
		$now = strtotime('now');
		if(is_file($this->_tmpfile))
		{
			$check_time = trim(file_get_contents($this->_tmpfile));
			if(($now - $check_time) > $period)
			{
				return file_put_contents($this->_tmpfile, $now);
			}
			return false;
		}
		else
		{
			return file_put_contents($this->_tmpfile, $now);
		}
	}
}
