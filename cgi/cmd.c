/**************************************************************************
 *
 * CMD.C -  Nagios Command CGI
 *
 *
 * License:
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
 *************************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"
#include "../include/comments.h"
#include "../include/downtime.h"

#include "../include/cgiutils.h"
#include "../include/cgiauth.h"
#include "../include/getcgi.h"

extern char main_config_file[MAX_FILENAME_LENGTH];
extern char url_html_path[MAX_FILENAME_LENGTH];
extern char url_images_path[MAX_FILENAME_LENGTH];
extern char command_file[MAX_FILENAME_LENGTH];

extern char url_stylesheets_path[MAX_FILENAME_LENGTH];

extern int  nagios_process_state;

extern int  use_authentication;

extern int  lock_author_names;

extern int ack_no_sticky;
extern int ack_no_send;

#define MAX_AUTHOR_LENGTH	64
#define MAX_COMMENT_LENGTH	1024

#define HTML_CONTENT   0
#define WML_CONTENT    1


char *host_name = "";
char *hostgroup_name = "";
char *servicegroup_name = "";
char *service_desc = "";
char *comment_author = "";
char *comment_data = "";
char *start_time_string = "";
char *end_time_string = "";
char *cookie_form_id = NULL, *form_id = NULL;

unsigned long comment_id = 0;
unsigned long downtime_id = 0;
int notification_delay = 0;
int schedule_delay = 0;
int persistent_comment = FALSE;
int sticky_ack = FALSE;
int send_notification = FALSE;
int force_check = FALSE;
int plugin_state = STATE_OK;
char plugin_output[MAX_INPUT_BUFFER] = "";
char performance_data[MAX_INPUT_BUFFER] = "";
time_t start_time = 0L;
time_t end_time = 0L;
int affect_host_and_services = FALSE;
int propagate_to_children = FALSE;
int fixed = FALSE;
unsigned long duration = 0L;
unsigned long triggered_by = 0L;
int child_options = 0;
int force_notification = 0;
int broadcast_notification = 0;

int command_type = CMD_NONE;
int command_mode = CMDMODE_REQUEST;

int content_type = HTML_CONTENT;

int display_header = TRUE;

authdata current_authdata;

void show_command_help(int);
void request_command_data(int);
void commit_command_data(int);
int print_comment_field(int cmd_id);
int commit_command(int);
int write_command_to_file(char *);
void clean_comment_data(char *);

void cgicfg_callback(const char*, const char*);
void document_header(int);
void document_footer(void);
int process_cgivars(void);

int string_to_time(char *, time_t *);



int main(void) {
	int result = OK;
	int formid_ok = OK;

	/* Initialize shared configuration variables */
	init_shared_cfg_vars(1);

	/* get the arguments passed in the URL */
	process_cgivars();

	/* reset internal variables */
	reset_cgi_vars();

	/* read the CGI configuration file */
	result = read_cgi_config_file(get_cgi_config_location(), cgicfg_callback);
	if(result == ERROR) {
		document_header(FALSE);
		if(content_type == WML_CONTENT)
			printf("<p>エラー: CGI設定ファイルが開けませんでした。</p>\n");
		else
			cgi_config_file_error(get_cgi_config_location());
		document_footer();
		return ERROR;
		}

	/* read the main configuration file */
	result = read_main_config_file(main_config_file);
	if(result == ERROR) {
		document_header(FALSE);
		if(content_type == WML_CONTENT)
			printf("<p>エラー: メインの設定ファイルが開けませんでした。</p>\n");
		else
			main_config_file_error(main_config_file);
		document_footer();
		return ERROR;
		}

	/* This requires the date_format parameter in the main config file */
	if(strcmp(start_time_string, ""))
		string_to_time(start_time_string, &start_time);

	if(strcmp(end_time_string, ""))
		string_to_time(end_time_string, &end_time);


	/* read all object configuration data */
	result = read_all_object_configuration_data(main_config_file, READ_ALL_OBJECT_DATA);
	if(result == ERROR) {
		document_header(FALSE);
		if(content_type == WML_CONTENT)
			printf("<p>エラー: オブジェクトデータが読み取り出来ませんでした。</p>\n");
		else
			object_data_error();
		document_footer();
		return ERROR;
		}

	document_header(TRUE);

	/* get authentication information */
	get_authentication_information(&current_authdata);

	if(display_header == TRUE) {

		/* begin top table */
		printf("<table border=0 width=100%%>\n");
		printf("<tr>\n");

		/* left column of the first row */
		printf("<td align=left valign=top width=33%%>\n");
		display_info_table("External Command Interface", FALSE, &current_authdata);
		printf("</td>\n");

		/* center column of the first row */
		printf("<td align=center valign=top width=33%%>\n");
		printf("</td>\n");

		/* right column of the first row */
		printf("<td align=right valign=bottom width=33%%>\n");

		/* display context-sensitive help */
		if(command_mode == CMDMODE_COMMIT)
			display_context_help(CONTEXTHELP_CMD_COMMIT);
		else
			display_context_help(CONTEXTHELP_CMD_INPUT);

		printf("</td>\n");

		/* end of top table */
		printf("</tr>\n");
		printf("</table>\n");
		}

	/* authorized_for_read_only should take priority */
	if(is_authorized_for_read_only(&current_authdata) == TRUE) {
		printf("<P><DIV CLASS='errorMessage'>要求したコマンドを送信する権限が無いようです。</DIV></P>\n");
		printf("<P><DIV CLASS='errorDescription'>このメッセージが何らかのエラーであると思う場合はHTTPサーバのこのCGIに対するアクセス権限の設定かNagiosのCGI用設定ファイルの認証に関するオプションを調べてみてください</DIV></P>\n");

		document_footer();

		/* free allocated memory */
		free_memory();
		free_object_data();

		return OK;
        }

	if (cookie_form_id && *cookie_form_id) {
		formid_ok = ERROR;
		if (form_id && *form_id) {
			if (!strcmp(form_id, cookie_form_id))
				formid_ok = OK;
		}
	}

	/* if no command was specified... */
	if(command_type == CMD_NONE) {
		if(content_type == WML_CONTENT)
			printf("<p>エラー: コマンドが選択されていません。</p>\n");
		else
			printf("<P><DIV CLASS='errorMessage'>エラー: コマンドが選択されていません。</DIV></P>\n");
		}

	/* if this is the first request for a command, present option */
	else if(command_mode == CMDMODE_REQUEST)
		request_command_data(command_type);

	/* the user wants to commit the command */
	else if(command_mode == CMDMODE_COMMIT) {
		if (formid_ok == ERROR) /* we're expecting an id but it wasn't there... */
			printf("<p>Error: Invalid form id!</p>\n");
		else
			commit_command_data(command_type);
	}

	document_footer();

	/* free allocated memory */
	free_memory();
	free_object_data();

	return OK;
	}

void cgicfg_callback(const char *var, const char *val)
{
	struct nagios_extcmd	*ecmd;
	const char				*cp = val;

	if (strncmp(var, "CMT_", 4))
		return;

	ecmd = extcmd_get_command_name(&var[4]);
	if (!ecmd)
		return;

	if (!isdigit(*val))
		return;

	ecmd->cmt_opt = atoi(val);
	while (isdigit(*cp) || *cp == ',' || *cp == ' ' || *cp == '\t')
		++cp;
	if (!*cp)
		return;
	ecmd->default_comment = strdup(cp);
	strip_html_brackets(ecmd->default_comment);
}


void document_header(int use_stylesheet) {

	if(content_type == WML_CONTENT) {

		printf("Content-type: text/vnd.wap.wml\r\n\r\n");

		printf("<?xml version=\"1.0\"?>\n");
		printf("<!DOCTYPE wml PUBLIC \"-//WAPFORUM//DTD WML 1.1//EN\" \"http://www.wapforum.org/DTD/wml_1.1.xml\">\n");

		printf("<wml>\n");

		printf("<card id='card1' title='コマンド結果'>\n");
		}

	else {

		printf("Content-type: text/html; charset=utf-8\r\n\r\n");

		printf("<html>\n");
		printf("<head>\n");
		printf("<link rel=\"shortcut icon\" href=\"%sfavicon.ico\" type=\"image/ico\">\n", url_images_path);
		printf("<title>\n");
		printf("外部コマンドインターフェース\n");
		printf("</title>\n");

		if(use_stylesheet == TRUE) {
			printf("<LINK REL='stylesheet' TYPE='text/css' HREF='%s%s'>\n", url_stylesheets_path, COMMON_CSS);
			printf("<LINK REL='stylesheet' TYPE='text/css' HREF='%s%s'>\n", url_stylesheets_path, COMMAND_CSS);
			}

		printf("</head>\n");

		printf("<body CLASS='cmd'>\n");

		/* include user SSI header */
		include_ssi_files(COMMAND_CGI, SSI_HEADER);
		}

	return;
	}


void document_footer(void) {

	if(content_type == WML_CONTENT) {
		printf("</card>\n");
		printf("</wml>\n");
		}

	else {

		/* include user SSI footer */
		include_ssi_files(COMMAND_CGI, SSI_FOOTER);

		printf("</body>\n");
		printf("</html>\n");
		}

	return;
	}


int process_cgivars(void) {
	char **variables;
	int error = FALSE;
	int x;

	variables = getcgivars();

	for(x = 0; variables[x]; x++) {

		/* do some basic length checking on the variable identifier to prevent buffer overflows */
		if(strlen(variables[x]) >= MAX_INPUT_BUFFER - 1) {
			continue;
			}

		/* we found the command type */
		else if(!strcmp(variables[x], "cmd_typ")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			command_type = atoi(variables[x]);
			}

		/* we found the command mode */
		else if(!strcmp(variables[x], "cmd_mod")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			command_mode = atoi(variables[x]);
			}

		/* we found the comment id */
		else if(!strcmp(variables[x], "com_id")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			comment_id = strtoul(variables[x], NULL, 10);
			}

		/* we found the downtime id */
		else if(!strcmp(variables[x], "down_id")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			downtime_id = strtoul(variables[x], NULL, 10);
			}

		/* we found the notification delay */
		else if(!strcmp(variables[x], "not_dly")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			notification_delay = atoi(variables[x]);
			}

		/* we found the schedule delay */
		else if(!strcmp(variables[x], "sched_dly")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			schedule_delay = atoi(variables[x]);
			}

		/* we found the comment author */
		else if(!strcmp(variables[x], "com_author")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			if((comment_author = (char *)strdup(variables[x])) == NULL)
				comment_author = "";
			strip_html_brackets(comment_author);
			}

		/* we found the comment data */
		else if(!strcmp(variables[x], "com_data")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			if((comment_data = (char *)strdup(variables[x])) == NULL)
				comment_data = "";
			strip_html_brackets(comment_data);
			}

		/* we found the host name */
		else if(!strcmp(variables[x], "host")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			if((host_name = (char *)strdup(variables[x])) == NULL)
				host_name = "";
			strip_html_brackets(host_name);
			}

		/* we found the hostgroup name */
		else if(!strcmp(variables[x], "hostgroup")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			if((hostgroup_name = (char *)strdup(variables[x])) == NULL)
				hostgroup_name = "";
			strip_html_brackets(hostgroup_name);
			}

		/* we found the service name */
		else if(!strcmp(variables[x], "service")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			if((service_desc = (char *)strdup(variables[x])) == NULL)
				service_desc = "";
			strip_html_brackets(service_desc);
			}

		/* we found the servicegroup name */
		else if(!strcmp(variables[x], "servicegroup")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			if((servicegroup_name = (char *)strdup(variables[x])) == NULL)
				servicegroup_name = "";
			strip_html_brackets(servicegroup_name);
			}

		/* we got the persistence option for a comment */
		else if(!strcmp(variables[x], "persistent"))
			persistent_comment = TRUE;

		/* we got the notification option for an acknowledgement */
		else if(!strcmp(variables[x], "send_notification"))
			send_notification = TRUE;

		/* we got the acknowledgement type */
		else if(!strcmp(variables[x], "sticky_ack"))
			sticky_ack = TRUE;

		/* we got the service check force option */
		else if(!strcmp(variables[x], "force_check"))
			force_check = TRUE;

		/* we got the option to affect host and all its services */
		else if(!strcmp(variables[x], "ahas"))
			affect_host_and_services = TRUE;

		/* we got the option to propagate to child hosts */
		else if(!strcmp(variables[x], "ptc"))
			propagate_to_children = TRUE;

		/* we got the option for fixed downtime */
		else if(!strcmp(variables[x], "fixed")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			fixed = (atoi(variables[x]) > 0) ? TRUE : FALSE;
			}

		/* we got the triggered by downtime option */
		else if(!strcmp(variables[x], "trigger")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			triggered_by = strtoul(variables[x], NULL, 10);
			}

		/* we got the child options */
		else if(!strcmp(variables[x], "childoptions")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			child_options = atoi(variables[x]);
			}

		/* we found the plugin output */
		else if(!strcmp(variables[x], "plugin_output")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			/* protect against buffer overflows */
			if(strlen(variables[x]) >= MAX_INPUT_BUFFER - 1) {
				error = TRUE;
				break;
				}
			else
				strcpy(plugin_output, variables[x]);
			}

		/* we found the performance data */
		else if(!strcmp(variables[x], "performance_data")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			/* protect against buffer overflows */
			if(strlen(variables[x]) >= MAX_INPUT_BUFFER - 1) {
				error = TRUE;
				break;
				}
			else
				strcpy(performance_data, variables[x]);
			}

		/* we found the plugin state */
		else if(!strcmp(variables[x], "plugin_state")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			plugin_state = atoi(variables[x]);
			}

		/* we found the hour duration */
		else if(!strcmp(variables[x], "hours")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			if(atoi(variables[x]) < 0) {
				error = TRUE;
				break;
				}
			duration += (unsigned long)(atoi(variables[x]) * 3600);
			}

		/* we found the minute duration */
		else if(!strcmp(variables[x], "minutes")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			if(atoi(variables[x]) < 0) {
				error = TRUE;
				break;
				}
			duration += (unsigned long)(atoi(variables[x]) * 60);
			}

		/* we found the start time */
		else if(!strcmp(variables[x], "start_time")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			start_time_string = (char *)malloc(strlen(variables[x]) + 1);
			if(start_time_string == NULL)
				start_time_string = "";
			else
				strcpy(start_time_string, variables[x]);
			}

		/* we found the end time */
		else if(!strcmp(variables[x], "end_time")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			end_time_string = (char *)malloc(strlen(variables[x]) + 1);
			if(end_time_string == NULL)
				end_time_string = "";
			else
				strcpy(end_time_string, variables[x]);
			}

		/* we found the content type argument */
		else if(!strcmp(variables[x], "content")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}
			if(!strcmp(variables[x], "wml")) {
				content_type = WML_CONTENT;
				display_header = FALSE;
				}
			else
				content_type = HTML_CONTENT;
			}

		/* we found the forced notification option */
		else if(!strcmp(variables[x], "force_notification"))
			force_notification = NOTIFICATION_OPTION_FORCED;

		/* we found the broadcast notification option */
		else if(!strcmp(variables[x], "broadcast_notification"))
			broadcast_notification = NOTIFICATION_OPTION_BROADCAST;

		/* we found the cookie form id */
		else if (!strcmp(variables[x], "NagFormId")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			cookie_form_id = (char*)strdup(variables[x]);
		}

		/* we found the form id on the form */
		else if (!strcmp(variables[x], "nagFormId")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			form_id = (char*)strdup(variables[x]);
		}

	}

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
	}



