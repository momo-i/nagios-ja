/***********************************************************************
 *
 * CONFIG.C - Nagios Configuration CGI (View Only)
 *
 *
 * This CGI program will display various configuration information.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ***********************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"
#include "../include/macros.h"
#include "../include/cgiutils.h"
#include "../include/cgiauth.h"
#include "../include/getcgi.h"

static nagios_macros *mac;

extern char   main_config_file[MAX_FILENAME_LENGTH];
extern char   url_html_path[MAX_FILENAME_LENGTH];
extern char   url_docs_path[MAX_FILENAME_LENGTH];
extern char   url_images_path[MAX_FILENAME_LENGTH];
extern char   url_logo_images_path[MAX_FILENAME_LENGTH];
extern char   url_stylesheets_path[MAX_FILENAME_LENGTH];

#define DISPLAY_NONE                     0
#define DISPLAY_HOSTS                    1
#define DISPLAY_HOSTGROUPS               2
#define DISPLAY_CONTACTS                 3
#define DISPLAY_CONTACTGROUPS            4
#define DISPLAY_SERVICES                 5
#define DISPLAY_TIMEPERIODS              6
#define DISPLAY_COMMANDS                 7
#define DISPLAY_HOSTGROUPESCALATIONS     8    /* no longer implemented */
#define DISPLAY_SERVICEDEPENDENCIES      9
#define DISPLAY_SERVICEESCALATIONS       10
#define DISPLAY_HOSTDEPENDENCIES         11
#define DISPLAY_HOSTESCALATIONS          12
#define DISPLAY_SERVICEGROUPS            15
#define DISPLAY_COMMAND_EXPANSION        16211

void document_header(int);
void document_footer(void);
int process_cgivars(void);

void display_options(void);

void display_hosts(void);
void display_hostgroups(void);
void display_servicegroups(void);
void display_contacts(void);
void display_contactgroups(void);
void display_services(void);
void display_timeperiods(void);
void display_commands(void);
void display_servicedependencies(void);
void display_serviceescalations(void);
void display_hostdependencies(void);
void display_hostescalations(void);
void display_command_expansion(void);

void unauthorized_message(void);


authdata current_authdata;

int display_type = DISPLAY_NONE;
char to_expand[MAX_COMMAND_BUFFER];
char hashed_color[8];

int embedded = FALSE;

static void print_expand_input(int type) {
	const char *seldesc = "";

	if(type == DISPLAY_COMMAND_EXPANSION) return;	/* Has its own form, w/ larger <input> */
	else if(type == DISPLAY_SERVICES) {
		seldesc = "ホスト上または指定されたサービスのみ";
		}
	else if(type == DISPLAY_SERVICEDEPENDENCIES) {
		seldesc = "ホストとの依存関係のみ";
		}
	else if(type == DISPLAY_SERVICEESCALATIONS) {
		seldesc = "ホスト上のエスカレーションのみ";
		}
	else if(type == DISPLAY_HOSTDEPENDENCIES) {
		seldesc = "ホスト上の依存関係のみ";
		}
	else if(type == DISPLAY_HOSTESCALATIONS) {
		seldesc = "ホスト用のエスカレーションのみ";
		}
	printf("<tr><td align=left class='reportSelectSubTitle'>%s表示:</td></tr>\n", seldesc);
	printf("<tr><td align=left class='reportSelectItem'><input type='text' name='expand'\n");
	printf("value='%s'>", html_encode(to_expand, FALSE));
	}

int main(void) {
	mac = get_global_macros();

	/* get the arguments passed in the URL */
	process_cgivars();

	/* reset internal variables */
	reset_cgi_vars();

	cgi_init(document_header, document_footer, READ_ALL_OBJECT_DATA, 0);

	/* initialize macros */
	init_macros();

	document_header(TRUE);

	/* get authentication information */
	get_authentication_information(&current_authdata);

	/* begin top table */
	printf("<table border=0 width=100%%>\n");
	printf("<tr>\n");

	/* left column of the first row */
	printf("<td align=left valign=top width=50%%>\n");
	display_info_table("各種設定 -Configuration-", FALSE, &current_authdata);
	printf("</td>\n");

	/* right hand column of top row */
	printf("<td align=right valign=bottom width=50%%>\n");

	if(display_type != DISPLAY_NONE) {

		printf("<form method=\"get\" action=\"%s\">\n", CONFIG_CGI);
		printf("<table border=0>\n");

		printf("<tr><td align=left class='reportSelectSubTitle'>オブジェクトタイプ:</td></tr>\n");
		printf("<tr><td align=left class='reportSelectItem'>");
		printf("<select name='type'>\n");
		printf("<option value='hosts' %s>ホスト\n", (display_type == DISPLAY_HOSTS) ? "SELECTED" : "");
		printf("<option value='hostdependencies' %s>ホスト依存関係\n", (display_type == DISPLAY_HOSTDEPENDENCIES) ? "SELECTED" : "");
		printf("<option value='hostescalations' %s>ホストエスカレーション(上位伝達)\n", (display_type == DISPLAY_HOSTESCALATIONS) ? "SELECTED" : "");
		printf("<option value='hostgroups' %s>ホストグループ\n", (display_type == DISPLAY_HOSTGROUPS) ? "SELECTED" : "");
		printf("<option value='services' %s>サービス\n", (display_type == DISPLAY_SERVICES) ? "SELECTED" : "");
		printf("<option value='servicegroups' %s>サービスグループ\n", (display_type == DISPLAY_SERVICEGROUPS) ? "SELECTED" : "");
		printf("<option value='servicedependencies' %s>サービス依存関係\n", (display_type == DISPLAY_SERVICEDEPENDENCIES) ? "SELECTED" : "");
		printf("<option value='serviceescalations' %s>サービスエスカレーション(上位伝達)\n", (display_type == DISPLAY_SERVICEESCALATIONS) ? "SELECTED" : "");
		printf("<option value='contacts' %s>通知先\n", (display_type == DISPLAY_CONTACTS) ? "SELECTED" : "");
		printf("<option value='contactgroups' %s>通知先グループ\n", (display_type == DISPLAY_CONTACTGROUPS) ? "SELECTED" : "");
		printf("<option value='timeperiods' %s>時間帯\n", (display_type == DISPLAY_TIMEPERIODS) ? "SELECTED" : "");
		printf("<option value='commands' %s>コマンド\n", (display_type == DISPLAY_COMMANDS) ? "SELECTED" : "");
		printf("<option value='command' %s>拡張コマンド\n", (display_type == DISPLAY_COMMAND_EXPANSION) ? "SELECTED" : "");
		printf("</select>\n");
		printf("</td></tr>\n");

		print_expand_input(display_type);

		printf("<tr><td class='reportSelectItem'><input type='submit' value='更新'></td></tr>\n");
		printf("</table>\n");
		printf("</form>\n");
		}

	/* display context-sensitive help */
	switch(display_type) {
		case DISPLAY_HOSTS:
			display_context_help(CONTEXTHELP_CONFIG_HOSTS);
			break;
		case DISPLAY_HOSTGROUPS:
			display_context_help(CONTEXTHELP_CONFIG_HOSTGROUPS);
			break;
		case DISPLAY_SERVICEGROUPS:
			display_context_help(CONTEXTHELP_CONFIG_SERVICEGROUPS);
			break;
		case DISPLAY_CONTACTS:
			display_context_help(CONTEXTHELP_CONFIG_CONTACTS);
			break;
		case DISPLAY_CONTACTGROUPS:
			display_context_help(CONTEXTHELP_CONFIG_CONTACTGROUPS);
			break;
		case DISPLAY_SERVICES:
			display_context_help(CONTEXTHELP_CONFIG_SERVICES);
			break;
		case DISPLAY_TIMEPERIODS:
			display_context_help(CONTEXTHELP_CONFIG_TIMEPERIODS);
			break;
		case DISPLAY_COMMANDS:
			display_context_help(CONTEXTHELP_CONFIG_COMMANDS);
			break;
		case DISPLAY_SERVICEDEPENDENCIES:
			display_context_help(CONTEXTHELP_CONFIG_SERVICEDEPENDENCIES);
			break;
		case DISPLAY_SERVICEESCALATIONS:
			display_context_help(CONTEXTHELP_CONFIG_HOSTESCALATIONS);
			break;
		case DISPLAY_HOSTDEPENDENCIES:
			display_context_help(CONTEXTHELP_CONFIG_HOSTDEPENDENCIES);
			break;
		case DISPLAY_HOSTESCALATIONS:
			display_context_help(CONTEXTHELP_CONFIG_HOSTESCALATIONS);
			break;
		case DISPLAY_COMMAND_EXPANSION:
			/* Reusing DISPLAY_COMMANDS help until further notice */
			display_context_help(CONTEXTHELP_CONFIG_COMMANDS);
			break;
		default:
			display_context_help(CONTEXTHELP_CONFIG_MENU);
			break;
		}

	printf("</td>\n");

	/* end of top table */
	printf("</tr>\n");
	printf("</table>\n");


	switch(display_type) {
		case DISPLAY_HOSTS:
			display_hosts();
			break;
		case DISPLAY_HOSTGROUPS:
			display_hostgroups();
			break;
		case DISPLAY_SERVICEGROUPS:
			display_servicegroups();
			break;
		case DISPLAY_CONTACTS:
			display_contacts();
			break;
		case DISPLAY_CONTACTGROUPS:
			display_contactgroups();
			break;
		case DISPLAY_SERVICES:
			display_services();
			break;
		case DISPLAY_TIMEPERIODS:
			display_timeperiods();
			break;
		case DISPLAY_COMMANDS:
			display_commands();
			break;
		case DISPLAY_SERVICEDEPENDENCIES:
			display_servicedependencies();
			break;
		case DISPLAY_SERVICEESCALATIONS:
			display_serviceescalations();
			break;
		case DISPLAY_HOSTDEPENDENCIES:
			display_hostdependencies();
			break;
		case DISPLAY_HOSTESCALATIONS:
			display_hostescalations();
			break;
		case DISPLAY_COMMAND_EXPANSION:
			display_command_expansion();
			break;
		default:
			display_options();
			break;
		}

	document_footer();

	return OK;
	}




void document_header(int use_stylesheet) {
	char date_time[MAX_DATETIME_LENGTH];
	time_t t;

	if(embedded == TRUE)
		return;

	time(&t);
	get_time_string(&t, date_time, sizeof(date_time), HTTP_DATE_TIME);

	printf("Cache-Control: no-store\r\n");
	printf("Pragma: no-cache\r\n");
	printf("Last-Modified: %s\r\n", date_time);
	printf("Expires: %s\r\n", date_time);
	printf("Content-type: text/html; charset=utf-8\r\n\r\n");

	printf("<html>\n");
	printf("<head>\n");
	printf("<meta http-equiv='content-type' content='text/html;charset=UTF-8'>\n");
	printf("<link rel=\"shortcut icon\" href=\"%sfavicon.ico\" type=\"image/ico\">\n", url_images_path);
	printf("<META HTTP-EQUIV='Pragma' CONTENT='no-cache'>\n");
	printf("<title>\n");
	printf("設定 -Configuration-\n");
	printf("</title>\n");

	if(use_stylesheet == TRUE) {
		printf("<LINK REL='stylesheet' TYPE='text/css' HREF='%s%s'>\n", url_stylesheets_path, COMMON_CSS);
		printf("<LINK REL='stylesheet' TYPE='text/css' HREF='%s%s'>\n", url_stylesheets_path, CONFIG_CSS);
		}

	printf("</head>\n");

	printf("<body CLASS='config'>\n");

	/* include user SSI header */
	include_ssi_files(CONFIG_CGI, SSI_HEADER);

	return;
	}


void document_footer(void) {

	if(embedded == TRUE)
		return;

	/* include user SSI footer */
	include_ssi_files(CONFIG_CGI, SSI_FOOTER);

	printf("</body>\n");
	printf("</html>\n");

	return;
	}


int process_cgivars(void) {
	char **variables;
	int error = FALSE;
	int x;

	variables = getcgivars();
	to_expand[0] = '\0';

	for(x = 0; variables[x]; x++) {

		/* do some basic length checking on the variable identifier to prevent buffer overflows */
		if(strlen(variables[x]) >= MAX_INPUT_BUFFER - 1) {
			continue;
			}

		/* we found the configuration type argument */
		else if(!strcmp(variables[x], "type")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			/* what information should we display? */
			if(!strcmp(variables[x], "hosts"))
				display_type = DISPLAY_HOSTS;
			else if(!strcmp(variables[x], "hostgroups"))
				display_type = DISPLAY_HOSTGROUPS;
			else if(!strcmp(variables[x], "servicegroups"))
				display_type = DISPLAY_SERVICEGROUPS;
			else if(!strcmp(variables[x], "contacts"))
				display_type = DISPLAY_CONTACTS;
			else if(!strcmp(variables[x], "contactgroups"))
				display_type = DISPLAY_CONTACTGROUPS;
			else if(!strcmp(variables[x], "services"))
				display_type = DISPLAY_SERVICES;
			else if(!strcmp(variables[x], "timeperiods"))
				display_type = DISPLAY_TIMEPERIODS;
			else if(!strcmp(variables[x], "commands"))
				display_type = DISPLAY_COMMANDS;
			else if(!strcmp(variables[x], "servicedependencies"))
				display_type = DISPLAY_SERVICEDEPENDENCIES;
			else if(!strcmp(variables[x], "serviceescalations"))
				display_type = DISPLAY_SERVICEESCALATIONS;
			else if(!strcmp(variables[x], "hostdependencies"))
				display_type = DISPLAY_HOSTDEPENDENCIES;
			else if(!strcmp(variables[x], "hostescalations"))
				display_type = DISPLAY_HOSTESCALATIONS;
			else if(!strcmp(variables[x], "command"))
				display_type = DISPLAY_COMMAND_EXPANSION;

			/* we found the embed option */
			else if(!strcmp(variables[x], "embedded"))
				embedded = TRUE;
			}

		/* we found the string-to-expand argument */
		else if(!strcmp(variables[x], "expand")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}
			strncpy(to_expand, variables[x], MAX_COMMAND_BUFFER);
			to_expand[MAX_COMMAND_BUFFER - 1] = '\0';
			}


		/* we received an invalid argument */
		else
			error = TRUE;

		}

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
	}



void display_hosts(void) {
	host *temp_host = NULL;
	hostsmember *temp_hostsmember = NULL;
	contactsmember *temp_contactsmember = NULL;
	contactgroupsmember *temp_contactgroupsmember = NULL;
	char *processed_string = NULL;
	int options = 0;
	int odd = 0;
	char time_string[24];
	const char *bg_class = "";
	int num_contacts = 0;

	/* see if user is authorized to view host information... */
	if(is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		unauthorized_message();
		return;
		}

	printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>ホスト %s</DIV></P>\n",
	       (*to_expand == '\0' ? "" : html_encode(to_expand, FALSE)));

	printf("<P><DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 CLASS='data'>\n");

	printf("<TR>\n");
	printf("<TH CLASS='data'>ホスト名</TH>");
	printf("<TH CLASS='data'>エイリアス/説明</TH>");
	printf("<TH CLASS='data'>アドレス</TH>");
	printf("<TH CLASS='data'>重要度 (ホスト)</TH>");
	printf("<TH CLASS='data'>重要度 (ホスト + サービス)</TH>");
	printf("<TH CLASS='data'>上位ホスト</TH>");
	printf("<TH CLASS='data'>最大チェック数</TH>");
	printf("<TH CLASS='data'>チェック間隔</TH>\n");
	printf("<TH CLASS='data'>再試行間隔</TH>\n");
	printf("<TH CLASS='data'>ホストチェックコマンド</TH>");
	printf("<TH CLASS='data'>チェック期間</TH>");
	printf("<TH CLASS='data'>Obsess Over</TH>\n");
	printf("<TH CLASS='data'>アクティブチェック</TH>\n");
	printf("<TH CLASS='data'>パッシブチェック</TH>\n");
	printf("<TH CLASS='data'>鮮度チェック(Freshness)</TH>\n");
	printf("<TH CLASS='data'>鮮度しきい値(Freshness)</TH>\n");
	printf("<TH CLASS='data'>デフォルトの通知先/グループ</TH>\n");
	printf("<TH CLASS='data'>通知間隔</TH>");
	printf("<TH CLASS='data'>初回通知遅延時間</TH>");
	printf("<TH CLASS='data'>通知オプション</TH>");
	printf("<TH CLASS='data'>通知時間帯</TH>");
	printf("<TH CLASS='data'>イベントハンドラ</TH>");
	printf("<TH CLASS='data'>イベントハンドラの有効化</TH>");
	printf("<TH CLASS='data'>追跡オプション</TH>\n");
	printf("<TH CLASS='data'>フラップ検知設定</TH>");
	printf("<TH CLASS='data'>低フラップ閾値</TH>");
	printf("<TH CLASS='data'>高フラップ閾値</TH>");
	printf("<TH CLASS='data'>フラップ検知オプション</TH>\n");
	printf("<TH CLASS='data'>パフォーマンスデータ処理</TH>");
	printf("<TH CLASS='data'>メモ</TH>");
	printf("<TH CLASS='data'>メモURL</TH>");
	printf("<TH CLASS='data'>アクションURL</TH>");
	printf("<TH CLASS='data'>2-D 位置</TH>");
	printf("<TH CLASS='data'>3-D 位置</TH>");
	printf("<TH CLASS='data'>ステータスマップ画像</TH>");
	printf("<TH CLASS='data'>VRML画像</TH>");
	printf("<TH CLASS='data'>ロゴ画像</TH>");
	printf("<TH CLASS='data'>画像代替文字</TH>");
	printf("<TH CLASS='data'>ステータス保持オプション</TH>");
	printf("</TR>\n");

	/* check all the hosts... */
	for(temp_host = host_list; temp_host != NULL; temp_host = temp_host->next) if(((*to_expand) == '\0') || !strcmp(to_expand, temp_host->name)) {

			/* grab macros */
			grab_host_macros_r(mac, temp_host);

			if(odd) {
				odd = 0;
				bg_class = "dataOdd";
				}
			else {
				odd = 1;
				bg_class = "dataEven";
				}

			printf("<TR CLASS='%s'>\n", bg_class);

			printf("<TD CLASS='%s'><a name='%s'><a href='%s?type=services&expand=%s'>%s</a></a></TD>\n", bg_class,
			       url_encode(temp_host->name), CONFIG_CGI, url_encode(temp_host->name), html_encode(temp_host->name, FALSE));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_host->alias, FALSE));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_host->address, FALSE));
			printf("<TD CLASS='%s'>%u</TD>\n", bg_class, temp_host->hourly_value);
			printf("<TD CLASS='%s'>%u</TD>\n", bg_class, temp_host->hourly_value + host_services_value(temp_host));

			printf("<TD CLASS='%s'>", bg_class);
			for(temp_hostsmember = temp_host->parent_hosts; temp_hostsmember != NULL; temp_hostsmember = temp_hostsmember->next) {

				if(temp_hostsmember != temp_host->parent_hosts)
					printf(", ");

				printf("<a href='%s?type=hosts&expand=%s'>%s</a>\n", CONFIG_CGI, url_encode(temp_hostsmember->host_name), html_encode(temp_hostsmember->host_name, FALSE));
				}
			if(temp_host->parent_hosts == NULL)
				printf("&nbsp;");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>%d</TD>\n", bg_class, temp_host->max_attempts);

			get_interval_time_string(temp_host->check_interval, time_string, sizeof(time_string));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, time_string);

			get_interval_time_string(temp_host->retry_interval, time_string, sizeof(time_string));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, time_string);

			printf("<TD CLASS='%s'>", bg_class);
			if(temp_host->check_command == NULL)
				printf("&nbsp;");
			else
				printf("<a href='%s?type=command&expand=%s'>%s</a></TD>\n", CONFIG_CGI, url_encode(temp_host->check_command), html_encode(temp_host->check_command, FALSE));
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			if(temp_host->check_period == NULL)
				printf("&nbsp;");
			else
				printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_host->check_period), html_encode(temp_host->check_period, FALSE));
			printf("</TD>\n");

			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->obsess == TRUE) ? "有効" : "無効");

			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->checks_enabled == TRUE) ? "有効" : "無効");

			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->accept_passive_checks == TRUE) ? "有効" : "無効");

			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->check_freshness == TRUE) ? "有効" : "無効");

			printf("<TD CLASS='%s'>", bg_class);
			if(temp_host->freshness_threshold == 0)
				printf("自動設定\n");
			else
				printf("%d 秒\n", temp_host->freshness_threshold);
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);

			/* find all the contacts for this host... */
			num_contacts = 0;
			for(temp_contactsmember = temp_host->contacts; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {
				num_contacts++;
				if(num_contacts > 1)
					printf(", ");

				printf("<A HREF='%s?type=contacts&expand=%s'>%s</A>\n", CONFIG_CGI, url_encode(temp_contactsmember->contact_name), html_encode(temp_contactsmember->contact_name, FALSE));
				}
			for(temp_contactgroupsmember = temp_host->contact_groups; temp_contactgroupsmember != NULL; temp_contactgroupsmember = temp_contactgroupsmember->next) {
				num_contacts++;
				if(num_contacts > 1)
					printf(", ");
				printf("<A HREF='%s?type=contactgroups&expand=%s'>%s</A>\n", CONFIG_CGI, url_encode(temp_contactgroupsmember->group_name), html_encode(temp_contactgroupsmember->group_name, FALSE));
				}
			if(num_contacts == 0)
				printf("&nbsp;");
			printf("</TD>\n");

			get_interval_time_string(temp_host->notification_interval, time_string, sizeof(time_string));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->notification_interval == 0) ? "<i>再通知無し</I>" : html_encode(time_string, FALSE));

			get_interval_time_string(temp_host->first_notification_delay, time_string, sizeof(time_string));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, time_string);

			printf("<TD CLASS='%s'>", bg_class);
			options = 0;
			if(flag_isset(temp_host->notification_options, OPT_DOWN) == TRUE) {
				options = 1;
				printf("停止(DOWN)");
				}
			if(flag_isset(temp_host->notification_options, OPT_UNREACHABLE) == TRUE) {
				printf("%s未到達(UNREACHABLE)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_host->notification_options, OPT_RECOVERY) == TRUE) {
				printf("%s復旧(RECOVERY)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_host->notification_options, OPT_FLAPPING) == TRUE) {
				printf("%sフラッピング(FLAPPING)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_host->notification_options, OPT_DOWNTIME) == TRUE) {
				printf("%sダウンタイム", (options) ? ", " : "");
				options = 1;
				}
			if(options == 0)
				printf("無し");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			if(temp_host->notification_period == NULL)
				printf("&nbsp;");
			else
				printf("<a href='%s?type=timeperiods&expand=%s'>%s</a>", CONFIG_CGI, url_encode(temp_host->notification_period), html_encode(temp_host->notification_period, FALSE));
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			if(temp_host->event_handler == NULL)
				printf("&nbsp");
			else
				/* printf("<a href='%s?type=commands&expand=%s'>%s</a></TD>\n",CONFIG_CGI,url_encode(strtok(temp_host->event_handler,"!")),html_encode(temp_host->event_handler,FALSE)); */
				printf("<a href='%s?type=command&expand=%s'>%s</a></TD>\n", CONFIG_CGI, url_encode(temp_host->event_handler), html_encode(temp_host->event_handler, FALSE));
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			printf("%s\n", (temp_host->event_handler_enabled == TRUE) ? "有効" : "無効");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			options = 0;
			if(flag_isset(temp_host->stalking_options, OPT_UP) == TRUE) {
				options = 1;
				printf("稼動(UP)");
				}
			if(flag_isset(temp_host->stalking_options, OPT_DOWN) == TRUE) {
				printf("%s停止(DOWN)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_host->stalking_options, OPT_UNREACHABLE) == TRUE) {
				printf("%s未到達(UNREACHABLE)", (options) ? ", " : "");
				options = 1;
				}
			if(options == 0)
				printf("無し");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			printf("%s\n", (temp_host->flap_detection_enabled == TRUE) ? "有効" : "無効");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			if(temp_host->low_flap_threshold == 0.0)
				printf("グローバル設定\n");
			else
				printf("%3.1f%%\n", temp_host->low_flap_threshold);
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			if(temp_host->high_flap_threshold == 0.0)
				printf("グローバル設定\n");
			else
				printf("%3.1f%%\n", temp_host->high_flap_threshold);
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			options = 0;
			if(flag_isset(temp_host->flap_detection_options, OPT_UP) == TRUE) {
				options = 1;
				printf("Up稼動(UP)");
				}
			if(flag_isset(temp_host->flap_detection_options, OPT_DOWN) == TRUE) {
				printf("%s停止(DOWN)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_host->flap_detection_options, OPT_UNREACHABLE) == TRUE) {
				printf("%s未到達(UNREACHABLE)", (options) ? ", " : "");
				options = 1;
				}
			if(options == 0)
				printf("無し");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			printf("%s\n", (temp_host->process_performance_data == TRUE) ? "有効" : "無効");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_host->notes == NULL) ? "&nbsp;" : html_encode(temp_host->notes, FALSE));

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_host->notes_url == NULL) ? "&nbsp;" : html_encode(temp_host->notes_url, FALSE));

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_host->action_url == NULL) ? "&nbsp;" : html_encode(temp_host->action_url, FALSE));

			if(temp_host->have_2d_coords == FALSE)
				printf("<TD CLASS='%s'>&nbsp;</TD>", bg_class);
			else
				printf("<TD CLASS='%s'>%d,%d</TD>", bg_class, temp_host->x_2d, temp_host->y_2d);

			if(temp_host->have_3d_coords == FALSE)
				printf("<TD CLASS='%s'>&nbsp;</TD>", bg_class);
			else
				printf("<TD CLASS='%s'>%.2f,%.2f,%.2f</TD>", bg_class, temp_host->x_3d, temp_host->y_3d, temp_host->z_3d);

			if(temp_host->statusmap_image == NULL)
				printf("<TD CLASS='%s'>&nbsp;</TD>", bg_class);
			else
				printf("<TD CLASS='%s' valign='center'><img src='%s%s' border='0' width='20' height='20'> %s</TD>", bg_class, url_logo_images_path, temp_host->statusmap_image, html_encode(temp_host->statusmap_image, FALSE));

			if(temp_host->vrml_image == NULL)
				printf("<TD CLASS='%s'>&nbsp;</TD>", bg_class);
			else
				printf("<TD CLASS='%s' valign='center'><img src='%s%s' border='0' width='20' height='20'> %s</TD>", bg_class, url_logo_images_path, temp_host->vrml_image, html_encode(temp_host->vrml_image, FALSE));

			if(temp_host->icon_image == NULL)
				printf("<TD CLASS='%s'>&nbsp;</TD>", bg_class);
			else {
				process_macros_r(mac, temp_host->icon_image, &processed_string, 0);
				printf("<TD CLASS='%s' valign='center'><img src='%s%s' border='0' width='20' height='20'> %s</TD>", bg_class, url_logo_images_path, processed_string, html_encode(temp_host->icon_image, FALSE));
				free(processed_string);
				}

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_host->icon_image_alt == NULL) ? "&nbsp;" : html_encode(temp_host->icon_image_alt, FALSE));

			printf("<TD CLASS='%s'>", bg_class);
			options = 0;
			if(temp_host->retain_status_information == TRUE) {
				options = 1;
				printf("ステータス情報");
				}
			if(temp_host->retain_nonstatus_information == TRUE) {
				printf("%sステータス情報以外", (options == 1) ? ", " : "");
				options = 1;
				}
			if(options == 0)
				printf("無し");
			printf("</TD>\n");

			printf("</TR>\n");
			}

	printf("</TABLE>\n");
	printf("</DIV>\n");
	printf("</P>\n");

	return;
	}