void request_command_data(int cmd) {
	time_t t;
	char start_time_str[MAX_DATETIME_LENGTH];
	char buffer[MAX_INPUT_BUFFER];
	contact *temp_contact;
	scheduled_downtime *temp_downtime;


	/* get default name to use for comment author */
	temp_contact = find_contact(current_authdata.username);
	if(temp_contact != NULL && temp_contact->alias != NULL)
		comment_author = temp_contact->alias;
	else
		comment_author = current_authdata.username;


	printf("<P><DIV ALIGN=CENTER CLASS='cmdType'>リクエストしたコマンド:");

	switch(cmd) {

		case CMD_ADD_HOST_COMMENT:
		case CMD_ADD_SVC_COMMENT:
			printf("%sにコメントを追加する", (cmd == CMD_ADD_HOST_COMMENT) ? "ホスト" : "サービス");
			break;

		case CMD_DEL_HOST_COMMENT:
		case CMD_DEL_SVC_COMMENT:
			printf("%sのコメントを削除する", (cmd == CMD_DEL_HOST_COMMENT) ? "ホスト" : "サービス");
			break;

		case CMD_DELAY_HOST_NOTIFICATION:
		case CMD_DELAY_SVC_NOTIFICATION:
			printf("%sの通知を遅らせる", (cmd == CMD_DELAY_HOST_NOTIFICATION) ? "ホスト" : "サービス");
			break;

		case CMD_SCHEDULE_SVC_CHECK:
			printf("次回のサービスチェックを再スケジュール");
			break;

		case CMD_ENABLE_SVC_CHECK:
		case CMD_DISABLE_SVC_CHECK:
			printf("指定したサービスチェックを%sにする", (cmd == CMD_ENABLE_SVC_CHECK) ? "有効" : "無効");
			break;

		case CMD_ENABLE_NOTIFICATIONS:
		case CMD_DISABLE_NOTIFICATIONS:
			printf("通知を%sにする", (cmd == CMD_ENABLE_NOTIFICATIONS) ? "有効" : "無効");
			break;

		case CMD_SHUTDOWN_PROCESS:
		case CMD_RESTART_PROCESS:
			printf("Nagiosの%s", (cmd == CMD_SHUTDOWN_PROCESS) ? "停止" : "再起動");
			break;

		case CMD_ENABLE_HOST_SVC_CHECKS:
		case CMD_DISABLE_HOST_SVC_CHECKS:
			printf("ホストの全サービスを%sにする", (cmd == CMD_ENABLE_HOST_SVC_CHECKS) ? "有効" : "無効");
			break;

		case CMD_SCHEDULE_HOST_SVC_CHECKS:
			printf("ホストの全てのサービスチェックを遅らせる");
			break;

		case CMD_DEL_ALL_HOST_COMMENTS:
		case CMD_DEL_ALL_SVC_COMMENTS:
			printf("%sの全てのコメントを削除する", (cmd == CMD_DEL_ALL_HOST_COMMENTS) ? "ホスト" : "サービス");
			break;

		case CMD_ENABLE_SVC_NOTIFICATIONS:
		case CMD_DISABLE_SVC_NOTIFICATIONS:
			printf("サービスの通知を%sにする", (cmd == CMD_ENABLE_SVC_NOTIFICATIONS) ? "有効" : "無効");
			break;

		case CMD_ENABLE_HOST_NOTIFICATIONS:
		case CMD_DISABLE_HOST_NOTIFICATIONS:
			printf("ホストの通知を%sにする", (cmd == CMD_ENABLE_HOST_NOTIFICATIONS) ? "有効" : "無効");
			break;

		case CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
		case CMD_DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
			printf("ホスト配下の全ホストとサービスの通知を%sにする", (cmd == CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST) ? "有効" : "無効");
			break;

		case CMD_ENABLE_HOST_SVC_NOTIFICATIONS:
		case CMD_DISABLE_HOST_SVC_NOTIFICATIONS:
			printf("ホスト上の全サービスの通知を%sにする", (cmd == CMD_ENABLE_HOST_SVC_NOTIFICATIONS) ? "有効" : "無効");
			break;

		case CMD_ACKNOWLEDGE_HOST_PROBLEM:
		case CMD_ACKNOWLEDGE_SVC_PROBLEM:
			printf("%sの問題を既知にする", (cmd == CMD_ACKNOWLEDGE_HOST_PROBLEM) ? "ホスト" : "サービス");
			break;

		case CMD_START_EXECUTING_SVC_CHECKS:
		case CMD_STOP_EXECUTING_SVC_CHECKS:
			printf("サービスチェックを%sする", (cmd == CMD_START_EXECUTING_SVC_CHECKS) ? "開始" : "停止");
			break;

		case CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS:
		case CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS:
			printf("パッシブサービスチェックを%sする", (cmd == CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS) ? "開始" : "停止");
			break;

		case CMD_ENABLE_PASSIVE_SVC_CHECKS:
		case CMD_DISABLE_PASSIVE_SVC_CHECKS:
			printf("指定したサービスのパッシブチェックを%sする", (cmd == CMD_ENABLE_PASSIVE_SVC_CHECKS) ? "開始" : "停止");
			break;

		case CMD_ENABLE_EVENT_HANDLERS:
		case CMD_DISABLE_EVENT_HANDLERS:
			printf("イベントハンドラを%sにする", (cmd == CMD_ENABLE_EVENT_HANDLERS) ? "有効" : "無効");
			break;

		case CMD_ENABLE_HOST_EVENT_HANDLER:
		case CMD_DISABLE_HOST_EVENT_HANDLER:
			printf("指定したホストのイベントハンドラを%sにする", (cmd == CMD_ENABLE_HOST_EVENT_HANDLER) ? "有効" : "無効");
			break;

		case CMD_ENABLE_SVC_EVENT_HANDLER:
		case CMD_DISABLE_SVC_EVENT_HANDLER:
			printf("指定したサービスのイベントハンドラを%sにする", (cmd == CMD_ENABLE_SVC_EVENT_HANDLER) ? "有効" : "無効");
			break;

		case CMD_ENABLE_HOST_CHECK:
		case CMD_DISABLE_HOST_CHECK:
			printf("指定したホストのアクティブチェックを%sにする", (cmd == CMD_ENABLE_HOST_CHECK) ? "有効" : "無効");
			break;

		case CMD_STOP_OBSESSING_OVER_SVC_CHECKS:
		case CMD_START_OBSESSING_OVER_SVC_CHECKS:
			printf("Obsessing Overサービスチェックを%sする", (cmd == CMD_STOP_OBSESSING_OVER_SVC_CHECKS) ? "停止" : "開始");
			break;

		case CMD_REMOVE_HOST_ACKNOWLEDGEMENT:
		case CMD_REMOVE_SVC_ACKNOWLEDGEMENT:
			printf("%sの既知の問題を削除する", (cmd == CMD_REMOVE_HOST_ACKNOWLEDGEMENT) ? "ホスト" : "サービス");
			break;

		case CMD_SCHEDULE_HOST_DOWNTIME:
		case CMD_SCHEDULE_SVC_DOWNTIME:
			printf("%sのダウンタイムをスケジュールに追加する", (cmd == CMD_SCHEDULE_HOST_DOWNTIME) ? "ホスト" : "サービス");
			break;

		case CMD_SCHEDULE_HOST_SVC_DOWNTIME:
			printf("ホストの全サービスのダウンタイムをスケジュールに追加する");
			break;

		case CMD_PROCESS_HOST_CHECK_RESULT:
		case CMD_PROCESS_SERVICE_CHECK_RESULT:
			printf("指定した%sのパッシブチェックの結果を登録する", (cmd == CMD_PROCESS_HOST_CHECK_RESULT) ? "ホスト" : "サービス");
			break;

		case CMD_ENABLE_HOST_FLAP_DETECTION:
		case CMD_DISABLE_HOST_FLAP_DETECTION:
			printf("ホストのフラップ検知を%sにする", (cmd == CMD_ENABLE_HOST_FLAP_DETECTION) ? "有効" : "無効");
			break;

		case CMD_ENABLE_SVC_FLAP_DETECTION:
		case CMD_DISABLE_SVC_FLAP_DETECTION:
			printf("サービスのフラップ検知を%sにする", (cmd == CMD_ENABLE_SVC_FLAP_DETECTION) ? "有効" : "無効");
			break;

		case CMD_ENABLE_FLAP_DETECTION:
		case CMD_DISABLE_FLAP_DETECTION:
			printf("ホストとサービスのフラップ検知を%sにする", (cmd == CMD_ENABLE_FLAP_DETECTION) ? "有効" : "無効");
			break;

		case CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS:
		case CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS:
			printf("指定したホストグループ上の全サービスの通知を%sにする", (cmd == CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS) ? "有効" : "無効");
			break;

		case CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS:
		case CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS:
			printf("指定したホストグループ上の全ホストの通知を%sにする", (cmd == CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS) ? "有効" : "無効");
			break;

		case CMD_ENABLE_HOSTGROUP_SVC_CHECKS:
		case CMD_DISABLE_HOSTGROUP_SVC_CHECKS:
			printf("指定したホストグループ上の全てのサービスチェックを%sにする", (cmd == CMD_ENABLE_HOSTGROUP_SVC_CHECKS) ? "有効" : "無効");
			break;

		case CMD_DEL_HOST_DOWNTIME:
		case CMD_DEL_SVC_DOWNTIME:
			printf("%sのダウンタイムをキャンセルする", (cmd == CMD_DEL_HOST_DOWNTIME) ? "ホスト" : "サービス");
			break;

		case CMD_ENABLE_PERFORMANCE_DATA:
		case CMD_DISABLE_PERFORMANCE_DATA:
			printf("ホストとサービスのパフォーマンスデータを%sにする", (cmd == CMD_ENABLE_PERFORMANCE_DATA) ? "有効" : "無効");
			break;

		case CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME:
			printf("指定したホストグループ上のホストのダウンタイムをスケジュールに追加する");
			break;

		case CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME:
			printf("指定したホストグループ上のサービスのダウンタイムをスケジュールに追加する");
			break;

		case CMD_START_EXECUTING_HOST_CHECKS:
		case CMD_STOP_EXECUTING_HOST_CHECKS:
			printf("ホストチェックを%sする", (cmd == CMD_START_EXECUTING_HOST_CHECKS) ? "開始" : "停止");
			break;

		case CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS:
		case CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS:
			printf("パッシブホストのチェックを%sする", (cmd == CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS) ? "開始" : "停止");
			break;

		case CMD_ENABLE_PASSIVE_HOST_CHECKS:
		case CMD_DISABLE_PASSIVE_HOST_CHECKS:
			printf("指定したホストのパッシブチェックを%sする", (cmd == CMD_ENABLE_PASSIVE_HOST_CHECKS) ? "開始" : "停止");
			break;

		case CMD_START_OBSESSING_OVER_HOST_CHECKS:
		case CMD_STOP_OBSESSING_OVER_HOST_CHECKS:
			printf("Obsessing Overホストのチェックを%sする", (cmd == CMD_START_OBSESSING_OVER_HOST_CHECKS) ? "開始" : "停止");
			break;

		case CMD_SCHEDULE_HOST_CHECK:
			printf("ホストチェックをスケジュールに追加する");
			break;

		case CMD_START_OBSESSING_OVER_SVC:
		case CMD_STOP_OBSESSING_OVER_SVC:
			printf("指定したサービスのObsessing Overを%sする", (cmd == CMD_START_OBSESSING_OVER_SVC) ? "開始" : "停止");
			break;

		case CMD_START_OBSESSING_OVER_HOST:
		case CMD_STOP_OBSESSING_OVER_HOST:
			printf("指定したホストのObsessing Overを%sする", (cmd == CMD_START_OBSESSING_OVER_HOST) ? "開始" : "停止");
			break;

		case CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
		case CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
			printf("指定したサービスグループ上の全サービスの通知を%sにする", (cmd == CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS) ? "有効" : "無効");
			break;

		case CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
		case CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
			printf("指定したサービスグループ上の全ホストの通知を%sにする", (cmd == CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS) ? "有効" : "無効");
			break;

		case CMD_ENABLE_SERVICEGROUP_SVC_CHECKS:
		case CMD_DISABLE_SERVICEGROUP_SVC_CHECKS:
			printf("指定したサービスグループ上の全てのサービスチェックを%sにする", (cmd == CMD_ENABLE_SERVICEGROUP_SVC_CHECKS) ? "有効" : "無効");
			break;

		case CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME:
			printf("指定したサービスグループ上のホストのダウンタイムをスケジュールに追加する");
			break;

		case CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME:
			printf("指定したサービスグループ上のサービスのダウンタイムをスケジュールに追加する");
			break;

		case CMD_CLEAR_HOST_FLAPPING_STATE:
		case CMD_CLEAR_SVC_FLAPPING_STATE:
			printf("%sのフラッピング状態をクリアする", (cmd == CMD_CLEAR_HOST_FLAPPING_STATE) ? "ホスト" : "サービス");
			break;

		case CMD_SEND_CUSTOM_HOST_NOTIFICATION:
		case CMD_SEND_CUSTOM_SVC_NOTIFICATION:
			printf("カスタム%s通知を送信する", (cmd == CMD_SEND_CUSTOM_HOST_NOTIFICATION) ? "ホスト" : "サービス");
			break;

		default:
			printf("不明なコマンドを実行しました</DIV>");
			return;
		}

	printf("</DIV></p>\n");

	printf("<p>\n");
	printf("<div align='center'>\n");

	printf("<table border=0 width=90%%>\n");
	printf("<tr>\n");
	printf("<td align=center valign=top>\n");

	printf("<DIV ALIGN=CENTER CLASS='optBoxTitle'>コマンドオプション</DIV>\n");

	printf("<TABLE CELLSPACING=0 CELLPADDING=0 BORDER=1 CLASS='optBox'>\n");
	printf("<TR><TD CLASS='optBoxItem'>\n");
	printf("<form method='post' action='%s'>\n", COMMAND_CGI);
	if (cookie_form_id && *cookie_form_id)
		printf("<INPUT TYPE='hidden' NAME='nagFormId' VALUE='%s'\n", cookie_form_id);
	printf("<TABLE CELLSPACING=0 CELLPADDING=0 CLASS='optBox'>\n");

	printf("<tr><td><INPUT TYPE='HIDDEN' NAME='cmd_typ' VALUE='%d'><INPUT TYPE='HIDDEN' NAME='cmd_mod' VALUE='%d'></td></tr>\n", cmd, CMDMODE_COMMIT);

	switch(cmd) {

		case CMD_ADD_HOST_COMMENT:
		case CMD_ACKNOWLEDGE_HOST_PROBLEM:
			printf("<tr><td CLASS='optBoxRequiredItem'>ホスト名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='host' VALUE='%s'>", escape_string(host_name));
			printf("</b></td></tr>\n");
			if(cmd == CMD_ACKNOWLEDGE_HOST_PROBLEM) {
				printf("<tr><td CLASS='optBoxItem'>既知の問題を固定:</td><td><b>");
				printf("<INPUT TYPE='checkbox' NAME='sticky_ack' %s>", (ack_no_sticky == TRUE) ? "" : "CHECKED");
				printf("</b></td></tr>\n");
				printf("<tr><td CLASS='optBoxItem'>警報を通知:</td><td><b>");
				printf("<INPUT TYPE='checkbox' NAME='send_notification' %s>", (ack_no_send == TRUE) ? "" : "CHECKED");
				printf("</b></td></tr>\n");
				}
			printf("<tr><td CLASS='optBoxItem'>再起動後も%s保持させる:</td><td><b>", (cmd == CMD_ACKNOWLEDGE_HOST_PROBLEM) ? "コメントを" : "");
			printf("<INPUT TYPE='checkbox' NAME='persistent' %s>", (cmd == CMD_ACKNOWLEDGE_HOST_PROBLEM) ? "" : "CHECKED");
			printf("</b></td></tr>\n");
			print_comment_field(cmd);
			break;

		case CMD_ADD_SVC_COMMENT:
		case CMD_ACKNOWLEDGE_SVC_PROBLEM:
			printf("<tr><td CLASS='optBoxRequiredItem'>ホスト名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='host' VALUE='%s'>", escape_string(host_name));
			printf("</b></td></tr>\n");
			printf("<tr><td CLASS='optBoxRequiredItem'>サービス名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='service' VALUE='%s'>", escape_string(service_desc));
			if(cmd == CMD_ACKNOWLEDGE_SVC_PROBLEM) {
				printf("<tr><td CLASS='optBoxItem'>既知の問題を固定:</td><td><b>");
				printf("<INPUT TYPE='checkbox' NAME='sticky_ack' %s>", (ack_no_sticky == TRUE) ? "" : "CHECKED");
				printf("</b></td></tr>\n");
				printf("<tr><td CLASS='optBoxItem'>警報を通知:</td><td><b>");
				printf("<INPUT TYPE='checkbox' NAME='send_notification' %s>", (ack_no_send == TRUE) ? "" : "CHECKED");
				printf("</b></td></tr>\n");
				}
			printf("<tr><td CLASS='optBoxItem'>再起動後も%s保持させる:</td><td><b>", (cmd == CMD_ACKNOWLEDGE_SVC_PROBLEM) ? "コメントを" : "");
			printf("<INPUT TYPE='checkbox' NAME='persistent' %s>", (cmd == CMD_ACKNOWLEDGE_SVC_PROBLEM) ? "" : "CHECKED");
			printf("</b></td></tr>\n");
			print_comment_field(cmd);
			break;

		case CMD_DEL_HOST_COMMENT:
		case CMD_DEL_SVC_COMMENT:
			printf("<tr><td CLASS='optBoxRequiredItem'>コメント ID:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='com_id' VALUE='%lu'>", comment_id);
			printf("</b></td></tr>\n");
			break;

		case CMD_DELAY_HOST_NOTIFICATION:
			printf("<tr><td CLASS='optBoxRequiredItem'>ホスト名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='host' VALUE='%s'>", escape_string(host_name));
			printf("</b></td></tr>\n");
			printf("<tr><td CLASS='optBoxRequiredItem'>通知を遅らせる時間(現在からの分):</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='not_dly' VALUE='%d'>", notification_delay);
			printf("</b></td></tr>\n");
			print_comment_field(cmd);
			break;

		case CMD_DELAY_SVC_NOTIFICATION:
			printf("<tr><td CLASS='optBoxRequiredItem'>ホスト名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='host' VALUE='%s'>", escape_string(host_name));
			printf("</b></td></tr>\n");
			printf("<tr><td CLASS='optBoxRequiredItem'>サービス名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='service' VALUE='%s'>", escape_string(service_desc));
			printf("<tr><td CLASS='optBoxRequiredItem'>通知を遅らせる時間(現在からの分):</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='not_dly' VALUE='%d'>", notification_delay);
			printf("</b></td></tr>\n");
			print_comment_field(cmd);
			break;

		case CMD_SCHEDULE_SVC_CHECK:
		case CMD_SCHEDULE_HOST_CHECK:
		case CMD_SCHEDULE_HOST_SVC_CHECKS:
			printf("<tr><td CLASS='optBoxRequiredItem'>ホスト名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='host' VALUE='%s'>", escape_string(host_name));
			printf("</b></td></tr>\n");
			if(cmd == CMD_SCHEDULE_SVC_CHECK) {
				printf("<tr><td CLASS='optBoxRequiredItem'>サービス名:</td><td><b>");
				printf("<INPUT TYPE='TEXT' NAME='service' VALUE='%s'>", escape_string(service_desc));
				printf("</b></td></tr>\n");
				}
			time(&t);
			get_time_string(&t, buffer, sizeof(buffer) - 1, SHORT_DATE_TIME);
			printf("<tr><td CLASS='optBoxRequiredItem'>チェック時刻:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='start_time' VALUE='%s'>", buffer);
			printf("</b></td></tr>\n");
			print_comment_field(cmd);
			printf("<tr><td CLASS='optBoxItem'>強制的にチェック:</td><td><b>");
			printf("<INPUT TYPE='checkbox' NAME='force_check' %s>", (force_check == TRUE) ? "CHECKED" : "");
			printf("</b></td></tr>\n");
			break;

		case CMD_ENABLE_SVC_CHECK:
		case CMD_DISABLE_SVC_CHECK:
		case CMD_DEL_ALL_SVC_COMMENTS:
		case CMD_ENABLE_SVC_NOTIFICATIONS:
		case CMD_DISABLE_SVC_NOTIFICATIONS:
		case CMD_ENABLE_PASSIVE_SVC_CHECKS:
		case CMD_DISABLE_PASSIVE_SVC_CHECKS:
		case CMD_ENABLE_SVC_EVENT_HANDLER:
		case CMD_DISABLE_SVC_EVENT_HANDLER:
		case CMD_REMOVE_SVC_ACKNOWLEDGEMENT:
		case CMD_ENABLE_SVC_FLAP_DETECTION:
		case CMD_DISABLE_SVC_FLAP_DETECTION:
		case CMD_START_OBSESSING_OVER_SVC:
		case CMD_STOP_OBSESSING_OVER_SVC:
		case CMD_CLEAR_SVC_FLAPPING_STATE:
			printf("<tr><td CLASS='optBoxRequiredItem'>ホスト名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='host' VALUE='%s'>", escape_string(host_name));
			printf("</b></td></tr>\n");
			printf("<tr><td CLASS='optBoxRequiredItem'>サービス名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='service' VALUE='%s'>", escape_string(service_desc));
			printf("</b></td></tr>\n");
			print_comment_field(cmd);
			break;

		case CMD_ENABLE_HOST_SVC_CHECKS:
		case CMD_DISABLE_HOST_SVC_CHECKS:
		case CMD_DEL_ALL_HOST_COMMENTS:
		case CMD_ENABLE_HOST_NOTIFICATIONS:
		case CMD_DISABLE_HOST_NOTIFICATIONS:
		case CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
		case CMD_DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
		case CMD_ENABLE_HOST_SVC_NOTIFICATIONS:
		case CMD_DISABLE_HOST_SVC_NOTIFICATIONS:
		case CMD_ENABLE_HOST_EVENT_HANDLER:
		case CMD_DISABLE_HOST_EVENT_HANDLER:
		case CMD_ENABLE_HOST_CHECK:
		case CMD_DISABLE_HOST_CHECK:
		case CMD_REMOVE_HOST_ACKNOWLEDGEMENT:
		case CMD_ENABLE_HOST_FLAP_DETECTION:
		case CMD_DISABLE_HOST_FLAP_DETECTION:
		case CMD_ENABLE_PASSIVE_HOST_CHECKS:
		case CMD_DISABLE_PASSIVE_HOST_CHECKS:
		case CMD_START_OBSESSING_OVER_HOST:
		case CMD_STOP_OBSESSING_OVER_HOST:
		case CMD_CLEAR_HOST_FLAPPING_STATE:
			printf("<tr><td CLASS='optBoxRequiredItem'>ホスト名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='host' VALUE='%s'>", escape_string(host_name));
			printf("</b></td></tr>\n");
			print_comment_field(cmd);
			if(cmd == CMD_ENABLE_HOST_SVC_CHECKS || cmd == CMD_DISABLE_HOST_SVC_CHECKS || cmd == CMD_ENABLE_HOST_SVC_NOTIFICATIONS || cmd == CMD_DISABLE_HOST_SVC_NOTIFICATIONS) {
				printf("<tr><td CLASS='optBoxItem'>同時にホストも%sにする:</td><td><b>", (cmd == CMD_ENABLE_HOST_SVC_CHECKS || cmd == CMD_ENABLE_HOST_SVC_NOTIFICATIONS) ? "有効" : "無効");
				printf("<INPUT TYPE='checkbox' NAME='ahas'>");
				printf("</b></td></tr>\n");
				}
			if(cmd == CMD_ENABLE_HOST_NOTIFICATIONS || cmd == CMD_DISABLE_HOST_NOTIFICATIONS) {
				printf("<tr><td CLASS='optBoxItem'>同時に下位ホストへの通知も%sにする:</td><td><b>", (cmd == CMD_ENABLE_HOST_NOTIFICATIONS) ? "有効" : "無効");
				printf("<INPUT TYPE='checkbox' NAME='ptc'>");
				printf("</b></td></tr>\n");
				}
			break;

		case CMD_ENABLE_NOTIFICATIONS:
		case CMD_DISABLE_NOTIFICATIONS:
		case CMD_SHUTDOWN_PROCESS:
		case CMD_RESTART_PROCESS:
		case CMD_START_EXECUTING_SVC_CHECKS:
		case CMD_STOP_EXECUTING_SVC_CHECKS:
		case CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS:
		case CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS:
		case CMD_ENABLE_EVENT_HANDLERS:
		case CMD_DISABLE_EVENT_HANDLERS:
		case CMD_START_OBSESSING_OVER_SVC_CHECKS:
		case CMD_STOP_OBSESSING_OVER_SVC_CHECKS:
		case CMD_ENABLE_FLAP_DETECTION:
		case CMD_DISABLE_FLAP_DETECTION:
		case CMD_ENABLE_PERFORMANCE_DATA:
		case CMD_DISABLE_PERFORMANCE_DATA:
		case CMD_START_EXECUTING_HOST_CHECKS:
		case CMD_STOP_EXECUTING_HOST_CHECKS:
		case CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS:
		case CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS:
		case CMD_START_OBSESSING_OVER_HOST_CHECKS:
		case CMD_STOP_OBSESSING_OVER_HOST_CHECKS:
			if (print_comment_field(cmd) == FALSE)
				printf("<tr><td CLASS='optBoxItem' colspan=2>このコマンドにはオプションはありません。<BR>'発行'ボタンを押してコマンドを送信してください</td></tr>");
			break;

		case CMD_PROCESS_HOST_CHECK_RESULT:
		case CMD_PROCESS_SERVICE_CHECK_RESULT:
			printf("<tr><td CLASS='optBoxRequiredItem'>ホスト名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='host' VALUE='%s'>", escape_string(host_name));
			printf("</b></td></tr>\n");
			if(cmd == CMD_PROCESS_SERVICE_CHECK_RESULT) {
				printf("<tr><td CLASS='optBoxRequiredItem'>サービス名:</td><td><b>");
				printf("<INPUT TYPE='TEXT' NAME='service' VALUE='%s'>", escape_string(service_desc));
				printf("</b></td></tr>\n");
				}
			printf("<tr><td CLASS='optBoxRequiredItem'>チェック結果:</td><td><b>");
			printf("<SELECT NAME='plugin_state'>");
			if(cmd == CMD_PROCESS_SERVICE_CHECK_RESULT) {
				printf("<OPTION VALUE=%d SELECTED>OK\n", STATE_OK);
				printf("<OPTION VALUE=%d>WARNING\n", STATE_WARNING);
				printf("<OPTION VALUE=%d>UNKNOWN\n", STATE_UNKNOWN);
				printf("<OPTION VALUE=%d>CRITICAL\n", STATE_CRITICAL);
				}
			else {
				printf("<OPTION VALUE=0 SELECTED>UP\n");
				printf("<OPTION VALUE=1>DOWN\n");
				printf("<OPTION VALUE=2>UNREACHABLE\n");
				}
			printf("</SELECT>\n");
			printf("</b></td></tr>\n");
			printf("<tr><td CLASS='optBoxRequiredItem'>チェック出力:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='plugin_output' VALUE=''>");
			printf("</b></td></tr>\n");
			printf("<tr><td CLASS='optBoxItem'>パフォーマンスデータ:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='performance_data' VALUE=''>");
			printf("</b></td></tr>\n");
			break;

		case CMD_SCHEDULE_HOST_DOWNTIME:
		case CMD_SCHEDULE_HOST_SVC_DOWNTIME:
		case CMD_SCHEDULE_SVC_DOWNTIME:

			printf("<tr><td CLASS='optBoxRequiredItem'>ホスト名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='host' VALUE='%s'>", escape_string(host_name));
			printf("</b></td></tr>\n");
			if(cmd == CMD_SCHEDULE_SVC_DOWNTIME) {
				printf("<tr><td CLASS='optBoxRequiredItem'>サービス名:</td><td><b>");
				printf("<INPUT TYPE='TEXT' NAME='service' VALUE='%s'>", escape_string(service_desc));
				}
			print_comment_field(cmd);

			printf("<tr><td CLASS='optBoxItem'><br></td></tr>\n");

			printf("<tr><td CLASS='optBoxItem'>トリガー:</td><td>\n");
			printf("<select name='trigger'>\n");
			printf("<option value='0'>N/A\n");

			for(temp_downtime = scheduled_downtime_list; temp_downtime != NULL; temp_downtime = temp_downtime->next) {
				if(temp_downtime->type != HOST_DOWNTIME)
					continue;
				printf("<option value='%lu'>", temp_downtime->downtime_id);
				get_time_string(&temp_downtime->start_time, start_time_str, sizeof(start_time_str), SHORT_DATE_TIME);
				printf("ID: %lu, Host '%s' starting @ %s\n", temp_downtime->downtime_id, temp_downtime->host_name, start_time_str);
				}
			for(temp_downtime = scheduled_downtime_list; temp_downtime != NULL; temp_downtime = temp_downtime->next) {
				if(temp_downtime->type != SERVICE_DOWNTIME)
					continue;
				printf("<option value='%lu'>", temp_downtime->downtime_id);
				get_time_string(&temp_downtime->start_time, start_time_str, sizeof(start_time_str), SHORT_DATE_TIME);
				printf("ID: %lu, Service '%s' on host '%s' starting @ %s \n", temp_downtime->downtime_id, temp_downtime->service_description, temp_downtime->host_name, start_time_str);
				}

			printf("</select>\n");
			printf("</td></tr>\n");

			printf("<tr><td CLASS='optBoxItem'><br></td></tr>\n");

			time(&t);
			get_time_string(&t, buffer, sizeof(buffer) - 1, SHORT_DATE_TIME);
			printf("<tr><td CLASS='optBoxRequiredItem'>開始時刻:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='start_time' VALUE='%s'>", buffer);
			printf("</b></td></tr>\n");
			t += (unsigned long)7200;
			get_time_string(&t, buffer, sizeof(buffer) - 1, SHORT_DATE_TIME);
			printf("<tr><td CLASS='optBoxRequiredItem'>終了時刻:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='end_time' VALUE='%s'>", buffer);
			printf("</b></td></tr>\n");
			printf("<tr><td CLASS='optBoxItem'>種類:</td><td><b>");
			printf("<SELECT NAME='fixed'>");
			printf("<OPTION VALUE=1>固定\n");
			printf("<OPTION VALUE=0>非固定\n");
			printf("</SELECT>\n");
			printf("</b></td></tr>\n");

			printf("<tr><td CLASS='optBoxItem'>期間(「非固定」を選択した場合のみ):</td><td>");
			printf("<table border=0><tr>\n");
			printf("<td align=right><INPUT TYPE='TEXT' NAME='hours' VALUE='2' SIZE=2 MAXLENGTH=2></td>\n");
			printf("<td align=left>時間</td>\n");
			printf("<td align=right><INPUT TYPE='TEXT' NAME='minutes' VALUE='0' SIZE=2 MAXLENGTH=2></td>\n");
			printf("<td align=left>分</td>\n");
			printf("</tr></table>\n");
			printf("</td></tr>\n");

			printf("<tr><td CLASS='optBoxItem'><br></td></tr>\n");

			if(cmd == CMD_SCHEDULE_HOST_DOWNTIME) {
				printf("<tr><td CLASS='optBoxItem'>下位ホスト:</td><td><b>");
				printf("<SELECT name='childoptions'>");
				printf("<option value='0'>何もしない\n");
				printf("<option value='1'>このダウンタイムをトリガとして全ての下位ホストにダウンタイムを設定する\n");
				printf("<option value='2'>このダウンタイムを同じように下位ホストに設定する\n");
				printf("</SELECT>\n");
				printf("</b></td></tr>\n");
				}

			printf("<tr><td CLASS='optBoxItem'><br></td></tr>\n");

			break;

		case CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS:
		case CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS:
		case CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS:
		case CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS:
		case CMD_ENABLE_HOSTGROUP_SVC_CHECKS:
		case CMD_DISABLE_HOSTGROUP_SVC_CHECKS:
			printf("<tr><td CLASS='optBoxRequiredItem'>ホストグループ名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='hostgroup' VALUE='%s'>", escape_string(hostgroup_name));
			printf("</b></td></tr>\n");
			if(cmd == CMD_ENABLE_HOSTGROUP_SVC_CHECKS || cmd == CMD_DISABLE_HOSTGROUP_SVC_CHECKS || cmd == CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS || cmd == CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS) {
				printf("<tr><td CLASS='optBoxItem'>同時にホストも%sにする:</td><td><b>", (cmd == CMD_ENABLE_HOSTGROUP_SVC_CHECKS || cmd == CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS) ? "有効" : "無効");
				printf("<INPUT TYPE='checkbox' NAME='ahas'>");
				printf("</b></td></tr>\n");
				}
			print_comment_field(cmd);
			break;

		case CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
		case CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
		case CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
		case CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
		case CMD_ENABLE_SERVICEGROUP_SVC_CHECKS:
		case CMD_DISABLE_SERVICEGROUP_SVC_CHECKS:
			printf("<tr><td CLASS='optBoxRequiredItem'>サービスグループ名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='servicegroup' VALUE='%s'>", escape_string(servicegroup_name));
			printf("</b></td></tr>\n");
			if(cmd == CMD_ENABLE_SERVICEGROUP_SVC_CHECKS || cmd == CMD_DISABLE_SERVICEGROUP_SVC_CHECKS || cmd == CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS || cmd == CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS) {
				printf("<tr><td CLASS='optBoxItem'>同時にホストも%sにする:</td><td><b>", (cmd == CMD_ENABLE_SERVICEGROUP_SVC_CHECKS || cmd == CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS) ? "有効" : "無効");
				printf("<INPUT TYPE='checkbox' NAME='ahas'>");
				printf("</b></td></tr>\n");
				}
			print_comment_field(cmd);
			break;

		case CMD_DEL_HOST_DOWNTIME:
		case CMD_DEL_SVC_DOWNTIME:
			printf("<tr><td CLASS='optBoxRequiredItem'>ダウンタイム ID:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='down_id' VALUE='%lu'>", downtime_id);
			printf("</b></td></tr>\n");
			print_comment_field(cmd);
			break;


		case CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME:
		case CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME:
		case CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME:
		case CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME:

			if(cmd == CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME || cmd == CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME) {
				printf("<tr><td CLASS='optBoxRequiredItem'>ホストグループ名:</td><td><b>");
				printf("<INPUT TYPE='TEXT' NAME='hostgroup' VALUE='%s'>", escape_string(hostgroup_name));
				printf("</b></td></tr>\n");
				}
			else {
				printf("<tr><td CLASS='optBoxRequiredItem'>サービスグループ名:</td><td><b>");
				printf("<INPUT TYPE='TEXT' NAME='servicegroup' VALUE='%s'>", escape_string(servicegroup_name));
				printf("</b></td></tr>\n");
				}
			print_comment_field(cmd);
			time(&t);
			get_time_string(&t, buffer, sizeof(buffer) - 1, SHORT_DATE_TIME);
			printf("<tr><td CLASS='optBoxRequiredItem'>開始時刻:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='start_time' VALUE='%s'>", buffer);
			printf("</b></td></tr>\n");
			t += (unsigned long)7200;
			get_time_string(&t, buffer, sizeof(buffer) - 1, SHORT_DATE_TIME);
			printf("<tr><td CLASS='optBoxRequiredItem'>終了時刻:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='end_time' VALUE='%s'>", buffer);
			printf("</b></td></tr>\n");
			printf("<tr><td CLASS='optBoxItem'>種類:</td><td><b>");
			printf("<SELECT NAME='fixed'>");
			printf("<OPTION VALUE=1>固定\n");
			printf("<OPTION VALUE=0>非固定\n");
			printf("</SELECT>\n");
			printf("</b></td></tr>\n");

			printf("<tr><td CLASS='optBoxItem'>期間(「非固定」を選択した場合のみ):</td><td>");
			printf("<table border=0><tr>\n");
			printf("<td align=right><INPUT TYPE='TEXT' NAME='hours' VALUE='2' SIZE=2 MAXLENGTH=2></td>\n");
			printf("<td align=left>時間</td>\n");
			printf("<td align=right><INPUT TYPE='TEXT' NAME='minutes' VALUE='0' SIZE=2 MAXLENGTH=2></td>\n");
			printf("<td align=left>分</td>\n");
			printf("</tr></table>\n");
			printf("</td></tr>\n");
			if(cmd == CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME || cmd == CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME) {
				printf("<tr><td CLASS='optBoxItem'>同時にホストもダウンタイムをスケジュールする:</td><td><b>");
				printf("<INPUT TYPE='checkbox' NAME='ahas'>");
				printf("</b></td></tr>\n");
				}
			break;

		case CMD_SEND_CUSTOM_HOST_NOTIFICATION:
		case CMD_SEND_CUSTOM_SVC_NOTIFICATION:
			printf("<tr><td CLASS='optBoxRequiredItem'>ホスト名:</td><td><b>");
			printf("<INPUT TYPE='TEXT' NAME='host' VALUE='%s'>", escape_string(host_name));
			printf("</b></td></tr>\n");

			if(cmd == CMD_SEND_CUSTOM_SVC_NOTIFICATION) {
				printf("<tr><td CLASS='optBoxRequiredItem'>サービス名:</td><td><b>");
				printf("<INPUT TYPE='TEXT' NAME='service' VALUE='%s'>", escape_string(service_desc));
				printf("</b></td></tr>\n");
				}

			printf("<tr><td CLASS='optBoxItem'>強制的に通知する:</td><td><b>");
			printf("<INPUT TYPE='checkbox' NAME='force_notification' ");
			printf("</b></td></tr>\n");

			printf("<tr><td CLASS='optBoxItem'>全員に通知する:</td><td><b>");
			printf("<INPUT TYPE='checkbox' NAME='broadcast_notification' ");
			printf("</b></td></tr>\n");

			print_comment_field(cmd);
			break;

		default:
			printf("<tr><td CLASS='optBoxItem'>何かおかしいです... (´・ω・｀)</td><td></td></tr>\n");
		}


	printf("<tr><td CLASS='optBoxItem' COLSPAN=2></td></tr>\n");
	printf("<tr><td CLASS='optBoxItem'></td><td CLASS='optBoxItem'><INPUT TYPE='submit' NAME='btnSubmit' VALUE='発行'> <INPUT TYPE='reset' VALUE='リセット'></td></tr>\n");

	printf("</table>\n");
	printf("</form>\n");
	printf("</td>\n");
	printf("</tr>\n");
	printf("</table>\n");

	printf("</td>\n");
	printf("<td align=center valign=top width=50%%>\n");

	/* show information about the command... */
	show_command_help(cmd);

	printf("</td>\n");
	printf("</tr>\n");
	printf("</table>\n");

	printf("</div>\n");
	printf("</p>\n");

	printf("<P><DIV CLASS='infoMessage'>コマンドを発行する前に必須項目(赤色)に入力してください。<br>入力されていない場合エラーになります。</DIV></P>");

	return;
	}


int print_comment_field(int cmd_id)
{
	char					*reqtext = "optBoxItem";
	char					*comment = comment_data;
	struct nagios_extcmd	*ecmd = extcmd_get_command_id(cmd_id);

	if (!ecmd || ecmd->cmt_opt == 0)
		return FALSE;

	if (ecmd->cmt_opt == 2)
		reqtext = "optBoxRequiredItem";

	if (!comment || !*comment) {
		if (ecmd->default_comment)
			comment = ecmd->default_comment;
	}

	printf("<tr><td CLASS='%s'>作成者:</td><td><b>", reqtext);
	printf("<INPUT TYPE='TEXT' NAME='com_author' VALUE='%s' %s>", escape_string(comment_author), (lock_author_names == TRUE) ? "READONLY DISABLED" : "");
	printf("</b></td></tr>\n");
	printf("<tr><td CLASS='%s'>コメント:</td><td><b>", reqtext);
	printf("<INPUT TYPE='TEXT' NAME='com_data' VALUE='%s' SIZE=40>", escape_string(comment));
	printf("</b></td></tr>\n");
	return TRUE;
}


void commit_command_data(int cmd) {
	char *error_string = NULL;
	int result = OK;
	int authorized = FALSE;
	service *temp_service;
	host *temp_host;
	hostgroup *temp_hostgroup;
	nagios_comment *temp_comment;
	scheduled_downtime *temp_downtime;
	servicegroup *temp_servicegroup = NULL;
	contact *temp_contact = NULL;

	struct nagios_extcmd *ecmd = extcmd_get_command_id(cmd);

	/* get authentication information */
	get_authentication_information(&current_authdata);

	/* get name to use for author */
	if(lock_author_names == TRUE) {
		temp_contact = find_contact(current_authdata.username);
		if(temp_contact != NULL && temp_contact->alias != NULL) {
			comment_author = temp_contact->alias;
		}
		else {
			comment_author = current_authdata.username;
		}
	}

	if (ecmd->cmt_opt == 2 && *comment_data == '\0') {
		if(!error_string) {
			error_string = strdup("コメントが入力されていません");
		}
	}
	clean_comment_data(comment_data);
	if (*comment_data != '\0' && *comment_author == '\0') {
		if(!error_string) {
			error_string = strdup("作成者が入力されていません");
		}
	}
	clean_comment_data(comment_author);

	switch(cmd) {
		case CMD_ADD_HOST_COMMENT:
		case CMD_ACKNOWLEDGE_HOST_PROBLEM:

			/* see if the user is authorized to issue a command... */
			temp_host = find_host(host_name);
			if(is_authorized_for_host_commands(temp_host, &current_authdata) == TRUE) {
				authorized = TRUE;
			}
			break;

		case CMD_ADD_SVC_COMMENT:
		case CMD_ACKNOWLEDGE_SVC_PROBLEM:

			/* see if the user is authorized to issue a command... */
			temp_service = find_service(host_name, service_desc);
			if(is_authorized_for_service_commands(temp_service, &current_authdata) == TRUE) {
				authorized = TRUE;
			}
			break;

		case CMD_DEL_HOST_COMMENT:
		case CMD_DEL_SVC_COMMENT:

			/* check the sanity of the comment id */
			if(comment_id == 0) {
				if(!error_string) {
					error_string = strdup("コメントIDは0にできません");
				}
			}

			/* find the comment */
			if(cmd == CMD_DEL_HOST_COMMENT) {
				temp_comment = find_host_comment(comment_id);
			}
			else {
				temp_comment = find_service_comment(comment_id);
			}

			/* see if the user is authorized to issue a command... */
			if(cmd == CMD_DEL_HOST_COMMENT && temp_comment != NULL) {
				temp_host = find_host(temp_comment->host_name);
				if(is_authorized_for_host_commands(temp_host, &current_authdata) == TRUE) {
					authorized = TRUE;
				}
			}
			if(cmd == CMD_DEL_SVC_COMMENT && temp_comment != NULL) {
				temp_service = find_service(temp_comment->host_name, temp_comment->service_description);
				if(is_authorized_for_service_commands(temp_service, &current_authdata) == TRUE) {
					authorized = TRUE;
				}
			}

			/* free comment data */
			free_comment_data();

			break;

		case CMD_DEL_HOST_DOWNTIME:
		case CMD_DEL_SVC_DOWNTIME:

			/* check the sanity of the downtime id */
			if(downtime_id == 0) {
				if(!error_string) {
					error_string = strdup("ダウンタイムIDは0にできません");
				}
			}

			/* find the downtime entry */
			if(cmd == CMD_DEL_HOST_DOWNTIME) {
				temp_downtime = find_host_downtime(downtime_id);
			}
			else {
				temp_downtime = find_service_downtime(downtime_id);
			}

			/* see if the user is authorized to issue a command... */
			if(cmd == CMD_DEL_HOST_DOWNTIME && temp_downtime != NULL) {
				temp_host = find_host(temp_downtime->host_name);
				if(is_authorized_for_host_commands(temp_host, &current_authdata) == TRUE) {
					authorized = TRUE;
				}
			}
			if(cmd == CMD_DEL_SVC_DOWNTIME && temp_downtime != NULL) {
				temp_service = find_service(temp_downtime->host_name, temp_downtime->service_description);
				if(is_authorized_for_service_commands(temp_service, &current_authdata) == TRUE) {
					authorized = TRUE;
				}
			}

			/* free downtime data */
			free_downtime_data();

			break;

		case CMD_SCHEDULE_SVC_CHECK:
		case CMD_ENABLE_SVC_CHECK:
		case CMD_DISABLE_SVC_CHECK:
		case CMD_DEL_ALL_SVC_COMMENTS:
		case CMD_ENABLE_SVC_NOTIFICATIONS:
		case CMD_DISABLE_SVC_NOTIFICATIONS:
		case CMD_ENABLE_PASSIVE_SVC_CHECKS:
		case CMD_DISABLE_PASSIVE_SVC_CHECKS:
		case CMD_ENABLE_SVC_EVENT_HANDLER:
		case CMD_DISABLE_SVC_EVENT_HANDLER:
		case CMD_REMOVE_SVC_ACKNOWLEDGEMENT:
		case CMD_PROCESS_SERVICE_CHECK_RESULT:
		case CMD_SCHEDULE_SVC_DOWNTIME:
		case CMD_DELAY_SVC_NOTIFICATION:
		case CMD_ENABLE_SVC_FLAP_DETECTION:
		case CMD_DISABLE_SVC_FLAP_DETECTION:
		case CMD_START_OBSESSING_OVER_SVC:
		case CMD_STOP_OBSESSING_OVER_SVC:
		case CMD_CLEAR_SVC_FLAPPING_STATE:

			/* see if the user is authorized to issue a command... */
			temp_service = find_service(host_name, service_desc);
			if(is_authorized_for_service_commands(temp_service, &current_authdata) == TRUE) {
				authorized = TRUE;
			}

			/* make sure we have passive check info (if necessary) */
			if(cmd == CMD_PROCESS_SERVICE_CHECK_RESULT && !strcmp(plugin_output, "")) {
				if(!error_string) {
					error_string = strdup("プラグイン出力は空にできません");
				}
			}

			/* make sure we have a notification delay (if necessary) */
			if(cmd == CMD_DELAY_SVC_NOTIFICATION && notification_delay <= 0) {
				if(!error_string) {
					error_string = strdup("通知の遅延は0よりも大きくしなければなりません");
				}
			}

			/* make sure we have check time (if necessary) */
			if(cmd == CMD_SCHEDULE_SVC_CHECK && start_time == (time_t)0) {
				if(!error_string) {
					error_string = strdup("開始時間が不正な形式が送信されたか、0以外のものでなければなりません。");
				}
			}

			/* make sure we have start/end times for downtime (if necessary) */
			if(cmd == CMD_SCHEDULE_SVC_DOWNTIME && (start_time == (time_t)0 || end_time == (time_t)0 || end_time < start_time)) {
				if(!error_string) {
					error_string = strdup("開始または終了時間が不正です");
				}
			}

			break;

		case CMD_ENABLE_NOTIFICATIONS:
		case CMD_DISABLE_NOTIFICATIONS:
		case CMD_SHUTDOWN_PROCESS:
		case CMD_RESTART_PROCESS:
		case CMD_START_EXECUTING_SVC_CHECKS:
		case CMD_STOP_EXECUTING_SVC_CHECKS:
		case CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS:
		case CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS:
		case CMD_ENABLE_EVENT_HANDLERS:
		case CMD_DISABLE_EVENT_HANDLERS:
		case CMD_START_OBSESSING_OVER_SVC_CHECKS:
		case CMD_STOP_OBSESSING_OVER_SVC_CHECKS:
		case CMD_ENABLE_FLAP_DETECTION:
		case CMD_DISABLE_FLAP_DETECTION:
		case CMD_ENABLE_PERFORMANCE_DATA:
		case CMD_DISABLE_PERFORMANCE_DATA:
		case CMD_START_EXECUTING_HOST_CHECKS:
		case CMD_STOP_EXECUTING_HOST_CHECKS:
		case CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS:
		case CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS:
		case CMD_START_OBSESSING_OVER_HOST_CHECKS:
		case CMD_STOP_OBSESSING_OVER_HOST_CHECKS:

			/* see if the user is authorized to issue a command... */
			if(is_authorized_for_system_commands(&current_authdata) == TRUE) {
				authorized = TRUE;
			}
			break;

		case CMD_ENABLE_HOST_SVC_CHECKS:
		case CMD_DISABLE_HOST_SVC_CHECKS:
		case CMD_DEL_ALL_HOST_COMMENTS:
		case CMD_SCHEDULE_HOST_SVC_CHECKS:
		case CMD_ENABLE_HOST_NOTIFICATIONS:
		case CMD_DISABLE_HOST_NOTIFICATIONS:
		case CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
		case CMD_DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
		case CMD_ENABLE_HOST_SVC_NOTIFICATIONS:
		case CMD_DISABLE_HOST_SVC_NOTIFICATIONS:
		case CMD_ENABLE_HOST_EVENT_HANDLER:
		case CMD_DISABLE_HOST_EVENT_HANDLER:
		case CMD_ENABLE_HOST_CHECK:
		case CMD_DISABLE_HOST_CHECK:
		case CMD_REMOVE_HOST_ACKNOWLEDGEMENT:
		case CMD_SCHEDULE_HOST_DOWNTIME:
		case CMD_SCHEDULE_HOST_SVC_DOWNTIME:
		case CMD_DELAY_HOST_NOTIFICATION:
		case CMD_ENABLE_HOST_FLAP_DETECTION:
		case CMD_DISABLE_HOST_FLAP_DETECTION:
		case CMD_PROCESS_HOST_CHECK_RESULT:
		case CMD_ENABLE_PASSIVE_HOST_CHECKS:
		case CMD_DISABLE_PASSIVE_HOST_CHECKS:
		case CMD_SCHEDULE_HOST_CHECK:
		case CMD_START_OBSESSING_OVER_HOST:
		case CMD_STOP_OBSESSING_OVER_HOST:
		case CMD_CLEAR_HOST_FLAPPING_STATE:

			/* see if the user is authorized to issue a command... */
			temp_host = find_host(host_name);
			if(is_authorized_for_host_commands(temp_host, &current_authdata) == TRUE) {
				authorized = TRUE;
			}

			/* make sure we have a notification delay (if necessary) */
			if(cmd == CMD_DELAY_HOST_NOTIFICATION && notification_delay <= 0) {
				if(!error_string) {
					error_string = strdup("通知の遅延は0以上でなければなりません");
				}
			}

			/* make sure we have start/end times for downtime (if necessary) */
			if((cmd == CMD_SCHEDULE_HOST_DOWNTIME || cmd == CMD_SCHEDULE_HOST_SVC_DOWNTIME) && (start_time == (time_t)0 || end_time == (time_t)0 || start_time > end_time)) {
				if(!error_string) {
					error_string = strdup("開始または終了時間が不正です");
				}
			}

			/* make sure we have check time (if necessary) */
			if((cmd == CMD_SCHEDULE_HOST_CHECK || cmd == CMD_SCHEDULE_HOST_SVC_CHECKS) && start_time == (time_t)0) {
				if(!error_string) {
					error_string = strdup("開始時間は不正な形式で送信されたか、0以外のものでなければなりません。");
				}
			}

			/* make sure we have passive check info (if necessary) */
			if(cmd == CMD_PROCESS_HOST_CHECK_RESULT && !strcmp(plugin_output, "")) {
				if(!error_string) {
					error_string = strdup("プラグイン出力は空にできません");
				}
			}

			break;

		case CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS:
		case CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS:
		case CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS:
		case CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS:
		case CMD_ENABLE_HOSTGROUP_SVC_CHECKS:
		case CMD_DISABLE_HOSTGROUP_SVC_CHECKS:
		case CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME:
		case CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME:

			/* make sure we have start/end times for downtime */
			if((cmd == CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME || cmd == CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME) && (start_time == (time_t)0 || end_time == (time_t)0 || start_time > end_time)) {
				if(!error_string) {
					error_string = strdup("開始または終了時間が不正です");
				}
			}

			/* see if the user is authorized to issue a command... */
			temp_hostgroup = find_hostgroup(hostgroup_name);
			if(is_authorized_for_hostgroup_commands(temp_hostgroup, &current_authdata) == TRUE) {
				authorized = TRUE;
			}

			break;

		case CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
		case CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
		case CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
		case CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
		case CMD_ENABLE_SERVICEGROUP_SVC_CHECKS:
		case CMD_DISABLE_SERVICEGROUP_SVC_CHECKS:
		case CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME:
		case CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME:

			/* make sure we have start/end times for downtime */
			if((cmd == CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME || cmd == CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME) && (start_time == (time_t)0 || end_time == (time_t)0 || start_time > end_time)) {
				if(!error_string) {
					error_string = strdup("開始または終了時間が不正です");
				}
			}

			/* see if the user is authorized to issue a command... */

			temp_servicegroup = find_servicegroup(servicegroup_name);
			if(is_authorized_for_servicegroup_commands(temp_servicegroup, &current_authdata) == TRUE) {
				authorized = TRUE;
			}

			break;

		case CMD_SEND_CUSTOM_HOST_NOTIFICATION:
		case CMD_SEND_CUSTOM_SVC_NOTIFICATION:

			/* see if the user is authorized to issue a command... */
			if(cmd == CMD_SEND_CUSTOM_HOST_NOTIFICATION) {
				temp_host = find_host(host_name);
				if(is_authorized_for_host_commands(temp_host, &current_authdata) == TRUE) {
					authorized = TRUE;
				}
			}
			else {
				temp_service = find_service(host_name, service_desc);
				if(is_authorized_for_service_commands(temp_service, &current_authdata) == TRUE) {
					authorized = TRUE;
				}
			}
			break;

		default:
			if(!error_string) {
				error_string = strdup("コマンドの実行中にエラーが発生しました！");
			}
		}


	/* to be safe, we are going to REQUIRE that the authentication functionality is enabled... */
	if(use_authentication == FALSE) {
		if(content_type == WML_CONTENT) {
			printf("<p>エラー: CGIの認証機能が無効になっています。</p>\n");
		}
		else {
			printf("<P>\n");
			printf("<DIV CLASS='errorMessage'>実行できません。</DIV><br>");
			printf("<DIV CLASS='errorDescription'>");
			printf("CGIの認証機能が無効になっています<br><br>");
			printf("これは無許可のユーザがNagiosに対してコマンドを使用できてしまうためあまり望ましくありません。");
			printf("それでもコマンドを使用出来るようにする場合は、Nagiosのセキュリティ機能を削除する必要があります。<br><br>");
			printf("<strong>HTMLドキュメントの CGI認証機能の項目を熟読してから認証機能の設定を行ってください。</strong>\n");
			printf("</DIV>\n");
			printf("</P>\n");
		}
	}

	/* the user is not authorized to issue the given command */
	else if(authorized == FALSE) {
		if(content_type == WML_CONTENT) {
			printf("<p>エラー: このコマンドを発行する権限がありません。</p>\n");
		}
		else {
			printf("<P><DIV CLASS='errorMessage'>このコマンドを発行する権限がありません。</DIV></P>\n");
			printf("<P><DIV CLASS='errorDescription'>HTMLドキュメントの CGI認証機能の項目を熟読してから認証機能の設定を行ってください。<BR><BR>\n");
			printf("<A HREF='javascript:window.history.go(-2)'>元のページへ戻る</A></DIV></P>\n");
		}
	}

	/* some error occurred (data was probably missing) */
	else if(error_string) {
		if(content_type == WML_CONTENT) {
			printf("<p>%s</p>\n", error_string);
		}
		else {
			printf("<P><DIV CLASS='errorMessage'>%s</DIV></P>\n", error_string);
			free(error_string);
			printf("<P><DIV CLASS='errorDescription'>入力に誤りが無いか<A HREF='javascript:window.history.go(-1)'>戻って</A>確認してください。<BR>\n");
			printf("<A HREF='javascript:window.history.go(-2)'>元のページへ戻る</A></DIV></P>\n");
		}
	}

	/* if Nagios isn't checking external commands, don't do anything... */
	else if(check_external_commands == FALSE) {
		if(content_type == WML_CONTENT) {
			printf("<p>エラー: Nagiosが外部コマンドをチェックできないためコマンド発行できませんでした。</p>\n");
		}
		else {
			printf("<P><DIV CLASS='errorMessage'>Nagiosが外部コマンドをチェックできないためコマンド発行できませんでした。</DIV></P>\n");
			printf("<P><DIV CLASS='errorDescription'>外部コマンド使い方についてドキュメントを読んでください。<BR><BR>\n");
			printf("<A HREF='javascript:window.history.go(-2)'>元のページへ戻る</A></DIV></P>\n");
		}
	}

	/* everything looks okay, so let's go ahead and commit the command... */
	else {

		/* commit the command */
		result = commit_command(cmd);

		if(result == OK) {
			if(content_type == WML_CONTENT) {
				printf("<p>コマンドを正常に受け付けました。</p>\n");
			}
			else {
				printf("<P><DIV CLASS='infoMessage'>コマンドを正常に受け付けました。<BR><BR>\n");
				printf("注)コマンドが実行されるまではしばらく時間がかかります。<BR><BR>\n");
				printf("<A HREF='javascript:window.history.go(-2)'>了解</A></DIV></P>");
			}
		}
		else {
			if(content_type == WML_CONTENT) {
				printf("<p>コマンドを処理する際にエラーが発生しました。</p>\n");
			}
			else {
				printf("<P><DIV CLASS='errorMessage'>コマンドを処理する際にエラーが発生しました。<BR><BR>\n");
				printf("<A HREF='javascript:window.history.go(-2)'>元のページへ戻る</A></DIV></P>\n");
			}
		}
	}

	my_free(error_string);
}