void display_hostgroups(void) {
	hostgroup *temp_hostgroup;
	hostsmember *temp_hostsmember;
	int odd = 0;
	const char *bg_class = "";

	/* see if user is authorized to view hostgroup information... */
	if(is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		unauthorized_message();
		return;
		}

	printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>ホストグループ %s</DIV></P>\n",
	       (*to_expand == '\0' ? "" : html_encode(to_expand, FALSE)));

	printf("<P>\n");
	printf("<DIV ALIGN=CENTER>\n");

	printf("<TABLE BORDER=0 CLASS='data'>\n");
	printf("<TR>\n");
	printf("<TH CLASS='data'>グループ名</TH>");
	printf("<TH CLASS='data'>説明</TH>");
	printf("<TH CLASS='data'>このホストのメンバー</TH>");
	printf("<TH CLASS='data'>メモ</TH>");
	printf("<TH CLASS='data'>メモURL</TH>");
	printf("<TH CLASS='data'>アクションURL</TH>");
	printf("</TR>\n");

	/* check all the hostgroups... */
	for(temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) if(((*to_expand) == '\0') || (!strcmp(to_expand, temp_hostgroup->group_name))) {

			if(odd) {
				odd = 0;
				bg_class = "dataOdd";
				}
			else {
				odd = 1;
				bg_class = "dataEven";
				}

			printf("<TR CLASS='%s'>\n", bg_class);

			printf("<TD CLASS='%s'>%s</TD>", bg_class, html_encode(temp_hostgroup->group_name, FALSE));

			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_hostgroup->alias, FALSE));

			printf("<TD CLASS='%s'>", bg_class);

			/* find all the hosts that are members of this hostgroup... */
			for(temp_hostsmember = temp_hostgroup->members; temp_hostsmember != NULL; temp_hostsmember = temp_hostsmember->next) {

				if(temp_hostsmember != temp_hostgroup->members)
					printf(", ");
				printf("<A HREF='%s?type=hosts&expand=%s'>%s</A>\n", CONFIG_CGI, url_encode(temp_hostsmember->host_name), html_encode(temp_hostsmember->host_name, FALSE));
				}
			printf("</TD>\n");

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_hostgroup->notes == NULL) ? "&nbsp;" : html_encode(temp_hostgroup->notes, FALSE));

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_hostgroup->notes_url == NULL) ? "&nbsp;" : html_encode(temp_hostgroup->notes_url, FALSE));

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_hostgroup->action_url == NULL) ? "&nbsp;" : html_encode(temp_hostgroup->action_url, FALSE));

			printf("</TR>\n");
			}

	printf("</TABLE>\n");
	printf("</DIV>\n");
	printf("</P>\n");

	return;
	}



void display_servicegroups(void) {
	servicegroup *temp_servicegroup;
	servicesmember *temp_servicesmember;
	int odd = 0;
	const char *bg_class = "";

	/* see if user is authorized to view servicegroup information... */
	if(is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		unauthorized_message();
		return;
		}

	printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>サービスグループ %s</DIV></P>\n",
	       (*to_expand == '\0' ? "" : html_encode(to_expand, FALSE)));

	printf("<P>\n");
	printf("<DIV ALIGN=CENTER>\n");

	printf("<TABLE BORDER=0 CLASS='data'>\n");
	printf("<TR>\n");
	printf("<TH CLASS='data'>グループ名</TH>");
	printf("<TH CLASS='data'>説明</TH>");
	printf("<TH CLASS='data'>このサービスのメンバー</TH>");
	printf("<TH CLASS='data'>メモ</TH>");
	printf("<TH CLASS='data'>メモURL</TH>");
	printf("<TH CLASS='data'>アクションURL</TH>");
	printf("</TR>\n");

	/* check all the servicegroups... */
	for(temp_servicegroup = servicegroup_list; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next) if(((*to_expand) == '\0') || (!strcmp(to_expand, temp_servicegroup->group_name))) {

			if(odd) {
				odd = 0;
				bg_class = "dataOdd";
				}
			else {
				odd = 1;
				bg_class = "dataEven";
				}

			printf("<TR CLASS='%s'>\n", bg_class);

			printf("<TD CLASS='%s'>%s</TD>", bg_class, html_encode(temp_servicegroup->group_name, FALSE));

			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_servicegroup->alias, FALSE));

			printf("<TD CLASS='%s'>", bg_class);

			/* find all the services that are members of this servicegroup... */
			for(temp_servicesmember = temp_servicegroup->members; temp_servicesmember != NULL; temp_servicesmember = temp_servicesmember->next) {

				printf("%s<A HREF='%s?type=hosts&expand=%s'>%s</A> / ", (temp_servicesmember == temp_servicegroup->members) ? "" : ", ", CONFIG_CGI, url_encode(temp_servicesmember->host_name), html_encode(temp_servicesmember->host_name, FALSE));

				printf("<A HREF='%s?type=services&expand=%s#%s;", CONFIG_CGI, url_encode(temp_servicesmember->host_name), url_encode(temp_servicesmember->host_name));
				printf("%s'>%s</A>\n", url_encode(temp_servicesmember->service_description), html_encode(temp_servicesmember->service_description, FALSE));
				}

			printf("</TD>\n");

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_servicegroup->notes == NULL) ? "&nbsp;" : html_encode(temp_servicegroup->notes, FALSE));

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_servicegroup->notes_url == NULL) ? "&nbsp;" : html_encode(temp_servicegroup->notes_url, FALSE));

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_servicegroup->action_url == NULL) ? "&nbsp;" : html_encode(temp_servicegroup->action_url, FALSE));

			printf("</TR>\n");
			}

	printf("</TABLE>\n");
	printf("</DIV>\n");
	printf("</P>\n");

	return;
	}