__attribute__((format(printf, 2, 3)))
static int cmd_submitf(int id, const char *fmt, ...) {
	char cmd[MAX_EXTERNAL_COMMAND_LENGTH];
	const char *command_name;
	int len;
	int len2;
	va_list ap;

	command_name = extcmd_get_name(id);
	/*
	 * We disallow sending 'CHANGE' commands from the cgi's
	 * until we do proper session handling to prevent cross-site
	 * request forgery
	 */
	if(!command_name || (strlen(command_name) > 6 && !memcmp("CHANGE", command_name, 6)))
		return ERROR;

	len = snprintf(cmd, sizeof(cmd), "[%lu] %s;", time(NULL), command_name);
	if(len < 0 || len >= sizeof(cmd))
		return ERROR;

	if(fmt) {
		va_start(ap, fmt);
		len2 = vsnprintf(cmd + len, sizeof(cmd) - len, fmt, ap);
		va_end(ap);
		len += len2;
		if(len2 < 0 || len >= sizeof(cmd))
			return ERROR;
		}

	if (*comment_data != '\0') {
		len2 = snprintf(cmd + len, sizeof(cmd) - len, ";%s;%s", comment_author, comment_data);
		len += len2;
		if(len2 < 0 || len >= sizeof(cmd))
			return ERROR;
	}

	cmd[len] = 0; /* 0 <= len < sizeof(cmd) */
	return write_command_to_file(cmd);
	}



/* commits a command for processing */
int commit_command(int cmd) {
	time_t current_time;
	time_t scheduled_time;
	time_t notification_time;
	int result;

	/* get the current time */
	time(&current_time);

	/* get the scheduled time */
	scheduled_time = current_time + (schedule_delay * 60);

	/* get the notification time */
	notification_time = current_time + (notification_delay * 60);

	/*
	 * these are supposed to be implanted inside the
	 * completed commands shipped off to nagios and
	 * must therefore never contain ';'
	 */
	if(host_name && strchr(host_name, ';'))
		return ERROR;
	if(service_desc && strchr(service_desc, ';'))
		return ERROR;
	if(comment_author && strchr(comment_author, ';'))
		return ERROR;
	if(hostgroup_name && strchr(hostgroup_name, ';'))
		return ERROR;
	if(servicegroup_name && strchr(servicegroup_name, ';'))
		return ERROR;

	/* decide how to form the command line... */
	switch(cmd) {

			/* commands without arguments */
		case CMD_START_EXECUTING_SVC_CHECKS:
		case CMD_STOP_EXECUTING_SVC_CHECKS:
		case CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS:
		case CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS:
		case CMD_ENABLE_EVENT_HANDLERS:
		case CMD_DISABLE_EVENT_HANDLERS:
		case CMD_START_OBSESSING_OVER_SVC_CHECKS:
		case CMD_STOP_OBSESSING_OVER_SVC_CHECKS:
		case CMD_ENABLE_FLAP_DETECTION:
		case CMD_DISABLE_FLAP_DETECTION:
		case CMD_ENABLE_PERFORMANCE_DATA:
		case CMD_DISABLE_PERFORMANCE_DATA:
		case CMD_START_EXECUTING_HOST_CHECKS:
		case CMD_STOP_EXECUTING_HOST_CHECKS:
		case CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS:
		case CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS:
		case CMD_START_OBSESSING_OVER_HOST_CHECKS:
		case CMD_STOP_OBSESSING_OVER_HOST_CHECKS:
			result = cmd_submitf(cmd, NULL);
			break;

			/** simple host commands **/
		case CMD_ENABLE_HOST_FLAP_DETECTION:
		case CMD_DISABLE_HOST_FLAP_DETECTION:
		case CMD_ENABLE_PASSIVE_HOST_CHECKS:
		case CMD_DISABLE_PASSIVE_HOST_CHECKS:
		case CMD_START_OBSESSING_OVER_HOST:
		case CMD_STOP_OBSESSING_OVER_HOST:
		case CMD_DEL_ALL_HOST_COMMENTS:
		case CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
		case CMD_DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
		case CMD_ENABLE_HOST_EVENT_HANDLER:
		case CMD_DISABLE_HOST_EVENT_HANDLER:
		case CMD_ENABLE_HOST_CHECK:
		case CMD_DISABLE_HOST_CHECK:
		case CMD_REMOVE_HOST_ACKNOWLEDGEMENT:
		case CMD_CLEAR_HOST_FLAPPING_STATE:
			result = cmd_submitf(cmd, "%s", host_name);
			break;

			/** simple service commands **/
		case CMD_ENABLE_SVC_FLAP_DETECTION:
		case CMD_DISABLE_SVC_FLAP_DETECTION:
		case CMD_ENABLE_PASSIVE_SVC_CHECKS:
		case CMD_DISABLE_PASSIVE_SVC_CHECKS:
		case CMD_START_OBSESSING_OVER_SVC:
		case CMD_STOP_OBSESSING_OVER_SVC:
		case CMD_DEL_ALL_SVC_COMMENTS:
		case CMD_ENABLE_SVC_NOTIFICATIONS:
		case CMD_DISABLE_SVC_NOTIFICATIONS:
		case CMD_ENABLE_SVC_EVENT_HANDLER:
		case CMD_DISABLE_SVC_EVENT_HANDLER:
		case CMD_ENABLE_SVC_CHECK:
		case CMD_DISABLE_SVC_CHECK:
		case CMD_REMOVE_SVC_ACKNOWLEDGEMENT:
		case CMD_CLEAR_SVC_FLAPPING_STATE:
			result = cmd_submitf(cmd, "%s;%s", host_name, service_desc);
			break;

		case CMD_ADD_HOST_COMMENT:
			result = cmd_submitf(cmd, "%s;%d", host_name, persistent_comment);
			break;

		case CMD_ADD_SVC_COMMENT:
			result = cmd_submitf(cmd, "%s;%s;%d", host_name, service_desc, persistent_comment);
			break;

		case CMD_DEL_HOST_COMMENT:
		case CMD_DEL_SVC_COMMENT:
			result = cmd_submitf(cmd, "%lu", comment_id);
			break;

		case CMD_DELAY_HOST_NOTIFICATION:
			result = cmd_submitf(cmd, "%s;%lu", host_name, notification_time);
			break;

		case CMD_DELAY_SVC_NOTIFICATION:
			result = cmd_submitf(cmd, "%s;%s;%lu", host_name, service_desc, notification_time);
			break;

		case CMD_SCHEDULE_SVC_CHECK:
		case CMD_SCHEDULE_FORCED_SVC_CHECK:
			if(force_check == TRUE)
				cmd = CMD_SCHEDULE_FORCED_SVC_CHECK;
			result = cmd_submitf(cmd, "%s;%s;%lu", host_name, service_desc, start_time);
			break;

		case CMD_DISABLE_NOTIFICATIONS:
		case CMD_ENABLE_NOTIFICATIONS:
		case CMD_SHUTDOWN_PROCESS:
		case CMD_RESTART_PROCESS:
			result = cmd_submitf(cmd, "%lu", scheduled_time);
			break;

		case CMD_ENABLE_HOST_SVC_CHECKS:
		case CMD_DISABLE_HOST_SVC_CHECKS:
			result = cmd_submitf(cmd, "%s", host_name);
			if(affect_host_and_services == TRUE) {
				cmd = (cmd == CMD_ENABLE_HOST_SVC_CHECKS) ? CMD_ENABLE_HOST_CHECK : CMD_DISABLE_HOST_CHECK;
				result |= cmd_submitf(cmd, "%s", host_name);
				}
			break;

		case CMD_SCHEDULE_HOST_SVC_CHECKS:
			if(force_check == TRUE)
				cmd = CMD_SCHEDULE_FORCED_HOST_SVC_CHECKS;
			result = cmd_submitf(cmd, "%s;%lu", host_name, scheduled_time);
			break;

		case CMD_ENABLE_HOST_NOTIFICATIONS:
		case CMD_DISABLE_HOST_NOTIFICATIONS:
			if(propagate_to_children == TRUE)
				cmd = (cmd == CMD_ENABLE_HOST_NOTIFICATIONS) ? CMD_ENABLE_HOST_AND_CHILD_NOTIFICATIONS : CMD_DISABLE_HOST_AND_CHILD_NOTIFICATIONS;
			result = cmd_submitf(cmd, "%s", host_name);
			break;

		case CMD_ENABLE_HOST_SVC_NOTIFICATIONS:
		case CMD_DISABLE_HOST_SVC_NOTIFICATIONS:
			result = cmd_submitf(cmd, "%s", host_name);
			if(affect_host_and_services == TRUE) {
				cmd = (cmd == CMD_ENABLE_HOST_SVC_NOTIFICATIONS) ? CMD_ENABLE_HOST_NOTIFICATIONS : CMD_DISABLE_HOST_NOTIFICATIONS;
				result |= cmd_submitf(cmd, "%s", host_name);
				}
			break;

		case CMD_ACKNOWLEDGE_HOST_PROBLEM:
			result = cmd_submitf(cmd, "%s;%d;%d;%d", host_name, (sticky_ack == TRUE) ? ACKNOWLEDGEMENT_STICKY : ACKNOWLEDGEMENT_NORMAL, send_notification, persistent_comment);
			break;

		case CMD_ACKNOWLEDGE_SVC_PROBLEM:
			result = cmd_submitf(cmd, "%s;%s;%d;%d;%d", host_name, service_desc, (sticky_ack == TRUE) ? ACKNOWLEDGEMENT_STICKY : ACKNOWLEDGEMENT_NORMAL, send_notification, persistent_comment);
			break;

		case CMD_PROCESS_SERVICE_CHECK_RESULT:
			result = cmd_submitf(cmd, "%s;%s;%d;%s|%s", host_name, service_desc, plugin_state, plugin_output, performance_data);
			break;

		case CMD_PROCESS_HOST_CHECK_RESULT:
			result = cmd_submitf(cmd, "%s;%d;%s|%s", host_name, plugin_state, plugin_output, performance_data);
			break;

		case CMD_SCHEDULE_HOST_DOWNTIME:
			if(child_options == 1)
				cmd = CMD_SCHEDULE_AND_PROPAGATE_TRIGGERED_HOST_DOWNTIME;
			else if(child_options == 2)
				cmd = CMD_SCHEDULE_AND_PROPAGATE_HOST_DOWNTIME;

			result = cmd_submitf(cmd, "%s;%lu;%lu;%d;%lu;%lu", host_name, start_time, end_time, fixed, triggered_by, duration);
			break;

		case CMD_SCHEDULE_HOST_SVC_DOWNTIME:
			result = cmd_submitf(cmd, "%s;%lu;%lu;%d;%lu;%lu", host_name, start_time, end_time, fixed, triggered_by, duration);
			break;

		case CMD_SCHEDULE_SVC_DOWNTIME:
			result = cmd_submitf(cmd, "%s;%s;%lu;%lu;%d;%lu;%lu", host_name, service_desc, start_time, end_time, fixed, triggered_by, duration);
			break;

		case CMD_DEL_HOST_DOWNTIME:
		case CMD_DEL_SVC_DOWNTIME:
			result = cmd_submitf(cmd, "%lu", downtime_id);
			break;

		case CMD_SCHEDULE_HOST_CHECK:
			if(force_check == TRUE)
				cmd = CMD_SCHEDULE_FORCED_HOST_CHECK;
			result = cmd_submitf(cmd, "%s;%lu", host_name, start_time);
			break;

		case CMD_SEND_CUSTOM_HOST_NOTIFICATION:
			result = cmd_submitf(cmd, "%s;%d", host_name, (force_notification | broadcast_notification));
			break;

		case CMD_SEND_CUSTOM_SVC_NOTIFICATION:
			result = cmd_submitf(cmd, "%s;%s;%d", host_name, service_desc, (force_notification | broadcast_notification));
			break;


			/***** HOSTGROUP COMMANDS *****/

		case CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS:
		case CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS:
			result = cmd_submitf(cmd, "%s", hostgroup_name);
			if(affect_host_and_services == TRUE) {
				cmd = (cmd == CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS) ? CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS : CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS;
				result |= cmd_submitf(cmd, "%s", hostgroup_name);
				}
			break;

		case CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS:
		case CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS:
			result = cmd_submitf(cmd, "%s", hostgroup_name);
			break;

		case CMD_ENABLE_HOSTGROUP_SVC_CHECKS:
		case CMD_DISABLE_HOSTGROUP_SVC_CHECKS:
			result = cmd_submitf(cmd, "%s", hostgroup_name);
			if(affect_host_and_services == TRUE) {
				cmd = (cmd == CMD_ENABLE_HOSTGROUP_SVC_CHECKS) ? CMD_ENABLE_HOSTGROUP_HOST_CHECKS : CMD_DISABLE_HOSTGROUP_HOST_CHECKS;
				result |= cmd_submitf(cmd, "%s", hostgroup_name);
				}
			break;

		case CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME:
			result = cmd_submitf(cmd, "%s;%lu;%lu;%d;0;%lu", hostgroup_name, start_time, end_time, fixed, duration);
			break;

		case CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME:
			result = cmd_submitf(cmd, "%s;%lu;%lu;%d;0;%lu", hostgroup_name, start_time, end_time, fixed, duration);
			if(affect_host_and_services == TRUE)
				result |= cmd_submitf(CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME, "%s;%lu;%lu;%d;0;%lu", hostgroup_name, start_time, end_time, fixed, duration);
			break;


			/***** SERVICEGROUP COMMANDS *****/

		case CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
		case CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
			result = cmd_submitf(cmd, "%s", servicegroup_name);
			if(affect_host_and_services == TRUE) {
				cmd = (cmd == CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS) ? CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS : CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS;
				result |= cmd_submitf(cmd, "%s", servicegroup_name);
				}
			break;

		case CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
		case CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
			result = cmd_submitf(cmd, "%s", servicegroup_name);
			break;

		case CMD_ENABLE_SERVICEGROUP_SVC_CHECKS:
		case CMD_DISABLE_SERVICEGROUP_SVC_CHECKS:
			result = cmd_submitf(cmd, "%s", servicegroup_name);
			if(affect_host_and_services == TRUE) {
				cmd = (cmd == CMD_ENABLE_SERVICEGROUP_SVC_CHECKS) ? CMD_ENABLE_SERVICEGROUP_HOST_CHECKS : CMD_DISABLE_SERVICEGROUP_HOST_CHECKS;
				result |= cmd_submitf(cmd, "%s", servicegroup_name);
				}
			break;

		case CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME:
			result = cmd_submitf(cmd, "%s;%lu;%lu;%d;0;%lu", servicegroup_name, start_time, end_time, fixed, duration);
			break;

		case CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME:
			result = cmd_submitf(cmd, "%s;%lu;%lu;%d;0;%lu", servicegroup_name, start_time, end_time, fixed, duration);
			if(affect_host_and_services == TRUE)
				result |= cmd_submitf(CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME, "%s;%lu;%lu;%d;0;%lu", servicegroup_name, start_time, end_time, fixed, duration);
			break;

		default:
			return ERROR;
			break;
		}

	return result;
	}