void display_contacts(void) {
	contact *temp_contact;
	commandsmember *temp_commandsmember;
	int odd = 0;
	int options;
	int found;
	const char *bg_class = "";

	/* see if user is authorized to view contact information... */
	if(is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		unauthorized_message();
		return;
		}

	printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>通知先 %s</DIV></P>\n",
	       (*to_expand == '\0' ? "" : html_encode(to_expand, FALSE)));

	printf("<P>\n");
	printf("<DIV ALIGN=CENTER>\n");

	printf("<TABLE CLASS='data'>\n");

	printf("<TR>\n");
	printf("<TH CLASS='data'>通知先名</TH>");
	printf("<TH CLASS='data'>エイリアス</TH>");
	printf("<TH CLASS='data'>メールアドレス</TH>");
	printf("<TH CLASS='data'>携帯番号/アドレス</TH>");
	printf("<TH CLASS='data'>最小重要度</TH>");
	printf("<TH CLASS='data'>サービス通知オプション</TH>");
	printf("<TH CLASS='data'>ホスト通知オプション</TH>");
	printf("<TH CLASS='data'>サービス通知期間</TH>");
	printf("<TH CLASS='data'>ホスト通知期間</TH>");
	printf("<TH CLASS='data'>サービス通知コマンド</TH>");
	printf("<TH CLASS='data'>ホスト通知コマンド</TH>");
	printf("<TH CLASS='data'>保持オプション</TH>");
	printf("</TR>\n");

	/* check all contacts... */
	for(temp_contact = contact_list; temp_contact != NULL; temp_contact = temp_contact->next) if(((*to_expand) == '\0') || (!strcmp(to_expand, temp_contact->name))) {

			if(odd) {
				odd = 0;
				bg_class = "dataOdd";
				}
			else {
				odd = 1;
				bg_class = "dataEven";
				}

			printf("<TR CLASS='%s'>\n", bg_class);

			printf("<TD CLASS='%s'><A NAME='%s'>%s</a></TD>\n", bg_class, url_encode(temp_contact->name), html_encode(temp_contact->name, FALSE));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_contact->alias, FALSE));
			printf("<TD CLASS='%s'><A HREF='mailto:%s'>%s</A></TD>\n", bg_class, (temp_contact->email == NULL) ? "&nbsp;" : url_encode(temp_contact->email), (temp_contact->email == NULL) ? "&nbsp;" : html_encode(temp_contact->email, FALSE));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_contact->pager == NULL) ? "&nbsp;" : html_encode(temp_contact->pager, FALSE));
			printf("<TD CLASS='%s'>%u</TD>\n", bg_class, temp_contact->minimum_value);

			printf("<TD CLASS='%s'>", bg_class);
			options = 0;
			if(flag_isset(temp_contact->service_notification_options, OPT_UNKNOWN)) {
				options = 1;
				printf("不明(UNKNOWN)");
				}
			if(flag_isset(temp_contact->service_notification_options, OPT_WARNING)) {
				printf("%s警告(WARNING)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_contact->service_notification_options, OPT_CRITICAL)) {
				printf("%s異常(CRITICAL)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_contact->service_notification_options, OPT_RECOVERY)) {
				printf("%s復旧(RECOVERY)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_contact->service_notification_options, OPT_FLAPPING)) {
				printf("%sフラッピング(FLAPPING)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_contact->service_notification_options, OPT_DOWNTIME)) {
				printf("%sダウンタイム", (options) ? ", " : "");
				options = 1;
				}
			if(!options)
				printf("無し");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			options = 0;
			if(flag_isset(temp_contact->host_notification_options, OPT_DOWN) == TRUE) {
				options = 1;
				printf("停止(DOWN)");
				}
			if(flag_isset(temp_contact->host_notification_options, OPT_UNREACHABLE) == TRUE) {
				printf("%s未到達(UNREACHABLE)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_contact->host_notification_options, OPT_RECOVERY) == TRUE) {
				printf("%s復旧(RECOVERY)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_contact->host_notification_options, OPT_FLAPPING) == TRUE) {
				printf("%sフラッピング(FLAPPING)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_contact->host_notification_options, OPT_DOWNTIME) == TRUE) {
				printf("%sダウンタイム", (options) ? ", " : "");
				options = 1;
				}
			if(!options)
				printf("無し");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>\n", bg_class);
			if(temp_contact->service_notification_period == NULL)
				printf("&nbsp;");
			else
				printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_contact->service_notification_period), html_encode(temp_contact->service_notification_period, FALSE));
			printf("</TD>\n");

			printf("<TD CLASS='%s'>\n", bg_class);
			if(temp_contact->host_notification_period == NULL)
				printf("&nbsp;");
			else
				printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_contact->host_notification_period), html_encode(temp_contact->host_notification_period, FALSE));
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			found = FALSE;
			for(temp_commandsmember = temp_contact->service_notification_commands; temp_commandsmember != NULL; temp_commandsmember = temp_commandsmember->next) {

				if(temp_commandsmember != temp_contact->service_notification_commands)
					printf(", ");

				/* printf("<A HREF='%s?type=commands&expand=%s'>%s</A>",CONFIG_CGI,url_encode(strtok(temp_commandsmember->command,"!")),html_encode(temp_commandsmember->command,FALSE)); */
				printf("<A HREF='%s?type=command&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_commandsmember->command), html_encode(temp_commandsmember->command, FALSE));

				found = TRUE;
				}
			if(found == FALSE)
				printf("無し");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			found = FALSE;
			for(temp_commandsmember = temp_contact->host_notification_commands; temp_commandsmember != NULL; temp_commandsmember = temp_commandsmember->next) {

				if(temp_commandsmember != temp_contact->host_notification_commands)
					printf(", ");

				/* printf("<A HREF='%s?type=commands&expand=%s'>%s</A>",CONFIG_CGI,url_encode(strtok(temp_commandsmember->command,"!")),html_encode(temp_commandsmember->command,FALSE)); */
				printf("<A HREF='%s?type=command&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_commandsmember->command), html_encode(temp_commandsmember->command, FALSE));

				found = TRUE;
				}
			if(found == FALSE)
				printf("無し");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			options = 0;
			if(temp_contact->retain_status_information == TRUE) {
				options = 1;
				printf("ステータス情報");
				}
			if(temp_contact->retain_nonstatus_information == TRUE) {
				printf("%sステータス情報以外", (options == 1) ? ", " : "");
				options = 1;
				}
			if(options == 0)
				printf("無し");
			printf("</TD>\n");

			printf("</TR>\n");
			}

	printf("</TABLE>\n");
	printf("</DIV>\n");
	printf("</P>\n");

	return;
	}



void display_contactgroups(void) {
	contactgroup *temp_contactgroup;
	contactsmember *temp_contactsmember;
	int odd = 0;
	const char *bg_class = "";

	/* see if user is authorized to view contactgroup information... */
	if(is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		unauthorized_message();
		return;
		}


	printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>通知先グループ %s</DIV></P>\n",
	       (*to_expand == '\0' ? "" : html_encode(to_expand, FALSE)));

	printf("<P>\n");
	printf("<DIV ALIGN=CENTER>\n");

	printf("<TABLE BORDER=0 CELLSPACING=3 CELLPADDING=0>\n");

	printf("<TR>\n");
	printf("<TH CLASS='data'>グループ名</TH>\n");
	printf("<TH CLASS='data'>説明</TH>\n");
	printf("<TH CLASS='data'>このグループのメンバー</TH>\n");
	printf("</TR>\n");

	/* check all the contact groups... */
	for(temp_contactgroup = contactgroup_list; temp_contactgroup != NULL; temp_contactgroup = temp_contactgroup->next) if(((*to_expand) == '\0') || (!strcmp(to_expand, temp_contactgroup->group_name))) {

			if(odd) {
				odd = 0;
				bg_class = "dataOdd";
				}
			else {
				odd = 1;
				bg_class = "dataEven";
				}

			printf("<TR CLASS='%s'>\n", bg_class);

			printf("<TD CLASS='%s'><A NAME='%s'></A>%s</TD>\n", bg_class, url_encode(temp_contactgroup->group_name), html_encode(temp_contactgroup->group_name, FALSE));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_contactgroup->alias, FALSE));

			/* find all the contact who are members of this contact group... */
			printf("<TD CLASS='%s'>", bg_class);
			for(temp_contactsmember = temp_contactgroup->members; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {

				if(temp_contactsmember != temp_contactgroup->members)
					printf(", ");

				printf("<A HREF='%s?type=contacts&expand=%s'>%s</A>\n", CONFIG_CGI, url_encode(temp_contactsmember->contact_name), html_encode(temp_contactsmember->contact_name, FALSE));
				}
			printf("</TD>\n");

			printf("</TR>\n");
			}

	printf("</TABLE>\n");
	printf("</DIV>\n");
	printf("</P>\n");

	return;
	}



void display_services(void) {
	service *temp_service = NULL;
	contactsmember *temp_contactsmember = NULL;
	contactgroupsmember *temp_contactgroupsmember = NULL;
	char *processed_string = NULL;
	char command_line[MAX_INPUT_BUFFER];
	char *command_name;
	int options;
	int odd = 0;
	char time_string[24];
	const char *bg_class;
	int num_contacts = 0;


	/* see if user is authorized to view service information... */
	if(is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		unauthorized_message();
		return;
		}

	printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>%sサービス %s</DIV></P>\n",
	       (*to_expand == '\0' ? "" : "次のホストまたは指定された"), (*to_expand == '\0' ? "" : html_encode(to_expand, FALSE)));

	printf("<P>\n");
	printf("<DIV ALIGN=CENTER>\n");

	printf("<TABLE BORDER=0 CLASS='data'>\n");
	printf("<TR>\n");
	printf("<TH CLASS='data' COLSPAN=2>サービス</TH>");
	printf("</TR>\n");
	printf("<TR>\n");
	printf("<TH CLASS='data'>ホスト</TH>\n");
	printf("<TH CLASS='data'>説明</TH>\n");
	printf("<TH CLASS='data'>重要度</TH>\n");
	printf("<TH CLASS='data'>最大チェック数</TH>\n");
	printf("<TH CLASS='data'>通常のチェック間隔</TH>\n");
	printf("<TH CLASS='data'>再試行のチェック間隔</TH>\n");
	printf("<TH CLASS='data'>チェックコマンド</TH>\n");
	printf("<TH CLASS='data'>チェック時間帯</TH>\n");
	printf("<TH CLASS='data'>並列処理</TH>\n");
	printf("<TH CLASS='data'>Volatile</TH>\n");
	printf("<TH CLASS='data'>Obsess Over</TH>\n");
	printf("<TH CLASS='data'>アクティブチェック</TH>\n");
	printf("<TH CLASS='data'>パッシブチェック</TH>\n");
	printf("<TH CLASS='data'>鮮度チェック(Freshness)</TH>\n");
	printf("<TH CLASS='data'>鮮度しきい値(Freshness)</TH>\n");
	printf("<TH CLASS='data'>デフォルトの通知先/グループ</TH>\n");
	printf("<TH CLASS='data'>通知</TH>\n");
	printf("<TH CLASS='data'>通知間隔</TH>\n");
	printf("<TH CLASS='data'>初回通知遅延時間</TH>\n");
	printf("<TH CLASS='data'>通知オプション</TH>\n");
	printf("<TH CLASS='data'>通知時間帯</TH>\n");
	printf("<TH CLASS='data'>イベントハンドラ</TH>");
	printf("<TH CLASS='data'>イベントハンドラの有効化</TH>");
	printf("<TH CLASS='data'>追跡オプション</TH>\n");
	printf("<TH CLASS='data'>フラップ検知設定</TH>");
	printf("<TH CLASS='data'>低フラップ閾値</TH>");
	printf("<TH CLASS='data'>高フラップ閾値</TH>");
	printf("<TH CLASS='data'>フラップ検知オプション</TH>");
	printf("<TH CLASS='data'>パフォーマンスデータ処理</TH>");
	printf("<TH CLASS='data'>メモ</TH>");
	printf("<TH CLASS='data'>メモURL</TH>");
	printf("<TH CLASS='data'>アクションURL</TH>");
	printf("<TH CLASS='data'>ロゴ画像</TH>");
	printf("<TH CLASS='data'>画像代替文字</TH>");
	printf("<TH CLASS='data'>ステータス保持オプション</TH>");
	printf("</TR>\n");

	/* check all the services... */
	for(temp_service = service_list; temp_service != NULL; temp_service = temp_service->next)
		if(((*to_expand) == '\0') || (!strcmp(to_expand, temp_service->host_name)) || (!strcmp(to_expand, temp_service->description))) {

			/* grab macros */
			grab_service_macros_r(mac, temp_service);

			if(odd) {
				odd = 0;
				bg_class = "dataOdd";
				}
			else {
				odd = 1;
				bg_class = "dataEven";
				}

			printf("<TR CLASS='%s'>\n", bg_class);

			printf("<TD CLASS='%s'><A NAME='%s;", bg_class, url_encode(temp_service->host_name));
			printf("%s'></A>", url_encode(temp_service->description));
			printf("<A HREF='%s?type=hosts&expand=%s'>%s</A></TD>\n", CONFIG_CGI, url_encode(temp_service->host_name), html_encode(temp_service->host_name, FALSE));

			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_service->description, FALSE));
			printf("<TD CLASS='%s'>%u</TD>\n", bg_class, temp_service->hourly_value);

			printf("<TD CLASS='%s'>%d</TD>\n", bg_class, temp_service->max_attempts);

			get_interval_time_string(temp_service->check_interval, time_string, sizeof(time_string));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, time_string);
			get_interval_time_string(temp_service->retry_interval, time_string, sizeof(time_string));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, time_string);

			strncpy(command_line, temp_service->check_command, sizeof(command_line));
			command_line[sizeof(command_line) - 1] = '\x0';
			command_name = strtok(strdup(command_line), "!");

			/* printf("<TD CLASS='%s'><A HREF='%s?type=commands&expand=%s'>%s</A></TD>\n",bg_class,CONFIG_CGI,url_encode(command_name),html_encode(command_line,FALSE)); */
			printf("<TD CLASS='%s'><A HREF='%s?type=command&expand=%s'>%s</A></TD>\n", bg_class, CONFIG_CGI, url_encode(command_line), html_encode(command_line, FALSE));
			free(command_name);
			printf("<TD CLASS='%s'>", bg_class);
			if(temp_service->check_period == NULL)
				printf("&nbsp;");
			else
				printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_service->check_period), html_encode(temp_service->check_period, FALSE));
			printf("</TD>\n");

			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->parallelize == TRUE) ? "有効" : "無効");

			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->is_volatile == TRUE) ? "有効" : "無効");

			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->obsess == TRUE) ? "有効" : "無効");

			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->checks_enabled == TRUE) ? "有効" : "無効");

			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->accept_passive_checks == TRUE) ? "有効" : "無効");

			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->check_freshness == TRUE) ? "有効" : "無効");

			printf("<TD CLASS='%s'>", bg_class);
			if(temp_service->freshness_threshold == 0)
				printf("自動設定\n");
			else
				printf("%d 秒\n", temp_service->freshness_threshold);
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			num_contacts = 0;
			for(temp_contactsmember = temp_service->contacts; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {
				num_contacts++;
				if(num_contacts > 1)
					printf(", ");
				printf("<A HREF='%s?type=contacts&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_contactsmember->contact_name), html_encode(temp_contactsmember->contact_name, FALSE));
				}
			for(temp_contactgroupsmember = temp_service->contact_groups; temp_contactgroupsmember != NULL; temp_contactgroupsmember = temp_contactgroupsmember->next) {
				num_contacts++;
				if(num_contacts > 1)
					printf(", ");
				printf("<A HREF='%s?type=contactgroups&expand=%s'>%s</A>\n", CONFIG_CGI, url_encode(temp_contactgroupsmember->group_name), html_encode(temp_contactgroupsmember->group_name, FALSE));
				}
			if(num_contacts == 0)
				printf("&nbsp;");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			printf("%s\n", (temp_service->notifications_enabled == TRUE) ? "有効" : "無効");
			printf("</TD>\n");

			get_interval_time_string(temp_service->notification_interval, time_string, sizeof(time_string));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->notification_interval == 0) ? "<i>再通知無し</i>" : html_encode(time_string, FALSE));

			get_interval_time_string(temp_service->first_notification_delay, time_string, sizeof(time_string));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, time_string);

			printf("<TD CLASS='%s'>", bg_class);
			options = 0;
			if(flag_isset(temp_service->notification_options, OPT_UNKNOWN) == TRUE) {
				options = 1;
				printf("不明(UNKNOWN)");
				}
			if(flag_isset(temp_service->notification_options, OPT_WARNING) == TRUE) {
				printf("%s警告(WARNING)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_service->notification_options, OPT_CRITICAL) == TRUE) {
				printf("%s異常(CRITICAL)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_service->notification_options, OPT_RECOVERY) == TRUE) {
				printf("%s復旧(RECOVERY)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_service->notification_options, OPT_FLAPPING) == TRUE) {
				printf("%sフラッピング(FLAPPING)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_service->notification_options, OPT_DOWNTIME) == TRUE) {
				printf("%sダウンタイム", (options) ? ", " : "");
				options = 1;
				}
			if(!options)
				printf("無し");
			printf("</TD>\n");
			printf("<TD CLASS='%s'>", bg_class);
			if(temp_service->notification_period == NULL)
				printf("&nbsp;");
			else
				printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_service->notification_period), html_encode(temp_service->notification_period, FALSE));
			printf("</TD>\n");
			printf("<TD CLASS='%s'>", bg_class);
			if(temp_service->event_handler == NULL)
				printf("&nbsp;");
			else
				/* printf("<A HREF='%s?type=commands&expand=%s'>%s</A>",CONFIG_CGI,url_encode(strtok(temp_service->event_handler,"!")),html_encode(temp_service->event_handler,FALSE)); */
				printf("<A HREF='%s?type=command&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_service->event_handler), html_encode(temp_service->event_handler, FALSE));
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			printf("%s\n", (temp_service->event_handler_enabled == TRUE) ? "有効" : "無効");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			options = 0;
			if(flag_isset(temp_service->stalking_options, OPT_OK) == TRUE) {
				options = 1;
				printf("正常(OK)");
				}
			if(flag_isset(temp_service->stalking_options, OPT_WARNING) == TRUE) {
				printf("%s警告(WARNING)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_service->stalking_options, OPT_UNKNOWN) == TRUE) {
				printf("%s不明(UNKNOWN)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_service->stalking_options, OPT_CRITICAL) == TRUE) {
				printf("%s異常(CRITICAL)", (options) ? ", " : "");
				options = 1;
				}
			if(options == 0)
				printf("無し");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			printf("%s\n", (temp_service->flap_detection_enabled == TRUE) ? "有効" : "無効");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			if(temp_service->low_flap_threshold == 0.0)
				printf("グローバル設定\n");
			else
				printf("%3.1f%%\n", temp_service->low_flap_threshold);
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			if(temp_service->high_flap_threshold == 0.0)
				printf("グローバル設定\n");
			else
				printf("%3.1f%%\n", temp_service->high_flap_threshold);
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			options = 0;
			if(flag_isset(temp_service->flap_detection_options, OPT_OK) == TRUE) {
				options = 1;
				printf("稼動(UP)");
				}
			if(flag_isset(temp_service->flap_detection_options, OPT_WARNING) == TRUE) {
				printf("%s警告(WARNING)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_service->flap_detection_options, OPT_UNKNOWN) == TRUE) {
				printf("%s不明(UNKNOWN)", (options) ? ", " : "");
				options = 1;
				}
			if(flag_isset(temp_service->flap_detection_options, OPT_CRITICAL) == TRUE) {
				printf("%s異常(CRITICAL)", (options) ? ", " : "");
				options = 1;
				}
			if(options == 0)
				printf("無し");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>", bg_class);
			printf("%s\n", (temp_service->process_performance_data == TRUE) ? "有効" : "無効");
			printf("</TD>\n");

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_service->notes == NULL) ? "&nbsp;" : html_encode(temp_service->notes, FALSE));

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_service->notes_url == NULL) ? "&nbsp;" : html_encode(temp_service->notes_url, FALSE));

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_service->action_url == NULL) ? "&nbsp;" : html_encode(temp_service->action_url, FALSE));

			if(temp_service->icon_image == NULL)
				printf("<TD CLASS='%s'>&nbsp;</TD>", bg_class);
			else {
				process_macros_r(mac, temp_service->icon_image, &processed_string, 0);
				printf("<TD CLASS='%s' valign='center'><img src='%s%s' border='0' width='20' height='20'> %s</TD>", bg_class, url_logo_images_path, processed_string, html_encode(temp_service->icon_image, FALSE));
				free(processed_string);
				}

			printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_service->icon_image_alt == NULL) ? "&nbsp;" : html_encode(temp_service->icon_image_alt, FALSE));

			printf("<TD CLASS='%s'>", bg_class);
			options = 0;
			if(temp_service->retain_status_information == TRUE) {
				options = 1;
				printf("ステータス情報");
				}
			if(temp_service->retain_nonstatus_information == TRUE) {
				printf("%sステータス情報以外", (options == 1) ? ", " : "");
				options = 1;
				}
			if(options == 0)
				printf("無し");
			printf("</TD>\n");

			printf("</TR>\n");
			}

	printf("</TABLE>\n");
	printf("</DIV>\n");
	printf("</P>\n");

	return;
	}