/* write a command entry to the command file */
int write_command_to_file(char *cmd) {
	FILE *fp;
	struct stat statbuf;

	/*
	 * Commands are not allowed to have newlines in them, as
	 * that allows malicious users to hand-craft requests that
	 * bypass the access-restrictions.
	 */
	if(!cmd || !*cmd || strchr(cmd, '\n'))
		return ERROR;

	/* bail out if the external command file doesn't exist */
	if(stat(command_file, &statbuf)) {

		if(content_type == WML_CONTENT)
			printf("<p>エラー: 外部コマンドファイルをstat()できません。</p>\n");
		else {
			printf("<P><DIV CLASS='errorMessage'>エラー: 外部コマンドファイル'%s'をstat()できません。</DIV></P>\n", command_file);
			printf("<P><DIV CLASS='errorDescription'>");
			printf("外部コマンドファイルが無いようです。Nagiosが動いてないか、Nagiosが外部コマンドをチェックできない状態の可能性があります。\n");
			printf("</DIV></P>\n");
			}

		return ERROR;
		}

	/* open the command for writing (since this is a pipe, it will really be appended) */
	fp = fopen(command_file, "w");
	if(fp == NULL) {

		if(content_type == WML_CONTENT)
			printf("<p>エラー: コマンドファイルをアップデートできません。</p>\n");
		else {
			printf("<P><DIV CLASS='errorMessage'>エラー: コマンドファイル'%s'をアップデートできません。</DIV></P>\n", command_file);
			printf("<P><DIV CLASS='errorDescription'>");
			printf("外部コマンドファイルもしくは、ディレクトリのパーミッションに誤りがある可能性があります。パーミッションが適切かどうか確認してください。\n");
			printf("</DIV></P>\n");
			}

		return ERROR;
		}

	/* write the command to file */
	fprintf(fp, "%s\n", cmd);

	/* flush buffer */
	fflush(fp);

	fclose(fp);

	return OK;
	}


/* strips out semicolons from comment data */
void clean_comment_data(char *buffer) {
	int x;
	int y;

	if (!buffer || !*buffer)
		return;

	y = (int)strlen(buffer);

	for(x = 0; x < y; x++) {
		if(buffer[x] == ';')
			buffer[x] = ' ';
		}

	return;
	}