void display_timeperiods(void) {
	timerange *temp_timerange = NULL;
	daterange *temp_daterange = NULL;
	timeperiod *temp_timeperiod = NULL;
	timeperiodexclusion *temp_timeperiodexclusion = NULL;
	char *months[12] = {"1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月"};
	char *days[7] = {"日曜日", "月曜日", "火曜日", "水曜日", "木曜日", "金曜日", "土曜日"};
	int odd = 0;
	int day = 0;
	int x = 0;
	const char *bg_class = "";
	char timestring[10];
	int hours = 0;
	int minutes = 0;
	int seconds = 0;
	int line = 0;
	int item = 0;

	/* see if user is authorized to view time period information... */
	if(is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		unauthorized_message();
		return;
		}

	printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>時間帯 %s</DIV></P>\n",
	       (*to_expand == '\0' ? "" : html_encode(to_expand, FALSE)));

	printf("<P>\n");
	printf("<DIV ALIGN=CENTER>\n");

	printf("<TABLE BORDER=0 CLASS='data'>\n");
	printf("<TR>\n");
	printf("<TH CLASS='data'>名前</TH>\n");
	printf("<TH CLASS='data'>エイリアス/説明</TH>\n");
	printf("<TH CLASS='data'>除外</TH>\n");
	printf("<TH CLASS='data'>曜日/日付</TH>\n");
	printf("<TH CLASS='data'>時間</TH>\n");
	printf("</TR>\n");

	/* check all the time periods... */
	for(temp_timeperiod = timeperiod_list; temp_timeperiod != NULL; temp_timeperiod = temp_timeperiod->next) if(((*to_expand) == '\0') || (!strcmp(to_expand, temp_timeperiod->name))) {

			if(odd) {
				odd = 0;
				bg_class = "dataOdd";
				}
			else {
				odd = 1;
				bg_class = "dataEven";
				}

			printf("<TR CLASS='%s'>\n", bg_class);

			printf("<TD CLASS='%s'><A NAME='%s'>%s</A></TD>\n", bg_class, url_encode(temp_timeperiod->name), html_encode(temp_timeperiod->name, FALSE));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_timeperiod->alias, FALSE));

			printf("<TD CLASS='%s'>", bg_class);
			item = 0;
			for(temp_timeperiodexclusion = temp_timeperiod->exclusions; temp_timeperiodexclusion != NULL; temp_timeperiodexclusion = temp_timeperiodexclusion->next) {
				item++;
				printf("%s<A HREF='#%s'>%s</A>", (item == 1) ? "" : ",&nbsp;", url_encode(temp_timeperiodexclusion->timeperiod_name), html_encode(temp_timeperiodexclusion->timeperiod_name, FALSE));
				}
			printf("</TD>");

			printf("<TD CLASS='%s'>", bg_class);

			line = 0;
			for(x = 0; x < DATERANGE_TYPES; x++) {
				for(temp_daterange = temp_timeperiod->exceptions[x]; temp_daterange != NULL; temp_daterange = temp_daterange->next) {

					line++;

					if(line > 1)
						printf("<TR><TD COLSPAN='3'></TD><TD CLASS='%s'>\n", bg_class);

					switch(temp_daterange->type) {
						case DATERANGE_CALENDAR_DATE:
							printf("%d-%02d-%02d", temp_daterange->syear, temp_daterange->smon + 1, temp_daterange->smday);
							if((temp_daterange->smday != temp_daterange->emday) || (temp_daterange->smon != temp_daterange->emon) || (temp_daterange->syear != temp_daterange->eyear))
								printf(" - %d-%02d-%02d", temp_daterange->eyear, temp_daterange->emon + 1, temp_daterange->emday);
							if(temp_daterange->skip_interval > 1)
								printf(" / %d", temp_daterange->skip_interval);
							break;
						case DATERANGE_MONTH_DATE:
							printf("%s %d", months[temp_daterange->smon], temp_daterange->smday);
							if((temp_daterange->smon != temp_daterange->emon) || (temp_daterange->smday != temp_daterange->emday)) {
								printf(" - %s %d", months[temp_daterange->emon], temp_daterange->emday);
								if(temp_daterange->skip_interval > 1)
									printf(" / %d", temp_daterange->skip_interval);
								}
							break;
						case DATERANGE_MONTH_DAY:
							printf("day %d", temp_daterange->smday);
							if(temp_daterange->smday != temp_daterange->emday) {
								printf(" - %d", temp_daterange->emday);
								if(temp_daterange->skip_interval > 1)
									printf(" / %d", temp_daterange->skip_interval);
								}
							break;
						case DATERANGE_MONTH_WEEK_DAY:
							printf("%s %d %s", days[temp_daterange->swday], temp_daterange->swday_offset, months[temp_daterange->smon]);
							if((temp_daterange->smon != temp_daterange->emon) || (temp_daterange->swday != temp_daterange->ewday) || (temp_daterange->swday_offset != temp_daterange->ewday_offset)) {
								printf(" - %s %d %s", days[temp_daterange->ewday], temp_daterange->ewday_offset, months[temp_daterange->emon]);
								if(temp_daterange->skip_interval > 1)
									printf(" / %d", temp_daterange->skip_interval);
								}
							break;
						case DATERANGE_WEEK_DAY:
							printf("%s %d", days[temp_daterange->swday], temp_daterange->swday_offset);
							if((temp_daterange->swday != temp_daterange->ewday) || (temp_daterange->swday_offset != temp_daterange->ewday_offset)) {
								printf(" - %s %d", days[temp_daterange->ewday], temp_daterange->ewday_offset);
								if(temp_daterange->skip_interval > 1)
									printf(" / %d", temp_daterange->skip_interval);
								}
							break;
						default:
							break;
						}

					printf("</TD><TD CLASS='%s'>\n", bg_class);

					for(temp_timerange = temp_daterange->times; temp_timerange != NULL; temp_timerange = temp_timerange->next) {

						if(temp_timerange != temp_daterange->times)
							printf(", ");

						hours = temp_timerange->range_start / 3600;
						minutes = (temp_timerange->range_start - (hours * 3600)) / 60;
						seconds = temp_timerange->range_start - (hours * 3600) - (minutes * 60);
						snprintf(timestring, sizeof(timestring) - 1, "%02d:%02d:%02d", hours, minutes, seconds);
						timestring[sizeof(timestring) - 1] = '\x0';
						printf("%s - ", timestring);

						hours = temp_timerange->range_end / 3600;
						minutes = (temp_timerange->range_end - (hours * 3600)) / 60;
						seconds = temp_timerange->range_end - (hours * 3600) - (minutes * 60);
						snprintf(timestring, sizeof(timestring) - 1, "%02d:%02d:%02d", hours, minutes, seconds);
						timestring[sizeof(timestring) - 1] = '\x0';
						printf("%s", timestring);
						}

					printf("</TD>\n");
					printf("</TR>\n");
					}
				}
			for(day = 0; day < 7; day++) {

				if(temp_timeperiod->days[day] == NULL)
					continue;

				line++;

				if(line > 1)
					printf("<TR><TD COLSPAN='3'></TD><TD CLASS='%s'>\n", bg_class);

				printf("%s", days[day]);

				printf("</TD><TD CLASS='%s'>\n", bg_class);

				for(temp_timerange = temp_timeperiod->days[day]; temp_timerange != NULL; temp_timerange = temp_timerange->next) {

					if(temp_timerange != temp_timeperiod->days[day])
						printf(", ");

					hours = temp_timerange->range_start / 3600;
					minutes = (temp_timerange->range_start - (hours * 3600)) / 60;
					seconds = temp_timerange->range_start - (hours * 3600) - (minutes * 60);
					snprintf(timestring, sizeof(timestring) - 1, "%02d:%02d:%02d", hours, minutes, seconds);
					timestring[sizeof(timestring) - 1] = '\x0';
					printf("%s - ", timestring);

					hours = temp_timerange->range_end / 3600;
					minutes = (temp_timerange->range_end - (hours * 3600)) / 60;
					seconds = temp_timerange->range_end - (hours * 3600) - (minutes * 60);
					snprintf(timestring, sizeof(timestring) - 1, "%02d:%02d:%02d", hours, minutes, seconds);
					timestring[sizeof(timestring) - 1] = '\x0';
					printf("%s", timestring);
					}

				printf("&nbsp;</TD>\n");
				printf("</TR>\n");
				}

			if(line == 0) {
				printf("</TD>\n");
				printf("</TR>\n");
				}
			}

	printf("</TABLE>\n");
	printf("</DIV>\n");
	printf("</P>\n");

	return;
	}



void display_commands(void) {
	command *temp_command;
	int odd = 0;
	const char *bg_class = "";

	/* see if user is authorized to view command information... */
	if(is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		unauthorized_message();
		return;
		}

	printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>コマンド %s</DIV></P>\n",
	       (*to_expand == '\0' ? "" : html_encode(to_expand, FALSE)));

	printf("<P><DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 CLASS='data'>\n");
	printf("<TR><TH CLASS='data'>Command Name</TH><TH CLASS='data'>コマンドライン</TH></TR>\n");

	/* check all commands */
	for(temp_command = command_list; temp_command != NULL; temp_command = temp_command->next) if(((*to_expand) == '\0') || (!strcmp(to_expand, temp_command->name))) {

			if(odd) {
				odd = 0;
				bg_class = "dataEven";
				}
			else {
				odd = 1;
				bg_class = "dataOdd";
				}

			printf("<TR CLASS='%s'>\n", bg_class);

			printf("<TD CLASS='%s'><A NAME='%s'></A>%s</TD>\n", bg_class, url_encode(temp_command->name), html_encode(temp_command->name, FALSE));
			printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_command->command_line, FALSE));

			printf("</TR>\n");
			}

	printf("</TABLE>\n");
	printf("</DIV></P>\n");

	return;
	}


static void display_servicedependency(servicedependency *temp_sd)
{
	const char *bg_class;
	static int odd = 0;
	int options;

	if(*to_expand != '\0' && (strcmp(to_expand, temp_sd->dependent_host_name) || strcmp(to_expand, temp_sd->host_name)))
		return;

	if(odd)
		bg_class = "dataOdd";
	else
		bg_class = "dataEven";
	odd ^= 1; /* xor with 1 always flips the switch */

	printf("<TR CLASS='%s'>\n", bg_class);

	printf("<TD CLASS='%s'><A HREF='%s?type=hosts&expand=%s'>%s</A></TD>", bg_class, CONFIG_CGI, url_encode(temp_sd->dependent_host_name), html_encode(temp_sd->dependent_host_name, FALSE));

	printf("<TD CLASS='%s'><A HREF='%s?type=services&expand=%s#%s;", bg_class, CONFIG_CGI, url_encode(temp_sd->dependent_host_name), url_encode(temp_sd->dependent_host_name));
	printf("%s'>%s</A></TD>\n", url_encode(temp_sd->dependent_service_description), html_encode(temp_sd->dependent_service_description, FALSE));

	printf("<TD CLASS='%s'><A HREF='%s?type=hosts&expand=%s'>%s</A></TD>", bg_class, CONFIG_CGI, url_encode(temp_sd->host_name), html_encode(temp_sd->host_name, FALSE));

	printf("<TD CLASS='%s'><A HREF='%s?type=services&expand=%s#%s;", bg_class, CONFIG_CGI, url_encode(temp_sd->host_name), url_encode(temp_sd->host_name));
	printf("%s'>%s</A></TD>\n", url_encode(temp_sd->service_description), html_encode(temp_sd->service_description, FALSE));

	printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_sd->dependency_type == NOTIFICATION_DEPENDENCY) ? "通知" : "チェック実行");

	printf("<TD CLASS='%s'>", bg_class);
	if(temp_sd->dependency_period == NULL)
		printf("&nbsp;");
	else
		printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_sd->dependency_period), html_encode(temp_sd->dependency_period, FALSE));
	printf("</TD>\n");

	printf("<TD CLASS='%s'>", bg_class);
	options = FALSE;
	if(flag_isset(temp_sd->failure_options, OPT_OK) == TRUE) {
		printf("稼働(OK)");
		options = TRUE;
		}
	if(flag_isset(temp_sd->failure_options, OPT_WARNING) == TRUE) {
		printf("%s警告(WARNING)", (options == TRUE) ? ", " : "");
		options = TRUE;
		}
	if(flag_isset(temp_sd->failure_options, OPT_UNKNOWN) == TRUE) {
		printf("%s不明(UNKNOWN)", (options == TRUE) ? ", " : "");
		options = TRUE;
		}
	if(flag_isset(temp_sd->failure_options, OPT_CRITICAL) == TRUE) {
		printf("%s異常(CRITICAL)", (options == TRUE) ? ", " : "");
		options = TRUE;
		}
	if(flag_isset(temp_sd->failure_options, OPT_PENDING) == TRUE) {
		printf("%s未解決(PENDING)", (options == TRUE) ? ", " : "");
		options = TRUE;
		}
	printf("</TD>\n");
	printf("</TR>\n");
	}

void display_servicedependencies(void) {
	unsigned int i;

	/* see if user is authorized to view hostgroup information... */
	if(is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		unauthorized_message();
		return;
		}

	printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>%sサービス依存関係 %s</DIV></P>\n",
	       (*to_expand == '\0' ? "" : "次のホストを含む"), (*to_expand == '\0' ? "" : html_encode(to_expand, FALSE)));

	printf("<P>\n");
	printf("<DIV ALIGN=CENTER>\n");

	printf("<TABLE BORDER=0 CLASS='data'>\n");
	printf("<TR>\n");
	printf("<TH CLASS='data' COLSPAN=2>依存サービス</TH>");
	printf("<TH CLASS='data' COLSPAN=2>上位サービス</TH>");
	printf("</TR>\n");
	printf("<TR>\n");
	printf("<TH CLASS='data'>ホスト</TH>");
	printf("<TH CLASS='data'>サービス</TH>");
	printf("<TH CLASS='data'>ホスト</TH>");
	printf("<TH CLASS='data'>サービス</TH>");
	printf("<TH CLASS='data'>依存タイプ</TH>");
	printf("<TH CLASS='data'>依存期間</TH>");
	printf("<TH CLASS='data'>依存失敗オプション</TH>");
	printf("</TR>\n");

	for(i = 0; i < num_objects.servicedependencies; i++) {
		display_servicedependency(servicedependency_ary[i]);
		}

	printf("</TABLE>\n");
	printf("</DIV>\n");
	printf("</P>\n");

	return;
	}



void display_serviceescalations(void) {
	serviceescalation *temp_se = NULL;
	contactsmember *temp_contactsmember = NULL;
	contactgroupsmember *temp_contactgroupsmember = NULL;
	char time_string[24] = "";
	int options = FALSE;
	int odd = 0;
	const char *bg_class = "";
	int num_contacts = 0;
	unsigned int i;

	/* see if user is authorized to view hostgroup information... */
	if(is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		unauthorized_message();
		return;
		}

	printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>%sサービスエスカレーション %s</DIV></P>\n",
	       (*to_expand == '\0' ? "" : "次のホストの"), (*to_expand == '\0' ? "" : html_encode(to_expand, FALSE)));

	printf("<P>\n");
	printf("<DIV ALIGN=CENTER>\n");

	printf("<TABLE BORDER=0 CLASS='data'>\n");
	printf("<TR>\n");
	printf("<TH CLASS='data' COLSPAN=2>サービス</TH>");
	printf("</TR>\n");
	printf("<TR>\n");
	printf("<TH CLASS='data'>ホスト</TH>");
	printf("<TH CLASS='data'>説明</TH>");
	printf("<TH CLASS='data'>通知先/グループ</TH>");
	printf("<TH CLASS='data'>初回通知</TH>");
	printf("<TH CLASS='data'>最終通知</TH>");
	printf("<TH CLASS='data'>通知間隔</TH>");
	printf("<TH CLASS='data'>エスカレーション期間</TH>");
	printf("<TH CLASS='data'>エスカレーションオプション</TH>");
	printf("</TR>\n");


	for(i = 0; i < num_objects.serviceescalations; i++) {
		temp_se = serviceescalation_ary[i];
		if(*to_expand != '\0' && strcmp(to_expand, temp_se->host_name))
			continue;

		if(odd) {
			odd = 0;
			bg_class = "dataOdd";
			}
		else {
			odd = 1;
			bg_class = "dataEven";
			}

		printf("<TR CLASS='%s'>\n", bg_class);

		printf("<TD CLASS='%s'><A HREF='%s?type=hosts&expand=%s'>%s</A></TD>", bg_class, CONFIG_CGI, url_encode(temp_se->host_name), html_encode(temp_se->host_name, FALSE));

		printf("<TD CLASS='%s'><A HREF='%s?type=services&expand=%s#%s;", bg_class, CONFIG_CGI, url_encode(temp_se->host_name), url_encode(temp_se->host_name));
		printf("%s'>%s</A></TD>\n", url_encode(temp_se->description), html_encode(temp_se->description, FALSE));

		printf("<TD CLASS='%s'>", bg_class);
		num_contacts = 0;
		for(temp_contactsmember = temp_se->contacts; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {
			num_contacts++;
			if(num_contacts > 1)
				printf(", ");
			printf("<A HREF='%s?type=contacts&expand=%s'>%s</A>\n", CONFIG_CGI, url_encode(temp_contactsmember->contact_name), html_encode(temp_contactsmember->contact_name, FALSE));
			}
		for(temp_contactgroupsmember = temp_se->contact_groups; temp_contactgroupsmember != NULL; temp_contactgroupsmember = temp_contactgroupsmember->next) {
			num_contacts++;
			if(num_contacts > 1)
				printf(", ");
			printf("<A HREF='%s?type=contactgroups&expand=%s'>%s</A>\n", CONFIG_CGI, url_encode(temp_contactgroupsmember->group_name), html_encode(temp_contactgroupsmember->group_name, FALSE));
			}
		if(num_contacts == 0)
			printf("&nbsp;");
		printf("</TD>\n");

		printf("<TD CLASS='%s'>%d</TD>", bg_class, temp_se->first_notification);

		printf("<TD CLASS='%s'>", bg_class);
		if(temp_se->last_notification == 0)
			printf("無限");
		else
			printf("%d", temp_se->last_notification);
		printf("</TD>\n");

		get_interval_time_string(temp_se->notification_interval, time_string, sizeof(time_string));
		printf("<TD CLASS='%s'>", bg_class);
		if(temp_se->notification_interval == 0.0)
			printf("1回のみ(再通知無し)");
		else
			printf("%s", time_string);
		printf("</TD>\n");

		printf("<TD CLASS='%s'>", bg_class);
		if(temp_se->escalation_period == NULL)
			printf("&nbsp;");
		else
			printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_se->escalation_period), html_encode(temp_se->escalation_period, FALSE));
		printf("</TD>\n");

		printf("<TD CLASS='%s'>", bg_class);
		options = FALSE;
		if(flag_isset(temp_se->escalation_options, OPT_WARNING) == TRUE) {
			printf("%s警告(WARNING)", (options == TRUE) ? ", " : "");
			options = TRUE;
			}
		if(flag_isset(temp_se->escalation_options, OPT_UNKNOWN) == TRUE) {
			printf("%s不明(UNKNOWN)", (options == TRUE) ? ", " : "");
			options = TRUE;
			}
		if(flag_isset(temp_se->escalation_options, OPT_CRITICAL) == TRUE) {
			printf("%s異常(CRITICAL)", (options == TRUE) ? ", " : "");
			options = TRUE;
			}
		if(flag_isset(temp_se->escalation_options, OPT_RECOVERY) == TRUE) {
			printf("%s復旧(RECOVERY)", (options == TRUE) ? ", " : "");
			options = TRUE;
			}
		if(options == FALSE)
			printf("無し");
		printf("</TD>\n");

		printf("</TR>\n");
		}

	printf("</TABLE>\n");
	printf("</DIV>\n");
	printf("</P>\n");

	return;
	}