/* display information about a command */
void show_command_help(int cmd) {

	printf("<DIV ALIGN=CENTER CLASS='descriptionTitle'>コマンドの説明</DIV>\n");
	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0 CLASS='commandDescription'>\n");
	printf("<TR><TD CLASS='commandDescription'>\n");

	/* decide what information to print out... */
	switch(cmd) {

		case CMD_ADD_HOST_COMMENT:
			printf("このコマンドはこのホストにコメントを追加します。仕事中に起こったホストに関する様々な問題を記録すると他の管理者と共有が行え便利です。\n");
			printf("「<b>再起動後も(コメントを)保持させる</b>」チェックボックスにチェックを入れないとNagiosが次に再起動したときにコメントは削除されます。\n");
			break;

		case CMD_ADD_SVC_COMMENT:
			printf("このコマンドはこのホストにコメントを追加します。仕事中に起こったホストに関する様々な問題を記録すると他の管理者と共有が行え便利です。\n");
			printf("「<b>再起動後も(コメントを)保持させる</b>」チェックボックスにチェックを入れないとNagiosが次に再起動したときにコメントは削除されます。<BR>\n");
			break;

		case CMD_DEL_HOST_COMMENT:
			printf("このコマンドはこのホストのコメントを削除します。\n");
			break;

		case CMD_DEL_SVC_COMMENT:
			printf("このコマンドはこのサービスのコメントを削除します。\n");
			break;

		case CMD_DELAY_HOST_NOTIFICATION:
			printf("このコマンドは障害を検出した時にこのホストの次回通知を遅らせます。ただし、遅らせた通知が作動する前にホストの状態が変化した場合は無視されます。\n");
			printf("また、ホストが稼働状態の場合はこのコマンドは効果がありません。\n");
			break;

		case CMD_DELAY_SVC_NOTIFICATION:
			printf("このコマンドは障害を検出した時にこのサービスの次回通知を遅らせます。ただし、遅らせた通知が作動する前にホストの状態が変化した場合は無視されます。\n");
			printf("また、ホストが稼働状態の場合はこのコマンドは効果がありません。\n");
			break;

		case CMD_SCHEDULE_SVC_CHECK:
			printf("このコマンドは指定したサービスを次回からチェックするようにスケジュールするために使います。実行するとNagiosは指定する時にチェックされるサービスをキューに入れます。\n");
			printf("また、「<b>強制的にチェック</b>」にチェックを入れるとNagiosは他にチェックがスケジュールされていない場合やこのサービスのサービスチェックが無効になっていた場合であっても強制的にチェックを行います。パッシブサービスチェックのみのサービスであってもアクティブチェックされていますので注意してください。\n");
			break;

		case CMD_ENABLE_SVC_CHECK:
			printf("このコマンドはアクティブなサービスのチェックを有効にするために使います。\n");
			break;

		case CMD_DISABLE_SVC_CHECK:
			printf("このコマンドはアクティブなサービスのチェックを無効にするために使います。\n");
			break;

		case CMD_DISABLE_NOTIFICATIONS:
			printf("このコマンドはホストとサービスの通知機能をグローバル設定として無効にします。\n");
			break;

		case CMD_ENABLE_NOTIFICATIONS:
			printf("このコマンドはホストとサービスの通知機能をグローバル設定として有効にします。\n");
			break;

		case CMD_SHUTDOWN_PROCESS:
			printf("このコマンドはNagiosを停止させます。注)一度このコマンドでNagiosを停止させると、WebインターフェースからNagiosを起動することはできないので注意してください。\n");
			break;

		case CMD_RESTART_PROCESS:
			printf("このコマンドはNagiosを再起動させます。再起動コマンドはプロセスにHUPシグナルのを送るのと同じです。\n");
			printf("メモリ上の全情報のクリア、設定ファイルを再読込したあとNagiosはモニタリングを開始します\n");
			break;

		case CMD_ENABLE_HOST_SVC_CHECKS:
			printf("このコマンドはこのホストの全サービスのチェックを有効にします。ただし、<B>ホストチェックは有効になりません</B>。ホストのチェックを有効にしたい場合は、「同時にホストも有効にする」をチェックします。\n");
			break;

		case CMD_DISABLE_HOST_SVC_CHECKS:
			printf("このコマンドはこのホストの全サービスのチェックを無効にします。サービスを無効にするとNagiosはモニターしなくなります。これは障害が起こっているときに通知を送るのを抑制することができます。再度サービスをチェックを行うには「有効」にすることで出来ます。ただし、<B>ホストチェックは無効になりません</B>。ホストのチェックも無効にしたい場合は、「同時にホストチェックも無効にする」をチェックします。\n");
			break;

		case CMD_SCHEDULE_HOST_SVC_CHECKS:
			printf("このコマンドは指定されたホスト上の全てのサービスを次回からチェックするようにスケジュールするために使います。「<b>強制的にチェック</b>」にチェックを入れるとNagiosは他にチェックがスケジュールされていても、このサービスのサービスチェックが無効になっていたとしても強制的にチェックを行います。パッシブサービスチェックのみのサービスであってもアクティブチェックされてますので注意してください。\n");
			break;

		case CMD_DEL_ALL_HOST_COMMENTS:
			printf("このコマンドは指定したホストの全コメントを削除します。\n");
			break;

		case CMD_DEL_ALL_SVC_COMMENTS:
			printf("このコマンドは指定したサービスの全コメントを削除します。\n");
			break;

		case CMD_ENABLE_SVC_NOTIFICATIONS:
			printf("このコマンドは指定したサービスの通知機能を有効にします。通知は定義したサービス情報の状態が変わった際に送られます。\n");
			break;

		case CMD_DISABLE_SVC_NOTIFICATIONS:
			printf("このコマンドは指定したサービスの通知機能を無効にします。通知機能は再度有効にするまで一切機能しません。\n");
			break;

		case CMD_ENABLE_HOST_NOTIFICATIONS:
			printf("このコマンドは指定したホストの通知機能を有効にします。通知は定義したホスト情報の状態が変わった際に送られます。\n");
			printf("注)このコマンドではホストに付随するサービスの通知機能までは<B>有効になりません</B>。\n");
			break;

		case CMD_DISABLE_HOST_NOTIFICATIONS:
			printf("このコマンドは指定したホストの通知機能を無効にします。通知機能は再度有効にするまで一切機能しません。\n");
			printf("注)このコマンドではホストに付随するサービスの通知機能までは<B>無効になりません</B>。\n");
			break;

		case CMD_ENABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
			printf("このコマンドは指定したホストの配下にある全てのホストとサービスの通知機能を有効にします。\n");
			break;

		case CMD_DISABLE_ALL_NOTIFICATIONS_BEYOND_HOST:
			printf("このコマンドは指定したホストの配下にある全てのホストとサービスの通知機能を無効にします。\n");
			break;

		case CMD_ENABLE_HOST_SVC_NOTIFICATIONS:
			printf("このコマンドは指定したホストの全サービスの通知機能を有効にします。通知は定義したサービス状態が変化した際に送られます。\n");
			printf("このコマンドではホストの通知機能は<b>有効になりません</B>。ホストも同様に通知機能を有効にしたい場合は「<B>同時にホストも有効にする</B>」をチェックします。\n");
			break;

		case CMD_DISABLE_HOST_SVC_NOTIFICATIONS:
			printf("このコマンドは指定したホストの全サービスの通知機能を無効にします。通知機能は再度有効にするまで機能しません。\n");
			printf("このコマンドではホストの通知機能は<b>無効になりません</B>。ホストも同様に通知機能を無効にしたい場合は「<B>同時にホストも無効にする</B>」をチェックします。\n");
			break;

		case CMD_ACKNOWLEDGE_HOST_PROBLEM:
			printf("このコマンドは指定したホストの問題を既知としてマークします。ホストの問題が認知済としてマークされたら、今後この問題に関して状態が変わるまで(たとえば復旧するなど)通知を行わなくなります。\n");
			printf("このコマンドが実行されるとホストの通知先に「問題を認知した」という通知が行われます。加えてコメントも追加されます。\n");
			printf("名前とちょっとした説明を入れてください。もしこの問題を認知してもコメントを残したい場合は「保存する」にチェックを入れます。もし、この問題を認知した警報を通知したくなければ「警報を通知」のチェックをはずしてください。\n");
			break;

		case CMD_ACKNOWLEDGE_SVC_PROBLEM:
			printf("このコマンドは指定したサービスの問題を既知としてマークします。ホストの問題が認知済としてマークされたら、今後この問題に関して状態が変わるまで(たとえば復旧するなど)通知を行わなくなります。\n");
			printf("このコマンドが実行されるとサービスの通知先に「問題を認知した」という通知が行われます。加えてコメントも追加されます。\n");
			printf("名前とちょっとした説明を入れてください。もしこの問題を認知してもコメントを残したい場合は「保存する」にチェックを入れます。もし、この問題を認知した警報を通知したくなければ「警報を通知」のチェックをはずしてください。\n");
			break;

		case CMD_START_EXECUTING_SVC_CHECKS:
			printf("このコマンドはグローバル設定としてサービスチェックを再開します。ただし、個々のサービスでサービスチェックが無効になっているものはそのまま無効になります。\n");
			break;

		case CMD_STOP_EXECUTING_SVC_CHECKS:
			printf("このコマンドは一時的に全てのサービスチェックを停止します。これによりどんな通知も行わなくなります。\n");
			printf("サービスチェックはサービスチェックを再開するコマンドを発行するまで行われません。\n");
			break;

		case CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS:
			printf("このコマンドはNagiosが外部コマンドからパッシブサービスチェックの結果を受け付けるようにします。\n");
			break;

		case CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS:
			printf("このコマンドはNagiosが外部コマンドからパッシブサービスチェックの結果を受け付ないようにします。\n");
			break;

		case CMD_ENABLE_PASSIVE_SVC_CHECKS:
			printf("このコマンドはNagiosが外部コマンドから指定したサービスについてのパッシブサービスチェックの結果を受け付けるようにします。\n");
			break;

		case CMD_DISABLE_PASSIVE_SVC_CHECKS:
			printf("このコマンドはNagiosが外部コマンドから指定したサービスについてのパッシブサービスチェックの結果を受け付けないようにします。\n");
			break;

		case CMD_ENABLE_EVENT_HANDLERS:
			printf("このコマンドはホスト、サービスのイベントハンドラを有効にします。\n");
			break;

		case CMD_DISABLE_EVENT_HANDLERS:
			printf("このコマンドはホスト、サービスのイベントハンドラを無効にします。\n");
			break;

		case CMD_ENABLE_SVC_EVENT_HANDLER:
			printf("このコマンドは指定したサービスのイベントハンドラを有効にします。\n");
			break;

		case CMD_DISABLE_SVC_EVENT_HANDLER:
			printf("このコマンドは指定したサービスのイベントハンドラを無効にします。\n");
			break;

		case CMD_ENABLE_HOST_EVENT_HANDLER:
			printf("このコマンドは指定したホストのイベントハンドラを有効にします。\n");
			break;

		case CMD_DISABLE_HOST_EVENT_HANDLER:
			printf("このコマンドは指定したホストのイベントハンドラを無効にします。\n");
			break;

		case CMD_ENABLE_HOST_CHECK:
			printf("このコマンドはホストチェックを有効にします。\n");
			break;

		case CMD_DISABLE_HOST_CHECK:
			printf("このコマンドはホストチェックを無効にします。もしNagiosがこのホストのチェックデータが必要となったら、無効にする前の状態を現在の状態とみなします。\n");
			break;

		case CMD_START_OBSESSING_OVER_SVC_CHECKS:
			printf("このコマンドはNagiosにObsessing overサービスチェックを開始させます。この機能についてはドキュメントを参照してください。\n");
			break;

		case CMD_STOP_OBSESSING_OVER_SVC_CHECKS:
			printf("このコマンドはNagiosにObsessing overサービスチェックを停止させます。この機能についてはドキュメントを参照してください。\n");
			break;

		case CMD_REMOVE_HOST_ACKNOWLEDGEMENT:
			printf("このコマンドは指定したホストの認知済マークを削除し、通知を再開します。\n");
			break;

		case CMD_REMOVE_SVC_ACKNOWLEDGEMENT:
			printf("このコマンドは指定したサービスの認知済マークを削除し、通知を再開します。\n");
			break;

		case CMD_PROCESS_SERVICE_CHECK_RESULT:
			printf("このコマンドは指定したサービスからのPassiveチェックの結果を送信します。これは作業が行われたり、作業を完了したり、セキュリティチェックなどに有効活用できます。\n");
			break;

		case CMD_PROCESS_HOST_CHECK_RESULT:
			printf("このコマンドは指定したホストのPassiveチェックの結果を送信します。\n");
			break;

		case CMD_SCHEDULE_HOST_DOWNTIME:
			printf("このコマンドは指定したホストのダウンタイムをスケジュールします。ダウンタイムを設定するとその間はNagiosはホストに関する通知を行わなくなります。\n");
			printf("ダウンタイムが経過したらNagiosは通常どおり通知を行います。このスケジュール内容はNagiosが再起動した場合でも保存されます。\n");
			printf("フィールドにダウンタイム開始時間と終了時間を<b>mm/dd/yyyy hh:mm:ss</b>形式で入力してください。\n");
			printf("もし「<B>固定</B>」にチェックを入れると入力した開始時間と終了時間きっちりにスケジュールされます。もし「<B>固定</b>」にチェックを入れない場合はNagiosは\"フレキシブル\"なダウンタイムとします。\n");
			printf("フレキシブルなダウンタイムとはホストが停止または未到達になる開始時間から経過時間を指定してダウンタイムを決定することです。「<B>固定</B>」にチェックを入れた場合<b>期間</b>を指定する箇所は入力しても無効になります。\n");
			break;

		case CMD_SCHEDULE_HOST_SVC_DOWNTIME:
			printf("このコマンドは指定したホストとその全サービスのダウンタイムをスケジュールします。ダウンタイムを設定するとその間はNagiosはホストに関する通知を行わなくなります。\n");
			printf("ダウンタイムが経過したらNagiosは通常どおり通知を行います。このスケジュール内容はNagiosが再起動した場合でも保存されます。\n");
			printf("フィールドにダウンタイム開始時間と終了時間を<b>mm/dd/yyyy hh:mm:ss</b>形式で入力してください。\n");
			printf("もし「<B>固定</B>」にチェックを入れると入力した開始時間と終了時間きっちりにスケジュールされます。もし「<B>固定</b>」にチェックを入れない場合はNagiosは\"フレキシブル\"なダウンタイムとします。\n");
			printf("フレキシブルなダウンタイムとはホストが停止または未到達になる開始時間から経過時間を指定してダウンタイムを決定することです。「<B>固定</B>」にチェックを入れた場合<b>期間</b>を指定する箇所は入力しても無効になります。\n");
			break;

		case CMD_SCHEDULE_SVC_DOWNTIME:
			printf("このコマンドは指定したサービスのダウンタイムをスケジュールします。ダウンタイムを設定するとその間はNagiosはサービスに関する通知を行わなくなります。\n");
			printf("ダウンタイムが経過したらNagiosは通常どおり通知を行います。このスケジュール内容はNagiosが再起動した場合でも保存されます。\n");
			printf("フィールドにダウンタイム開始時間と終了時間を<b>mm/dd/yyyy hh:mm:ss</b>形式で入力してください。");
			printf("もし「<B>固定</B>」にチェックを入れると入力した開始時間と終了時間きっちりにスケジュールされます。もし「<B>固定</b>」にチェックを入れない場合はNagiosは\"フレキシブル\"なダウンタイムとします。\n");
			printf("フレキシブルなダウンタイムとはサービスが停止または未到達になる開始時間から経過時間を指定してダウンタイムを決定することです。「<B>固定</B>」にチェックを入れた場合<b>期間</b>を指定する箇所は入力しても無効になります。</b>\n");
			break;

		case CMD_ENABLE_HOST_FLAP_DETECTION:
			printf("このコマンドは指定したホストのフラップ検知を有効にします。ただし、フラップ検知がグローバル設定で無効に設定されている場合はこのコマンドは何の効果も持ちません。\n");
			break;

		case CMD_DISABLE_HOST_FLAP_DETECTION:
			printf("このコマンドは指定したホストのフラップ検知を無効にします。\n");
			break;

		case CMD_ENABLE_SVC_FLAP_DETECTION:
			printf("このコマンドは指定したサービスのフラップ検知を有効にします。ただし、フラップ検知がグローバル設定で無効に設定されている場合はこのコマンドは何の効果も持ちません。\n");
			break;

		case CMD_DISABLE_SVC_FLAP_DETECTION:
			printf("このコマンドは指定したサービスのフラップ検知を無効にします。\n");
			break;

		case CMD_ENABLE_FLAP_DETECTION:
			printf("このコマンドはグローバル設定としてフラップ検知を有効にします。個々のホストやサービスで無効に設定されている場合、それらは保持されたままになります。\n");
			break;

		case CMD_DISABLE_FLAP_DETECTION:
			printf("このコマンドはグローバル設定としてフラップ検知を無効にします。\n");
			break;

		case CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS:
			printf("このコマンドは指定したホストグループの全サービスの通知を有効にします。通知はサービス定義で定義したサービスのみ送られます。\n");
			printf("このコマンドでは指定したホストグループ上のホストの通知機能は<b>有効になりません</b>。もし有効にしたい場合は「<B>同時にホストも有効にする</b>」をチェックします。\n");
			break;

		case CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS:
			printf("このコマンドは指定したホストグループの全サービスの通知を無効にします。通知はサービス定義で定義したサービスのみ送られます。\n");
			printf("このコマンドでは指定したホストグループ上のホストの通知機能は<b>無効になりません</b>。もし無効にしたい場合は「<B>同時にホストも無効にする</b>」をチェックします。\n");
			break;

		case CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS:
			printf("このコマンドは指定したホストグループ上の全ホストの通知を有効にします。\n");
			printf("通知はホスト定義で定義したホストのみに送られます。\n");
			break;

		case CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS:
			printf("このコマンドは指定したホストグループ上の全ホストの通知を無効にします。\n");
			printf("再度このホストグループ上の全ホストに通知を有効にするまでどんな警報も通知されません。\n");
			break;

		case CMD_ENABLE_HOSTGROUP_SVC_CHECKS:
			printf("このコマンドは指定したホストグループ上のサービスチェックを有効にします。\n");
			printf("ただし、ホストチェックまではこのコマンドでは有効になりません。有効にしたい場合は「<b>同時にホストも有効にする</b>」をチェックします。\n");
			break;

		case CMD_DISABLE_HOSTGROUP_SVC_CHECKS:
			printf("このコマンドは指定したホストグループ上のサービスチェックを無効にします。このコマンドではホストグループのホストチェックは<b>無効になりません</b>。無効にしたい場合は「<B>同時>にホストも無効にする</B>」をチェックします。\n");
			break;

		case CMD_DEL_HOST_DOWNTIME:
			printf("このコマンドは指定したホストのダウンタイムをキャンセルもしくは保留にします。\n");
			break;

		case CMD_DEL_SVC_DOWNTIME:
			printf("このコマンドは指定したサービスのダウンタイムをキャンセルもしくは保留にします。\n");
			break;

		case CMD_ENABLE_PERFORMANCE_DATA:
			printf("このコマンドはグローバル設定としてホストとサービスのパフォーマンスデータを有効にします。\n");
			printf("個々のホストやサービスが無効になっている場合は無効のままになります。\n");
			break;

		case CMD_DISABLE_PERFORMANCE_DATA:
			printf("このコマンドはグローバル設定としてホストとサービスのパフォーマンスデータを無効にします。\n");
			break;

		case CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME:
			printf("このコマンドは指定したホストグループ上の全ホストのダウンタイムをスケジュールします。ダウンタイムを設定するとその間はNagiosはホストに関する通知を行わなくなります。\n");
			printf("ダウンタイムが経過したらNagiosは通常どおり通知を行います。このスケジュール内容はNagiosが再起動した場合でも保存されます。\n");
			printf("フィールドにダウンタイム開始時間と終了時間を<b>mm/dd/yyyy hh:mm:ss</b>形式で入力してください。\n");
			printf("もし「<B>固定</B>」にチェックを入れると入力した開始時間と終了時間きっちりにスケジュールされます。もし「<B>固定</b>」にチェックを入れない場合はNagiosは\"フレキシブル\"なダウンタイムとします。\n");
			printf("フレキシブルなダウンタイムとはホストが停止または未到達になる開始時間から経過時間を指定してダウンタイムを決定することです。「<B>固定</B>」にチェックを入れた場合<b>期間</b>を指定する箇所は入力しても無効になります。</b>\n");
			break;

		case CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME:
			printf("このコマンドは指定したホストグループ上の全サービスのダウンタイムをスケジュールします。ダウンタイムを設定するとその間はNagiosはサービスに関する通知を行わなくなります。\n");
			printf("ダウンタイムが経過したらNagiosは通常どおり通知を行います。このスケジュール内容はNagiosが再起動した場合でも保存されます。\n");
			printf("フィールドにダウンタイム開始時間と終了時間を<b>mm/dd/yyyy hh:mm:ss</b>形式で入力してください。\n");
			printf("もし「<B>固定</B>」にチェックを入れると入力した開始時間と終了時間きっちりにスケジュールされます。もし<「B>固定</b>」にチェックを入れない場合はNagiosは\"フレキシブル\"なダウンタイムとします。\n");
			printf("フレキシブルなダウンタイムとはホストが停止または未到達になる開始時間から経過時間を指定してダウンタイムを決定することです。「<B>固定</B>」にチェックを入れた場合<b>期間</b>を指定する箇所は入力しても無効になります。</b>\n");
			break;

		case CMD_START_EXECUTING_HOST_CHECKS:
			printf("このコマンドはグローバル設定としてホストチェックを有効にするために使います。\n");
			break;

		case CMD_STOP_EXECUTING_HOST_CHECKS:
			printf("このコマンドはグローバル設定としてホストチェックを無効にするために使います。\n");
			break;

		case CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS:
			printf("このコマンドはNagiosがObsessing Overホストチェックを開始するために使います。詳細な情報はドキュメントの分散モニタリング(distributed monitoring)の部分を参照してください。\n");
			break;

		case CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS:
			printf("このコマンドはNagiosがObsessing Overホストチェックを停止するために使います。\n");
			break;

		case CMD_ENABLE_PASSIVE_HOST_CHECKS:
			printf("このコマンドはNagiosが指定したホストの外部コマンドからパッシブチェックを行うことを許可するために使用します。\n");
			break;

		case CMD_DISABLE_PASSIVE_HOST_CHECKS:
			printf("このコマンドはNagiosが指定したホストの外部コマンドからパッシブチェックを行うことを拒否するために使用します。これを実行すると指定したホストからの全てのPassiveチェックが無視されます。\n");
			break;

		case CMD_START_OBSESSING_OVER_HOST_CHECKS:
			printf("このコマンドはobsessing overホストチェックを開始するために使います。詳細な情報はドキュメントの分散モニタリング(distributed monitoring)の部分を参照してください。\n");
			break;

		case CMD_STOP_OBSESSING_OVER_HOST_CHECKS:
			printf("このコマンドはobsessing overホストチェックを停止するために使います。\n");
			break;

		case CMD_SCHEDULE_HOST_CHECK:
			printf("このコマンドは指定したホストを次回からチェックするようにスケジュールするために使います。実行するとNagiosは指定する時にチェックされるサービスをキューに入れます。\n");
			printf("また、「<b>強制的にチェック</b>」にチェックを入れるとNagiosは他にチェックがスケジュールされていない場合やこのホストのホストチェックが無効になっていた場合であっても強制的にチェックを行います。\n");
			break;

		case CMD_START_OBSESSING_OVER_SVC:
			printf("このコマンドは指定したサービスのobsessing overを開始するために使います。\n");
			break;

		case CMD_STOP_OBSESSING_OVER_SVC:
			printf("このコマンドは指定したサービスのobsessing overを停止するために使います。\n");
			break;

		case CMD_START_OBSESSING_OVER_HOST:
			printf("このコマンドは指定したホストのobsessing overを開始するために使います。\n");
			break;

		case CMD_STOP_OBSESSING_OVER_HOST:
			printf("このコマンドは指定したホストのobsessing overを停止するために使います。\n");
			break;

		case CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
			printf("このコマンドは指定したサービスグループの全サービスの通知を有効にします。通知はサービス定義で定義したサービスのみ送られます。\n");
			printf("このコマンドでは指定したサービスグループ上のホストの通知機能は<b>有効になりません</b>。もし有効にしたい場合は「<B>同時にホストも有効にする</b>」をチェックします。\n");
			break;

		case CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS:
			printf("このコマンドは指定したサービスグループの全サービスの通知を無効にします。通知はサービス定義で定義したサービスのみ送られます。\n");
			printf("このコマンドでは指定したサービスグループ上のホストの通知機能は<b>無効になりません</b>。もし有効にしたい場合は「<B>同時にホストも無効にする</b>」をチェックします。\n");
			break;

		case CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
			printf("このコマンドは指定したサービスグループ上の全ホストの通知を有効にします。\n");
			printf("通知はホスト定義で定義したホストのみに送られます。\n");
			break;

		case CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS:
			printf("このコマンドは指定したサービスグループ上の全ホストの通知を無効にします。\n");
			printf("再度このサービスグループ上の全ホストに通知を有効にするまでどんな警報も通知されません。\n");
			break;

		case CMD_ENABLE_SERVICEGROUP_SVC_CHECKS:
			printf("このコマンドは指定したサービスグループ上のサービスチェックを有効にします。このコマンドではサービスグループのホストチェックは<b>有効になりません</b>。有効にしたい場合は「<B>同時にホストも有効にする</B>」にチェックを入れてください。\n");
			break;

		case CMD_DISABLE_SERVICEGROUP_SVC_CHECKS:
			printf("このコマンドは指定したサービスグループ上のサービスチェックを無効にします。このコマンドではサービスグループのホストチェックは<b>無効になりません</b>。無効にしたい場合は「<B>同時にホストも無効にする</B>」にチェックを入れてください。\n");
			break;

		case CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME:
			printf("このコマンドは指定したサービスグループ上の全ホストのダウンタイムをスケジュールします。ダウンタイムを設定するとその間はNagiosはホストに関する通知を行わなくなります。\n");
			printf("ダウンタイムが経過したらNagiosは通常どおり通知を行います。このスケジュール内容はNagiosが再起動した場合でも保存されます\n");
			printf("フィールドにダウンタイム開始時間と終了時間を<b>mm/dd/yyyy hh:mm:ss</b>形式で入力してください。\n");
			printf("もし「<B>固定</B>」にチェックを入れると入力した開始時間と終了時間きっちりにスケジュールされます。もし「<B>固定</B>」にチェックを入れない場合はNagiosは\"フレキシブル\"なダウンタイムとします。\n");
			printf("フレキシブルなダウンタイムとはホストが停止または未到達になる開始時間から経過時間を指定してダウンタイムを決定することです。「<B>固定</B>」にチェックを入れた場合<b>期間</b>を指定する箇所は入力しても無効になります。</b>\n");
			break;

		case CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME:
			printf("このコマンドは指定したサービスグループ上の全サービスのダウンタイムをスケジュールします。ダウンタイムを設定するとその間はNagiosはサービスに関する通知を行わなくなります。\n");
			printf("ダウンタイムが経過したらNagiosは通常どおり通知を行います。このスケジュール内容はNagiosが再起動した場合でも保存されます\n");
			printf("フィールドにダウンタイム開始時間と終了時間を<b>mm/dd/yyyy hh:mm:ss</b>形式で入力してください。\n");
			printf("もし「<B>固定</B>」にチェックを入れると入力した開始時間と終了時間きっちりにスケジュールされます。もし「<B>固定</B>」にチェックを入れない場合はNagiosは\"フレキシブル\"なダウンタイムとします。\n");
			printf("フレキシブルなダウンタイムとはホストが停止または未到達になる開始時間から経過時間を指定してダウンタイムを決定することです。「<B>固定</B>」にチェックを入れた場合<b>期間</b>を指定する箇所は入力しても無効になります。</b>\n");
			break;

		case CMD_CLEAR_HOST_FLAPPING_STATE:
		case CMD_CLEAR_SVC_FLAPPING_STATE:
			printf("このコマンドは指定された%sのフラッピング状態をリセットするために使用されます。\n",
				(cmd == CMD_CLEAR_HOST_FLAPPING_STATE) ? "ホスト" : "サービス");
			printf("指定されて%sのすべての状態履歴がクリアされます。\n",
				(cmd == CMD_CLEAR_HOST_FLAPPING_STATE) ? "ホスト" : "サービス");
			break;

		case CMD_SEND_CUSTOM_HOST_NOTIFICATION:
		case CMD_SEND_CUSTOM_SVC_NOTIFICATION:
			printf("このコマンドは%sに対してすぐに通知します。これは監視されているシステムあるいはサービスについて発生した問題を管理者側へ通知する必要がある緊急時において有用です。\n", (cmd == CMD_SEND_CUSTOM_HOST_NOTIFICATION) ? "ホスト" : "サービス");
			printf("この作業はNagiosで通常行われる通知作業の後に行われます。「強制的に通知する」を選択した場合、通知機能が有効になっているかや時間による制限が行われているかなどにかかわらず通知を行うようになります。「全員に通知」を選択すると全ての利用者に対して通知を行います。\n");
			printf("尚、これらの選択はあなたが重要なメッセージを受け取りたいと考えているならば通常の通知よりも優先しておく必要があります。\n");
			break;

		default:
			printf("このコマンドの情報はありません。");
		}

	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	return;
	}