static void display_hostdependency(hostdependency *temp_hd)
{
	int options;
	const char *bg_class = "";
	static int odd = 0;

	if(*to_expand != '\0' && (strcmp(to_expand, temp_hd->dependent_host_name) && !strcmp(to_expand, temp_hd->host_name)))
		return;

	if(odd)
		bg_class = "dataOdd";
	else
		bg_class = "dataEven";
	odd ^= 1;

	printf("<TR CLASS='%s'>\n", bg_class);

	printf("<TD CLASS='%s'><A HREF='%s?type=hosts&expand=%s'>%s</A></TD>", bg_class, CONFIG_CGI, url_encode(temp_hd->dependent_host_name), html_encode(temp_hd->dependent_host_name, FALSE));

	printf("<TD CLASS='%s'><A HREF='%s?type=hosts&expand=%s'>%s</A></TD>", bg_class, CONFIG_CGI, url_encode(temp_hd->host_name), html_encode(temp_hd->host_name, FALSE));

	printf("<TD CLASS='%s'>%s</TD>", bg_class, (temp_hd->dependency_type == NOTIFICATION_DEPENDENCY) ? "通知" : "チェック実行");

	printf("<TD CLASS='%s'>", bg_class);
	if(temp_hd->dependency_period == NULL)
		printf("&nbsp;");
	else
		printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_hd->dependency_period), html_encode(temp_hd->dependency_period, FALSE));
	printf("</TD>\n");

	printf("<TD CLASS='%s'>", bg_class);
	options = FALSE;
	if(flag_isset(temp_hd->failure_options, OPT_UP) == TRUE) {
		printf("稼働(OK)");
		options = TRUE;
		}
	if(flag_isset(temp_hd->failure_options, OPT_DOWN) == TRUE) {
		printf("%s停止(DOWN)", (options == TRUE) ? ", " : "");
		options = TRUE;
		}
	if(flag_isset(temp_hd->failure_options, OPT_UNREACHABLE) == TRUE) {
		printf("%s未到達(UNREACHABLE)", (options == TRUE) ? ", " : "");
		options = TRUE;
		}
	if(flag_isset(temp_hd->failure_options, OPT_PENDING) == TRUE) {
		printf("%s未解決(PENDING)", (options == TRUE) ? ", " : "");
		options = TRUE;
		}
	printf("</TD>\n");

	printf("</TR>\n");
	}

void display_hostdependencies(void) {
	unsigned int i;

	/* see if user is authorized to view hostdependency information... */
	if(is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		unauthorized_message();
		return;
		}

	printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>%sホスト依存関係 %s</DIV></P>\n",
	       (*to_expand == '\0' ? "" : "次のホストを含む"), (*to_expand == '\0' ? "" : html_encode(to_expand, FALSE)));

	printf("<P>\n");
	printf("<DIV ALIGN=CENTER>\n");

	printf("<TABLE BORDER=0 CLASS='data'>\n");
	printf("<TR>\n");
	printf("<TH CLASS='data'>依存ホスト</TH>");
	printf("<TH CLASS='data'>上位ホスト</TH>");
	printf("<TH CLASS='data'>依存タイプ</TH>");
	printf("<TH CLASS='data'>依存期間</TH>");
	printf("<TH CLASS='data'>依存失敗オプション</TH>");
	printf("</TR>\n");

	/* print all host dependencies... */
	for(i = 0; i < num_objects.hostdependencies; i++)
		display_hostdependency(hostdependency_ary[i]);

	printf("</TABLE>\n");
	printf("</DIV>\n");
	printf("</P>\n");

	return;
	}



void display_hostescalations(void) {
	hostescalation *temp_he = NULL;
	contactsmember *temp_contactsmember = NULL;
	contactgroupsmember *temp_contactgroupsmember = NULL;
	char time_string[24] = "";
	int options = FALSE;
	int odd = 0;
	const char *bg_class = "";
	int num_contacts = 0;
	unsigned int i;

	/* see if user is authorized to view hostgroup information... */
	if(is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		unauthorized_message();
		return;
		}

	printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>%sホストエスカレーション %s</DIV></P>\n",
	       (*to_expand == '\0' ? "" : "次のホストの"), (*to_expand == '\0' ? "" : html_encode(to_expand, FALSE)));

	printf("<P>\n");
	printf("<DIV ALIGN=CENTER>\n");

	printf("<TABLE BORDER=0 CLASS='data'>\n");
	printf("<TR>\n");
	printf("<TH CLASS='data'>ホスト</TH>");
	printf("<TH CLASS='data'>通知先/グループ</TH>");
	printf("<TH CLASS='data'>初回通知</TH>");
	printf("<TH CLASS='data'>最終通知</TH>");
	printf("<TH CLASS='data'>通知間隔</TH>");
	printf("<TH CLASS='data'>エスカレーション期間</TH>");
	printf("<TH CLASS='data'>エスカレーションオプション</TH>");
	printf("</TR>\n");

	/* print all hostescalations... */
	for(i = 0; i < num_objects.hostescalations; i++) {
		temp_he = hostescalation_ary[i];
		if(*to_expand != '\0' && strcmp(to_expand, temp_he->host_name))
			continue;

		if(odd) {
			odd = 0;
			bg_class = "dataOdd";
			}
		else {
			odd = 1;
			bg_class = "dataEven";
			}

		printf("<TR CLASS='%s'>\n", bg_class);

		printf("<TD CLASS='%s'><A HREF='%s?type=hosts&expand=%s'>%s</A></TD>", bg_class, CONFIG_CGI, url_encode(temp_he->host_name), html_encode(temp_he->host_name, FALSE));

		printf("<TD CLASS='%s'>", bg_class);
		num_contacts = 0;
		for(temp_contactsmember = temp_he->contacts; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {
			num_contacts++;
			if(num_contacts > 1)
				printf(", ");
			printf("<A HREF='%s?type=contacts&expand=%s'>%s</A>\n", CONFIG_CGI, url_encode(temp_contactsmember->contact_name), html_encode(temp_contactsmember->contact_name, FALSE));
			}
		for(temp_contactgroupsmember = temp_he->contact_groups; temp_contactgroupsmember != NULL; temp_contactgroupsmember = temp_contactgroupsmember->next) {
			num_contacts++;
			if(num_contacts > 1)
				printf(", ");
			printf("<A HREF='%s?type=contactgroups&expand=%s'>%s</A>\n", CONFIG_CGI, url_encode(temp_contactgroupsmember->group_name), html_encode(temp_contactgroupsmember->group_name, FALSE));
			}
		if(num_contacts == 0)
			printf("&nbsp;");
		printf("</TD>\n");

		printf("<TD CLASS='%s'>%d</TD>", bg_class, temp_he->first_notification);

		printf("<TD CLASS='%s'>", bg_class);
		if(temp_he->last_notification == 0)
			printf("無限");
		else
			printf("%d", temp_he->last_notification);
		printf("</TD>\n");

		get_interval_time_string(temp_he->notification_interval, time_string, sizeof(time_string));
		printf("<TD CLASS='%s'>", bg_class);
		if(temp_he->notification_interval == 0.0)
			printf("1回のみ(再通知無し)");
		else
			printf("%s", time_string);
		printf("</TD>\n");

		printf("<TD CLASS='%s'>", bg_class);
		if(temp_he->escalation_period == NULL)
			printf("&nbsp;");
		else
			printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_he->escalation_period), html_encode(temp_he->escalation_period, FALSE));
		printf("</TD>\n");

		printf("<TD CLASS='%s'>", bg_class);
		options = FALSE;
		if(flag_isset(temp_he->escalation_options, OPT_DOWN) == TRUE) {
			printf("%s停止(DOWN)", (options == TRUE) ? ", " : "");
			options = TRUE;
			}
		if(flag_isset(temp_he->escalation_options, OPT_UNREACHABLE) == TRUE) {
			printf("%s未到達(UNREACHABLE)", (options == TRUE) ? ", " : "");
			options = TRUE;
			}
		if(flag_isset(temp_he->escalation_options, OPT_RECOVERY) == TRUE) {
			printf("%s復旧(RECOVERY)", (options == TRUE) ? ", " : "");
			options = TRUE;
			}
		if(options == FALSE)
			printf("無し");
		printf("</TD>\n");

		printf("</TR>\n");
		}

	printf("</TABLE>\n");
	printf("</DIV>\n");
	printf("</P>\n");

	return;
	}



void unauthorized_message(void) {

	printf("<P><DIV CLASS='errorMessage'>要求したコマンドを送信する権限が無いようです。</DIV></P>\n");
	printf("<P><DIV CLASS='errorDescription'>このメッセージが何らかのエラーであると思う場合はHTTPサーバのこのCGIに対するアクセス権限の設定かNagiosのCGI用設定ファイルの認証に関するオプションを調べてみてください</DIV></P>\n");

	return;
	}


static const char *hash_color(int i) {
	char c;

	/* This is actually optimized for MAX_COMMAND_ARGUMENTS==32 ... */

	if((i % 32) < 16) {
		if((i % 32) < 8) c = '7';
		else c = '4';
		}
	else {
		if((i % 32) < 24) c = '6';
		else c = '5';
		}

	/* Computation for standard case */
	hashed_color[0] = '#';
	hashed_color[1] = hashed_color[2] = ((i % 2) ? c : '0');
	hashed_color[3] = hashed_color[4] = (((i / 2) % 2) ? c : '0');
	hashed_color[5] = hashed_color[6] = (((i / 4) % 2) ? c : '0');
	hashed_color[7] = '\0';

	/* Override shades of grey */
	if((i % 8) == 7) hashed_color[1] = hashed_color[3] = '0';
	if((i % 8) == 0) hashed_color[2] = hashed_color[3] = hashed_color[4] = hashed_color[6] = c;

	return(hashed_color);
	}

void display_command_expansion(void) {
	command *temp_command;
	int odd = 0;
	const char *bg_class = "";
	int i, j;
	char *c, *cc;
	char commandline[MAX_COMMAND_BUFFER];
	char *command_args[MAX_COMMAND_ARGUMENTS];
	int arg_count[MAX_COMMAND_ARGUMENTS],
	    lead_space[MAX_COMMAND_ARGUMENTS],
	    trail_space[MAX_COMMAND_ARGUMENTS];

	/* see if user is authorized to view command information... */
	if(is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		unauthorized_message();
		return;
		}

	printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>拡張コマンド</DIV></P>\n");

	/* Parse to_expand into parts */
	for(i = 0; i < MAX_COMMAND_ARGUMENTS; i++) command_args[i] = NULL;
	for(i = 0, command_args[0] = cc = c = strdup(to_expand); c && ((*c) != '\0') && (i < MAX_COMMAND_ARGUMENTS); c++, cc++) {
		if((*c) == '\\') c++;
		else if((*c) == '!') {
			(*cc) = '\0';
			cc = c++;
			command_args[++i] = (c--);
			}
		(*cc) = (*c);
		}
	if((*c) == '\0')(*cc) = '\0';
	/* Precompute indexes of dangling whitespace */
	for(i = 0; i < MAX_COMMAND_ARGUMENTS; i++) {
		for(cc = command_args[i], lead_space[i] = 0; cc && isspace(*cc); cc++, lead_space[i]++) ;
		trail_space[i] = 0;
		for(; cc && ((*cc) != '\0'); cc++) if(isspace(*cc)) trail_space[i]++;
			else trail_space[i] = 0;
		}

	printf("<P><DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 CLASS='data'>\n");
	printf("<TR><TH CLASS='data'>Command Name</TH><TH CLASS='data'>コマンドライン</TH></TR>\n");

	if((*to_expand) != '\0') {
		arg_count[0] = 0;

		printf("<TR CLASS='dataEven'><TD CLASS='dataEven'>拡張:</TD><TD CLASS='dataEven'>%s", escape_string(command_args[0]));
		for(i = 1; (i < MAX_COMMAND_ARGUMENTS) && command_args[i]; i++)
			printf("!<FONT\n   COLOR='%s'>%s</FONT>", hash_color(i), escape_string(command_args[i]));
		printf("\n</TD></TR>\n");

		/* check all commands */
		for(temp_command = command_list; temp_command != NULL; temp_command = temp_command->next) {

			if(!strcmp(temp_command->name, command_args[0])) {

				arg_count[0]++;

				if(odd) {
					odd = 0;
					bg_class = "dataEven";
					}
				else {
					odd = 1;
					bg_class = "dataOdd";
					}

				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'><A NAME='%s'></A>%s</TD>\n", bg_class, url_encode(temp_command->name), html_encode(temp_command->name, FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_command->command_line, FALSE));

				printf("</TR>\n<TR CLASS='%s'>\n", bg_class);

				for(i = 1; i < MAX_COMMAND_ARGUMENTS; i++) arg_count[i] = 0;

				printf("<TD CLASS='%s' ALIGN='right'>-&gt;</TD>\n", bg_class);
				printf("<TD CLASS='%s'>", bg_class);
				strncpy(commandline, temp_command->command_line, MAX_COMMAND_BUFFER);
				commandline[MAX_COMMAND_BUFFER - 1] = '\0';
				for(c = commandline; c && (cc = strstr(c, "$"));) {
					(*(cc++)) = '\0';
					printf("%s", html_encode(c, FALSE));
					if((*cc) == '$') {
						/* Escaped '$' */
						printf("<FONT COLOR='#444444'>$</FONT>");
						c = (++cc);
						}
					else if(strncmp("ARG", cc, 3)) {
						/* Non-$ARGn$ macro */
						c = strstr(cc, "$");
						if(c)(*(c++)) = '\0';
						printf("<FONT COLOR='#777777'>$%s%s</FONT>", html_encode(cc, FALSE), (c ? "$" : ""));
						if(!c) printf("<FONT COLOR='#FF0000'> (適切に終端していません)</FONT>");
						}
					else {
						/* $ARGn$ macro */
						for(c = (cc += 3); isdigit(*c); c++) ;
						if(((*c) == '\0') || ((*c) == '$')) {
							/* Index is numeric */
							i = atoi(cc);
							if((i > 0) && (i <= MAX_COMMAND_ARGUMENTS)) {
								arg_count[i]++;
								if(command_args[i]) {
									if(*(command_args[i]) != '\0') printf("<FONT COLOR='%s'><B>%s%s%s</B></FONT>",
										                                      hash_color(i), ((lead_space[i] > 0) || (trail_space[i] > 0) ? "<U>&zwj;" : ""),
										                                      html_encode(command_args[i], FALSE), ((lead_space[i] > 0) || (trail_space[i] > 0) ? "&zwj;</U>" : ""));
									else printf("<FONT COLOR='#0000FF'>(空)</FONT>");
									}
								else printf("<FONT COLOR='#0000FF'>(未定義)</FONT>");
								}
							else printf("<FONT COLOR='#FF0000'>(有効な $ARGn$ インデックスではありません: %u)</FONT>", i);
							if((*c) != '\0') c++;
							else printf("<FONT COLOR='#FF0000'> (適切に終端していません)</FONT>");
							}
						else {
							/* Syntax err in index */
							c = strstr(cc, "$");
							printf("<FONT COLOR='#FF0000'>($ARGn$ インデックスではありません: &quot;%s&quot;)</FONT>", html_encode(strtok(cc, "$"), FALSE));
							if(c) c++;
							}
						}
					}
				if(c) printf("%s", html_encode(c, FALSE));

				printf("</TD></TR>\n");

				for(i = 1; (i < MAX_COMMAND_ARGUMENTS) && (command_args[i]); i++) {
					if(arg_count[i] == 0) {
						printf("<TR CLASS='%s'><TD CLASS='%s' ALIGN='right'><FONT COLOR='#FF0000'>未使用:</FONT></TD>\n", bg_class, bg_class);
						printf("<TD CLASS='%s'>$ARG%u$=<FONT COLOR='%s'>%s%s%s</FONT></TD></TR>\n", bg_class, i, hash_color(i),
						       ((lead_space[i] > 0) || (trail_space[i] > 0) ? "<U>&zwj;" : ""), html_encode(command_args[i], FALSE),
						       ((lead_space[i] > 0) || (trail_space[i] > 0) ? "&zwj;</U>" : ""));
						}
					else if(arg_count[i] > 1) {
						printf("<TR CLASS='%s'><TD CLASS='%s' ALIGN='right'>使用 %u x:</TD>\n", bg_class, bg_class, i);
						printf("<TD CLASS='%s'>$ARG%u$=<FONT COLOR='%s'>%s%s%s</FONT></TD></TR>\n", bg_class, i, hash_color(i),
						       ((lead_space[i] > 0) || (trail_space[i] > 0) ? "<U>&zwj;" : ""), html_encode(command_args[i], FALSE),
						       ((lead_space[i] > 0) || (trail_space[i] > 0) ? "&zwj;</U>" : ""));
						}
					if((lead_space[i] > 0) || (trail_space[i] > 0)) {
						printf("<TR CLASS='%s'><TD CLASS='%s' ALIGN='right'><FONT COLOR='#0000FF'>空白がぶらぶらしてます(dangling whitespace):</FONT></TD>\n", bg_class, bg_class);
						printf("<TD CLASS='%s'>$ARG%u$=<FONT COLOR='#0000FF'>", bg_class, i);
						for(c = command_args[i], j = 0; c && isspace(*c); c++, j++)
							/* TODO: As long as the hyperlinks change all whitespace into actual spaces,
							   we'll output "[WS]" (whitespace) instead of "[SP]"(ace). */
							/* if ((*c)==' ')		printf("[SP]"); */
							if((*c) == ' ')		printf("[WS]");
							else if((*c) == '\f')	printf("[FF]");
							else if((*c) == '\n')	printf("[LF]");
							else if((*c) == '\r')	printf("[CR]");
							else if((*c) == '\t')	printf("[HT]");
							else if((*c) == '\v')	printf("[VT]");
							else			printf("[0x%x]", *c);
						printf("</FONT><FONT COLOR='%s'>", hash_color(i));
						for(; c && ((*c) != '\0') && (j < (int)strlen(command_args[i]) - trail_space[i]); c++, j++) putchar(*c);
						printf("</FONT><FONT COLOR='#0000FF'>");
						for(; c && ((*c) != '\0'); c++)
							/* TODO: As long as the hyperlinks change all whitespace into actual spaces,
							   we'll output "[WS]" (whitespace) instead of "[SP]"(ace). */
							/* if ((*c)==' ')		printf("[SP]"); */
							if((*c) == ' ')		printf("[WS]");
							else if((*c) == '\f')	printf("[FF]");
							else if((*c) == '\n')	printf("[LF]");
							else if((*c) == '\r')	printf("[CR]");
							else if((*c) == '\t')	printf("[HT]");
							else if((*c) == '\v')	printf("[VT]");
							else			printf("[0x%x]", *c);
						printf("</FONT></TD></TR>\n");
						}
					}
				}

			}

		if(!arg_count[0]) {
			printf("<TR CLASS='dataOdd'><TD CLASS='dataOdd' ALIGN='right'><FONT\n");
			printf("COLOR='#FF0000'>エラー:</FONT></TD><TD CLASS='dataOdd'><FONT COLOR='#FF0000'>No\n");
			printf("コマンド &quot;%s&quot; が見つかりました</FONT></TD></TR>\n", html_encode(command_args[0], FALSE));
			}
		}

	printf("<TR CLASS='dataEven'><TD><BR/></TD><TD CLASS='dataEven'>ホストまたはサービスの定期からcommand_checkの定義を入力してコマンドの拡張を見に行くには実行を押します。</TD></TR>\n");
	printf("<TR CLASS='dataEven'><TD CLASS='dataEven'>拡張:</TD><TD CLASS='dataEven'><FORM\n");
	printf("METHOD='GET' ACTION='%s'><INPUT TYPE='HIDDEN' NAME='type' VALUE='command'><INPUT\n", CONFIG_CGI);
	printf("TYPE='text' NAME='expand' SIZE='100%%' VALUE='%s'>\n", html_encode(to_expand, FALSE));
	printf("<INPUT TYPE='SUBMIT' VALUE='実行'></FORM></TD></TR>\n");

	printf("</TABLE>\n");
	printf("</DIV></P>\n");

	return;
	}



void display_options(void) {

	printf("<br><br>\n");

	printf("<div align=center class='reportSelectTitle'>閲覧したい設定項目を選んでください</div>\n");

	printf("<br><br>\n");

	printf("<form method=\"get\" action=\"%s\">\n", CONFIG_CGI);

	printf("<div align=center>\n");
	printf("<table border=0>\n");

	printf("<tr><td align=left class='reportSelectSubTitle'>オブジェクトタイプ:</td></tr>\n");
	printf("<tr><td align=left class='reportSelectItem'>");
	printf("<select name='type'>\n");
	printf("<option value='hosts' %s>ホスト\n", (display_type == DISPLAY_HOSTS) ? "SELECTED" : "");
	printf("<option value='hostdependencies' %s>ホスト依存関係\n", (display_type == DISPLAY_HOSTDEPENDENCIES) ? "SELECTED" : "");
	printf("<option value='hostescalations' %s>ホストエスカレーション\n", (display_type == DISPLAY_HOSTESCALATIONS) ? "SELECTED" : "");
	printf("<option value='hostgroups' %s>ホストグループ\n", (display_type == DISPLAY_HOSTGROUPS) ? "SELECTED" : "");
	printf("<option value='services' %s>サービス\n", (display_type == DISPLAY_SERVICES) ? "SELECTED" : "");
	printf("<option value='servicegroups' %s>サービスグループ\n", (display_type == DISPLAY_SERVICEGROUPS) ? "SELECTED" : "");
	printf("<option value='servicedependencies' %s>サービス依存関係\n", (display_type == DISPLAY_SERVICEDEPENDENCIES) ? "SELECTED" : "");
	printf("<option value='serviceescalations' %s>サービスエスカレーション\n", (display_type == DISPLAY_SERVICEESCALATIONS) ? "SELECTED" : "");
	printf("<option value='contacts' %s>通知先\n", (display_type == DISPLAY_CONTACTS) ? "SELECTED" : "");
	printf("<option value='contactgroups' %s>通知先グループ\n", (display_type == DISPLAY_CONTACTGROUPS) ? "SELECTED" : "");
	printf("<option value='timeperiods' %s>時間帯\n", (display_type == DISPLAY_TIMEPERIODS) ? "SELECTED" : "");
	printf("<option value='commands' %s>コマンド\n", (display_type == DISPLAY_COMMANDS) ? "SELECTED" : "");
	printf("<option value='command' %s>拡張コマンド\n", (display_type == DISPLAY_COMMAND_EXPANSION) ? "SELECTED" : "");
	printf("</select>\n");
	printf("</td></tr>\n");

	printf("<tr><td class='reportSelectItem'><input type='submit' value='続行'></td></tr>\n");
	printf("</table>\n");
	printf("</div>\n");

	printf("</form>\n");

	return;
	}