/* converts a time string to a UNIX timestamp, respecting the date_format option */
int string_to_time(char *buffer, time_t *t) {
	struct tm lt;
	int ret = 0;


	/* Initialize some variables just in case they don't get parsed
	   by the sscanf() call.  A better solution is to also check the
	   CGI input for validity, but this should suffice to prevent
	   strange problems if the input is not valid.
	   Jan 15 2003  Steve Bonds */
	lt.tm_mon = 0;
	lt.tm_mday = 1;
	lt.tm_year = 1900;
	lt.tm_hour = 0;
	lt.tm_min = 0;
	lt.tm_sec = 0;
	lt.tm_wday = 0;
	lt.tm_yday = 0;


	if(date_format == DATE_FORMAT_EURO)
		ret = sscanf(buffer, "%02d-%02d-%04d %02d:%02d:%02d", &lt.tm_mday, &lt.tm_mon, &lt.tm_year, &lt.tm_hour, &lt.tm_min, &lt.tm_sec);
	else if(date_format == DATE_FORMAT_ISO8601 || date_format == DATE_FORMAT_STRICT_ISO8601)
		ret = sscanf(buffer, "%04d-%02d-%02d%*[ T]%02d:%02d:%02d", &lt.tm_year, &lt.tm_mon, &lt.tm_mday, &lt.tm_hour, &lt.tm_min, &lt.tm_sec);
	else
		ret = sscanf(buffer, "%02d-%02d-%04d %02d:%02d:%02d", &lt.tm_mon, &lt.tm_mday, &lt.tm_year, &lt.tm_hour, &lt.tm_min, &lt.tm_sec);

	if(ret != 6)
		return ERROR;

	lt.tm_mon--;
	lt.tm_year -= 1900;

	/* tell mktime() to try and compute DST automatically */
	lt.tm_isdst = -1;

	*t = mktime(&lt);

	return OK;
	}
