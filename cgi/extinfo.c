/**************************************************************************
 *
 * EXTINFO.C -  Nagios Extended Information CGI
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
#include "../include/macros.h"
#include "../include/comments.h"
#include "../include/downtime.h"
#include "../include/statusdata.h"

#include "../include/cgiutils.h"
#include "../include/getcgi.h"
#include "../include/cgiauth.h"

static nagios_macros *mac;

extern char             nagios_process_info[MAX_INPUT_BUFFER];
extern int              nagios_process_state;
extern int              refresh_rate;
extern int				enable_page_tour;

extern int              buffer_stats[1][3];
extern int              program_stats[MAX_CHECK_STATS_TYPES][3];

extern char main_config_file[MAX_FILENAME_LENGTH];
extern char url_html_path[MAX_FILENAME_LENGTH];
extern char url_stylesheets_path[MAX_FILENAME_LENGTH];
extern char url_docs_path[MAX_FILENAME_LENGTH];
extern char url_images_path[MAX_FILENAME_LENGTH];
extern char url_logo_images_path[MAX_FILENAME_LENGTH];
extern char url_js_path[MAX_FILENAME_LENGTH];

extern int              enable_splunk_integration;

extern char             *notes_url_target;
extern char             *action_url_target;

extern hoststatus *hoststatus_list;
extern servicestatus *servicestatus_list;


#define MAX_MESSAGE_BUFFER		4096

#define HEALTH_WARNING_PERCENTAGE       85
#define HEALTH_CRITICAL_PERCENTAGE      75

/* SORTDATA structure */
typedef struct sortdata_struct {
	int is_service;
	servicestatus *svcstatus;
	hoststatus *hststatus;
	struct sortdata_struct *next;
	} sortdata;

void document_header(int);
void document_footer(void);
int process_cgivars(void);

void show_process_info(void);
void show_host_info(void);
void show_service_info(void);
void show_all_comments(void);
void show_performance_data(void);
void show_hostgroup_info(void);
void show_servicegroup_info(void);
void show_all_downtime(void);
void show_scheduling_queue(void);
void display_comments(int);

int sort_data(int, int);
int compare_sortdata_entries(int, int, sortdata *, sortdata *);
void free_sortdata_list(void);

authdata current_authdata;

sortdata *sortdata_list = NULL;

char *host_name = "";
char *hostgroup_name = "";
char *servicegroup_name = "";
char *service_desc = "";

int display_type = DISPLAY_PROCESS_INFO;

int sort_type = SORT_ASCENDING;
int sort_option = SORT_NEXTCHECKTIME;

int embedded = FALSE;
int display_header = TRUE;



int main(void) {
	int found = FALSE;
	char temp_buffer[MAX_INPUT_BUFFER] = "";
	char *processed_string = NULL;
	host *temp_host = NULL;
	hostsmember *temp_parenthost = NULL;
	hostgroup *temp_hostgroup = NULL;
	service *temp_service = NULL;
	servicegroup *temp_servicegroup = NULL;

	mac = get_global_macros();

	/* get the arguments passed in the URL */
	process_cgivars();

	/* reset internal variables */
	reset_cgi_vars();

	cgi_init(document_header, document_footer, READ_ALL_OBJECT_DATA, READ_ALL_STATUS_DATA);

	/* initialize macros */
	init_macros();

	/* get authentication information */
	get_authentication_information(&current_authdata);

	document_header(TRUE);


	if(display_header == TRUE) {

		/* begin top table */
		printf("<table border=0 width=100%%>\n");
		printf("<tr>\n");

		/* left column of the first row */
		printf("<td align=left valign=top width=33%%>\n");

		if(display_type == DISPLAY_HOST_INFO)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "ホスト情報");
		else if(display_type == DISPLAY_SERVICE_INFO)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "サービス情報");
		else if(display_type == DISPLAY_COMMENTS)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "全てのホストとサービスのコメント");
		else if(display_type == DISPLAY_PERFORMANCE)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "パフォーマンス情報");
		else if(display_type == DISPLAY_HOSTGROUP_INFO)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "ホストグループ情報");
		else if(display_type == DISPLAY_SERVICEGROUP_INFO)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "サービスグループ情報");
		else if(display_type == DISPLAY_DOWNTIME)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "全てのホストとサービスのダウンタイムスケジュール");
		else if(display_type == DISPLAY_SCHEDULING_QUEUE)
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "スケジュールキューの確認");
		else
			snprintf(temp_buffer, sizeof(temp_buffer) - 1, "Nagiosプロセス情報");
		temp_buffer[sizeof(temp_buffer) - 1] = '\x0';
		display_info_table(temp_buffer, TRUE, &current_authdata);

		/* find the host */
		if(display_type == DISPLAY_HOST_INFO || display_type == DISPLAY_SERVICE_INFO) {

			temp_host = find_host(host_name);
			grab_host_macros_r(mac, temp_host);

			if(display_type == DISPLAY_SERVICE_INFO) {
				temp_service = find_service(host_name, service_desc);
				grab_service_macros_r(mac, temp_service);
				}

			/* write some Javascript helper functions */
			if(temp_host != NULL) {
				printf("<SCRIPT LANGUAGE=\"JavaScript\">\n<!--\n");
				printf("function nagios_get_host_name()\n{\n");
				printf("return \"%s\";\n", temp_host->name);
				printf("}\n");
				printf("function nagios_get_host_address()\n{\n");
				printf("return \"%s\";\n", temp_host->address);
				printf("}\n");
				if(temp_service != NULL) {
					printf("function nagios_get_service_description()\n{\n");
					printf("return \"%s\";\n", temp_service->description);
					printf("}\n");
					}
				printf("//-->\n</SCRIPT>\n");
				}
			}

		/* find the hostgroup */
		else if(display_type == DISPLAY_HOSTGROUP_INFO) {
			temp_hostgroup = find_hostgroup(hostgroup_name);
			grab_hostgroup_macros_r(mac, temp_hostgroup);
			}

		/* find the servicegroup */
		else if(display_type == DISPLAY_SERVICEGROUP_INFO) {
			temp_servicegroup = find_servicegroup(servicegroup_name);
			grab_servicegroup_macros_r(mac, temp_servicegroup);
			}

		if((display_type == DISPLAY_HOST_INFO && temp_host != NULL) || (display_type == DISPLAY_SERVICE_INFO && temp_host != NULL && temp_service != NULL) || (display_type == DISPLAY_HOSTGROUP_INFO && temp_hostgroup != NULL) || (display_type == DISPLAY_SERVICEGROUP_INFO && temp_servicegroup != NULL)) {
			printf("<TABLE BORDER=1 CELLPADDING=0 CELLSPACING=0 CLASS='linkBox'>\n");
			printf("<TR><TD CLASS='linkBox'>\n");
			if(display_type == DISPLAY_SERVICE_INFO)
				printf("<A HREF='%s?type=%d&host=%s'>このホストの情報</A><br>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(host_name));
			if(display_type == DISPLAY_SERVICE_INFO || display_type == DISPLAY_HOST_INFO)
				printf("<A HREF='%s?host=%s'>このホストの稼動状態</A><BR>\n", STATUS_CGI, url_encode(host_name));
			if(display_type == DISPLAY_HOST_INFO) {
				printf("<A HREF='%s?host=%s'>このホストの警報履歴</A><BR>\n", HISTORY_CGI, url_encode(host_name));
#ifdef USE_TRENDS
				printf("<A HREF='%s?host=%s'>このホストの傾向</A><BR>\n", TRENDS_CGI, url_encode(host_name));
#endif
#ifdef USE_HISTOGRAM
				printf("<A HREF='%s?host=%s'>このホストの警報ヒストグラム</A><BR>\n", HISTOGRAM_CGI, url_encode(host_name));
#endif
				printf("<A HREF='%s?host=%s&show_log_entries'>このホストの稼働率レポート</A><BR>\n", AVAIL_CGI, url_encode(host_name));
				printf("<A HREF='%s?host=%s'>このホストの通知履歴</A>\n", NOTIFICATIONS_CGI, url_encode(host_name));
				}
			else if(display_type == DISPLAY_SERVICE_INFO) {
				printf("<A HREF='%s?host=%s&", HISTORY_CGI, url_encode(host_name));
				printf("service=%s'>このサービスの警報履歴</A><BR>\n", url_encode(service_desc));
#ifdef USE_TRENDS
				printf("<A HREF='%s?host=%s&", TRENDS_CGI, url_encode(host_name));
				printf("service=%s'>このサービスの傾向</A><BR>\n", url_encode(service_desc));
#endif
#ifdef USE_HISTOGRAM
				printf("<A HREF='%s?host=%s&", HISTOGRAM_CGI, url_encode(host_name));
				printf("service=%s'>このサービスの警報ヒストグラム</A><BR>\n", url_encode(service_desc));
#endif
				printf("<A HREF='%s?host=%s&", AVAIL_CGI, url_encode(host_name));
				printf("service=%s&show_log_entries'>このサービスの稼動レポート</A><BR>\n", url_encode(service_desc));
				printf("<A HREF='%s?host=%s&", NOTIFICATIONS_CGI, url_encode(host_name));
				printf("service=%s'>このサービスの通知履歴</A>\n", url_encode(service_desc));
				}
			else if(display_type == DISPLAY_HOSTGROUP_INFO) {
				printf("<A HREF='%s?hostgroup=%s&style=detail'>このホストグループの稼動状態</A><BR>\n", STATUS_CGI, url_encode(hostgroup_name));
				printf("<A HREF='%s?hostgroup=%s&style=overview'>このホストグループのステータスオーバービュー</A><BR>\n", STATUS_CGI, url_encode(hostgroup_name));
				printf("<A HREF='%s?hostgroup=%s&style=grid'>このホストグループのステータスグリッド</A><BR>\n", STATUS_CGI, url_encode(hostgroup_name));
				printf("<A HREF='%s?hostgroup=%s'>このホストグループの稼動レポート</A><BR>\n", AVAIL_CGI, url_encode(hostgroup_name));
				}
			else if(display_type == DISPLAY_SERVICEGROUP_INFO) {
				printf("<A HREF='%s?servicegroup=%s&style=detail'>このサービスグループの稼動状態</A><BR>\n", STATUS_CGI, url_encode(servicegroup_name));
				printf("<A HREF='%s?servicegroup=%s&style=overview'>このサービスグループのステータスオーバービュー</A><BR>\n", STATUS_CGI, url_encode(servicegroup_name));
				printf("<A HREF='%s?servicegroup=%s&style=grid'>このサービスグループのステータスグリッド</A><BR>\n", STATUS_CGI, url_encode(servicegroup_name));
				printf("<A HREF='%s?servicegroup=%s'>このサービスグループの稼動レポート</A><BR>\n", AVAIL_CGI, url_encode(servicegroup_name));
				}
			printf("</TD></TR>\n");
			printf("</TABLE>\n");
			}

		printf("</td>\n");

		/* middle column of top row */
		printf("<td align=center valign=middle width=33%%>\n");

		if((display_type == DISPLAY_HOST_INFO && temp_host != NULL) || (display_type == DISPLAY_SERVICE_INFO && temp_host != NULL && temp_service != NULL) || (display_type == DISPLAY_HOSTGROUP_INFO && temp_hostgroup != NULL) || (display_type == DISPLAY_SERVICEGROUP_INFO && temp_servicegroup != NULL)) {

			if(display_type == DISPLAY_HOST_INFO) {
				printf("<DIV CLASS='data'>ホスト</DIV>\n");
				printf("<DIV CLASS='dataTitle'>%s</DIV>\n", temp_host->alias);
				printf("<DIV CLASS='dataTitle'>(%s)</DIV><BR>\n", temp_host->name);

				if(temp_host->parent_hosts != NULL) {
					/* print all parent hosts */
					printf("<DIV CLASS='data'>上位:</DIV>\n");
					for(temp_parenthost = temp_host->parent_hosts; temp_parenthost != NULL; temp_parenthost = temp_parenthost->next)
						printf("<DIV CLASS='dataTitle'><A HREF='%s?host=%s'>%s</A></DIV>\n", STATUS_CGI, url_encode(temp_parenthost->host_name), temp_parenthost->host_name);
					printf("<BR>");
					}

				printf("<DIV CLASS='data'>所属メンバー</DIV><DIV CLASS='dataTitle'>");
				for(temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) {
					if(is_host_member_of_hostgroup(temp_hostgroup, temp_host) == TRUE) {
						if(found == TRUE)
							printf(", ");
						printf("<A HREF='%s?hostgroup=%s&style=overview'>%s</A>", STATUS_CGI, url_encode(temp_hostgroup->group_name), temp_hostgroup->group_name);
						found = TRUE;
						}
					}

				if(found == FALSE)
					printf("ホストグループ無し");
				printf("</DIV><BR>\n");
				printf("<DIV CLASS='data'>%s</DIV>\n", temp_host->address);
				}
			if(display_type == DISPLAY_SERVICE_INFO) {
				printf("<DIV CLASS='data'>ホスト</DIV><DIV CLASS='dataTitle'>%s</DIV><DIV CLASS='data'>上のサービス</DIV>\n", service_desc);
				printf("<DIV CLASS='dataTitle'>%s</DIV>\n", temp_host->alias);
				printf("<DIV CLASS='dataTitle'>(<A HREF='%s?type=%d&host=%s'>%s</a>)</DIV><BR>\n", EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_host->name), temp_host->name);
				printf("<DIV CLASS='data'>所属メンバー</DIV><DIV CLASS='dataTitle'>");
				for(temp_servicegroup = servicegroup_list; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next) {
					if(is_service_member_of_servicegroup(temp_servicegroup, temp_service) == TRUE) {
						if(found == TRUE)
							printf(", ");
						printf("<A HREF='%s?servicegroup=%s&style=overview'>%s</A>", STATUS_CGI, url_encode(temp_servicegroup->group_name), temp_servicegroup->group_name);
						found = TRUE;
						}
					}

				if(found == FALSE)
					printf("サービスグループ無し");
				printf("</DIV><BR>\n");

				printf("<DIV CLASS='data'>%s</DIV>\n", temp_host->address);
				}
			if(display_type == DISPLAY_HOSTGROUP_INFO) {
				printf("<DIV CLASS='data'>ホストグループ</DIV>\n");
				printf("<DIV CLASS='dataTitle'>%s</DIV>\n", temp_hostgroup->alias);
				printf("<DIV CLASS='dataTitle'>(%s)</DIV>\n", temp_hostgroup->group_name);
				if(temp_hostgroup->notes != NULL) {
					process_macros_r(mac, temp_hostgroup->notes, &processed_string, 0);
					printf("<p>%s</p>", processed_string);
					free(processed_string);
					}
				}
			if(display_type == DISPLAY_SERVICEGROUP_INFO) {
				printf("<DIV CLASS='data'>サービスグループ</DIV>\n");
				printf("<DIV CLASS='dataTitle'>%s</DIV>\n", temp_servicegroup->alias);
				printf("<DIV CLASS='dataTitle'>(%s)</DIV>\n", temp_servicegroup->group_name);
				if(temp_servicegroup->notes != NULL) {
					process_macros_r(mac, temp_servicegroup->notes, &processed_string, 0);
					printf("<p>%s</p>", processed_string);
					free(processed_string);
					}
				}

			if(display_type == DISPLAY_SERVICE_INFO) {
				if(temp_service->icon_image != NULL) {
					printf("<img src='%s", url_logo_images_path);
					process_macros_r(mac, temp_service->icon_image, &processed_string, 0);
					printf("%s", processed_string);
					free(processed_string);
					printf("' border=0 alt='%s' title='%s'><BR CLEAR=ALL>", (temp_service->icon_image_alt == NULL) ? "" : temp_service->icon_image_alt, (temp_service->icon_image_alt == NULL) ? "" : temp_service->icon_image_alt);
					}
				if(temp_service->icon_image_alt != NULL)
					printf("<font size=-1><i>( %s )</i></font>\n", temp_service->icon_image_alt);
				if(temp_service->notes != NULL) {
					process_macros_r(mac, temp_service->notes, &processed_string, 0);
					printf("<p>%s</p>\n", processed_string);
					free(processed_string);
					}
				}

			if(display_type == DISPLAY_HOST_INFO) {
				if(temp_host->icon_image != NULL) {
					printf("<img src='%s", url_logo_images_path);
					process_macros_r(mac, temp_host->icon_image, &processed_string, 0);
					printf("%s", processed_string);
					free(processed_string);
					printf("' border=0 alt='%s' title='%s'><BR CLEAR=ALL>", (temp_host->icon_image_alt == NULL) ? "" : temp_host->icon_image_alt, (temp_host->icon_image_alt == NULL) ? "" : temp_host->icon_image_alt);
					}
				if(temp_host->icon_image_alt != NULL)
					printf("<font size=-1><i>( %s )</i><font>\n", temp_host->icon_image_alt);
				if(temp_host->notes != NULL) {
					process_macros_r(mac, temp_host->notes, &processed_string, 0);
					printf("<p>%s</p>\n", processed_string);
					free(processed_string);
					}
				}
			}

		printf("</td>\n");

		/* right column of top row */
		printf("<td align=right valign=bottom width=33%%>\n");

		if(display_type == DISPLAY_HOST_INFO && temp_host != NULL) {
			printf("<TABLE BORDER='0'>\n");
			if(temp_host->action_url != NULL && strcmp(temp_host->action_url, "")) {
				printf("<TR><TD ALIGN='right'>\n");
				printf("<A HREF='");
				process_macros_r(mac, temp_host->action_url, &processed_string, 0);
				printf("%s", processed_string);
				free(processed_string);
				printf("' TARGET='%s'><img src='%s%s' border=0 alt='このホストの対応情報' title='このホストの対応情報'></A>\n", (action_url_target == NULL) ? "_blank" : action_url_target, url_images_path, ACTION_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>対応情報</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				printf("</TD></TR>\n");
				}
			if(temp_host->notes_url != NULL && strcmp(temp_host->notes_url, "")) {
				printf("<TR><TD ALIGN='right'>\n");
				printf("<A HREF='");
				process_macros_r(mac, temp_host->notes_url, &processed_string, 0);
				printf("%s", processed_string);
				free(processed_string);
				/*print_extra_host_url(temp_host->name,temp_host->notes_url);*/
				printf("' TARGET='%s'><img src='%s%s' border=0 alt='このホストの追加情報' title='このホストの追加情報'></A>\n", (notes_url_target == NULL) ? "_blank" : notes_url_target, url_images_path, NOTES_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>追加情報</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				printf("</TD></TR>\n");
				}
			printf("</TABLE>\n");
			}

		else if(display_type == DISPLAY_SERVICE_INFO && temp_service != NULL) {
			printf("<TABLE BORDER='0'><TR><TD ALIGN='right'>\n");
			if(temp_service->action_url != NULL && strcmp(temp_service->action_url, "")) {
				printf("<A HREF='");
				process_macros_r(mac, temp_service->action_url, &processed_string, 0);
				printf("%s", processed_string);
				free(processed_string);
				printf("' TARGET='%s'><img src='%s%s' border=0 alt='このサービスの対応情報' title='このサービスの対応情報'></A>\n", (action_url_target == NULL) ? "_blank" : action_url_target, url_images_path, ACTION_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>対応情報</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				}
			if(temp_service->notes_url != NULL && strcmp(temp_service->notes_url, "")) {
				printf("<A HREF='");
				process_macros_r(mac, temp_service->notes_url, &processed_string, 0);
				printf("%s", processed_string);
				free(processed_string);
				printf("' TARGET='%s'><img src='%s%s' border=0 alt='このサービスの追加情報' title='このサービスの追加情報'></A>\n", (notes_url_target == NULL) ? "_blank" : notes_url_target, url_images_path, NOTES_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>追加情報</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				}
			printf("</TD></TR></TABLE>\n");
			}

		if(display_type == DISPLAY_HOSTGROUP_INFO && temp_hostgroup != NULL) {
			printf("<TABLE BORDER='0'>\n");
			if(temp_hostgroup->action_url != NULL && strcmp(temp_hostgroup->action_url, "")) {
				printf("<TR><TD ALIGN='right'>\n");
				printf("<A HREF='");
				print_extra_hostgroup_url(temp_hostgroup->group_name, temp_hostgroup->action_url);
				printf("' TARGET='%s'><img src='%s%s' border=0 alt='このホストグループの対応情報' title='このホストグループの対応情報'></A>\n", (action_url_target == NULL) ? "_blank" : action_url_target, url_images_path, ACTION_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>対応情報</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				printf("</TD></TR>\n");
				}
			if(temp_hostgroup->notes_url != NULL && strcmp(temp_hostgroup->notes_url, "")) {
				printf("<TR><TD ALIGN='right'>\n");
				printf("<A HREF='");
				print_extra_hostgroup_url(temp_hostgroup->group_name, temp_hostgroup->notes_url);
				printf("' TARGET='%s'><img src='%s%s' border=0 alt='このホストグループの追加情報' title='このホストグループの追加情報'></A>\n", (notes_url_target == NULL) ? "_blank" : notes_url_target, url_images_path, NOTES_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>追加情報</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				printf("</TD></TR>\n");
				}
			printf("</TABLE>\n");
			}

		else if(display_type == DISPLAY_SERVICEGROUP_INFO && temp_servicegroup != NULL) {
			printf("<TABLE BORDER='0'>\n");
			if(temp_servicegroup->action_url != NULL && strcmp(temp_servicegroup->action_url, "")) {
				printf("<A HREF='");
				print_extra_servicegroup_url(temp_servicegroup->group_name, temp_servicegroup->action_url);
				printf("' TARGET='%s'><img src='%s%s' border=0 alt='このサービスグループの対応情報' title='このサービスグループの対応情報'></A>\n", (action_url_target == NULL) ? "_blank" : action_url_target, url_images_path, ACTION_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>対応情報</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				}
			if(temp_servicegroup->notes_url != NULL && strcmp(temp_servicegroup->notes_url, "")) {
				printf("<A HREF='");
				print_extra_servicegroup_url(temp_servicegroup->group_name, temp_servicegroup->notes_url);
				printf("' TARGET='%s'><img src='%s%s' border=0 alt='このサービスグループの追加情報' title='このサービスグループの追加情報'></A>\n", (notes_url_target == NULL) ? "_blank" : notes_url_target, url_images_path, NOTES_ICON);
				printf("<BR CLEAR=ALL><FONT SIZE=-1><I>追加情報</I></FONT><BR CLEAR=ALL><BR CLEAR=ALL>\n");
				}
			printf("</TABLE>\n");
			}

		/* display context-sensitive help */
		if(display_type == DISPLAY_HOST_INFO)
			display_context_help(CONTEXTHELP_EXT_HOST);
		else if(display_type == DISPLAY_SERVICE_INFO)
			display_context_help(CONTEXTHELP_EXT_SERVICE);
		else if(display_type == DISPLAY_HOSTGROUP_INFO)
			display_context_help(CONTEXTHELP_EXT_HOSTGROUP);
		else if(display_type == DISPLAY_SERVICEGROUP_INFO)
			display_context_help(CONTEXTHELP_EXT_SERVICEGROUP);
		else if(display_type == DISPLAY_PROCESS_INFO)
			display_context_help(CONTEXTHELP_EXT_PROCESS);
		else if(display_type == DISPLAY_PERFORMANCE)
			display_context_help(CONTEXTHELP_EXT_PERFORMANCE);
		else if(display_type == DISPLAY_COMMENTS)
			display_context_help(CONTEXTHELP_EXT_COMMENTS);
		else if(display_type == DISPLAY_DOWNTIME)
			display_context_help(CONTEXTHELP_EXT_DOWNTIME);
		else if(display_type == DISPLAY_SCHEDULING_QUEUE)
			display_context_help(CONTEXTHELP_EXT_QUEUE);

		printf("</td>\n");

		/* end of top table */
		printf("</tr>\n");
		printf("</table>\n");

		}

	printf("<BR>\n");

	if(display_type == DISPLAY_HOST_INFO)
		show_host_info();
	else if(display_type == DISPLAY_SERVICE_INFO)
		show_service_info();
	else if(display_type == DISPLAY_COMMENTS)
		show_all_comments();
	else if(display_type == DISPLAY_PERFORMANCE)
		show_performance_data();
	else if(display_type == DISPLAY_HOSTGROUP_INFO)
		show_hostgroup_info();
	else if(display_type == DISPLAY_SERVICEGROUP_INFO)
		show_servicegroup_info();
	else if(display_type == DISPLAY_DOWNTIME)
		show_all_downtime();
	else if(display_type == DISPLAY_SCHEDULING_QUEUE)
		show_scheduling_queue();
	else
		show_process_info();

	document_footer();

	/* free all allocated memory */
	free_memory();
	free_comment_data();
	free_downtime_data();

	return OK;
	}



void document_header(int use_stylesheet) {
	char date_time[MAX_DATETIME_LENGTH];
	char *vidurl = NULL;
	time_t current_time;
	time_t expire_time;

	printf("Cache-Control: no-store\r\n");
	printf("Pragma: no-cache\r\n");
	printf("Refresh: %d\r\n", refresh_rate);

	time(&current_time);
	get_time_string(&current_time, date_time, (int)sizeof(date_time), HTTP_DATE_TIME);
	printf("Last-Modified: %s\r\n", date_time);

	expire_time = (time_t)0L;
	get_time_string(&expire_time, date_time, (int)sizeof(date_time), HTTP_DATE_TIME);
	printf("Expires: %s\r\n", date_time);

	printf("Content-type: text/html; charset=utf-8\r\n\r\n");

	if(embedded == TRUE)
		return;

	printf("<html>\n");
	printf("<head>\n");
	printf("<meta http-equiv='content-type' content='text/html;charset=UTF-8'>\n");
	printf("<link rel=\"shortcut icon\" href=\"%sfavicon.ico\" type=\"image/ico\">\n", url_images_path);
	printf("<title>\n");
	printf("情報\n");
	printf("</title>\n");

	if(use_stylesheet == TRUE) {
		printf("<LINK REL='stylesheet' TYPE='text/css' HREF='%s%s'>", url_stylesheets_path, COMMON_CSS);
		printf("<LINK REL='stylesheet' TYPE='text/css' HREF='%s%s'>", url_stylesheets_path, EXTINFO_CSS);
		printf("<LINK REL='stylesheet' TYPE='text/css' HREF='%s%s'>\n", url_stylesheets_path, NAGFUNCS_CSS);
		}

	if (display_type == DISPLAY_HOST_INFO)
		vidurl = "https://www.youtube.com/embed/n3QEAf-MxY4";
	else if(display_type == DISPLAY_SERVICE_INFO)
		vidurl = "https://www.youtube.com/embed/f_knwQOS6FI";

	if (enable_page_tour == TRUE && vidurl) {
		printf("<script type='text/javascript' src='%s%s'></script>\n", url_js_path, JQUERY_JS);
		printf("<script type='text/javascript' src='%s%s'></script>\n", url_js_path, NAGFUNCS_JS);
		printf("<script type='text/javascript'>\n");
		printf("var vbox, vBoxId='extinfo%d', vboxText = "
				"'<a href=https://www.nagios.com/tours target=_blank>"
				"Nagios4のツアー全体を見るにはここをクリック！</a>';\n",
				display_type);
		printf("$(document).ready(function() {\n"
				"var user = '%s';\nvBoxId += ';' + user;\n",
				current_authdata.username);
		printf("vbox = new vidbox({pos:'lr',vidurl:'%s',text:vboxText,"
				"vidid:vBoxId});\n", vidurl);
		printf("});\n</script>\n");
		}

	printf("</head>\n");

	printf("<body CLASS='extinfo'>\n");

	/* include user SSI header */
	include_ssi_files(EXTINFO_CGI, SSI_HEADER);

	return;
	}


void document_footer(void) {

	if(embedded == TRUE)
		return;

	/* include user SSI footer */
	include_ssi_files(EXTINFO_CGI, SSI_FOOTER);

	printf("</body>\n");
	printf("</html>\n");

	return;
	}


int process_cgivars(void) {
	char **variables;
	int error = FALSE;
	int temp_type;
	int x;

	variables = getcgivars();

	for(x = 0; variables[x]; x++) {

		/* do some basic length checking on the variable identifier to prevent buffer overflows */
		if(strlen(variables[x]) >= MAX_INPUT_BUFFER - 1) {
			continue;
			}

		/* we found the display type */
		else if(!strcmp(variables[x], "type")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}
			temp_type = atoi(variables[x]);
			if(temp_type == DISPLAY_HOST_INFO)
				display_type = DISPLAY_HOST_INFO;
			else if(temp_type == DISPLAY_SERVICE_INFO)
				display_type = DISPLAY_SERVICE_INFO;
			else if(temp_type == DISPLAY_COMMENTS)
				display_type = DISPLAY_COMMENTS;
			else if(temp_type == DISPLAY_PERFORMANCE)
				display_type = DISPLAY_PERFORMANCE;
			else if(temp_type == DISPLAY_HOSTGROUP_INFO)
				display_type = DISPLAY_HOSTGROUP_INFO;
			else if(temp_type == DISPLAY_SERVICEGROUP_INFO)
				display_type = DISPLAY_SERVICEGROUP_INFO;
			else if(temp_type == DISPLAY_DOWNTIME)
				display_type = DISPLAY_DOWNTIME;
			else if(temp_type == DISPLAY_SCHEDULING_QUEUE)
				display_type = DISPLAY_SCHEDULING_QUEUE;
			else
				display_type = DISPLAY_PROCESS_INFO;
			}

		/* we found the host name */
		else if(!strcmp(variables[x], "host")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			host_name = strdup(variables[x]);
			if(host_name == NULL)
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

			hostgroup_name = strdup(variables[x]);
			if(hostgroup_name == NULL)
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

			service_desc = strdup(variables[x]);
			if(service_desc == NULL)
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

			servicegroup_name = strdup(variables[x]);
			if(servicegroup_name == NULL)
				servicegroup_name = "";
			strip_html_brackets(servicegroup_name);
			}

		/* we found the sort type argument */
		else if(!strcmp(variables[x], "sorttype")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			sort_type = atoi(variables[x]);
			}

		/* we found the sort option argument */
		else if(!strcmp(variables[x], "sortoption")) {
			x++;
			if(variables[x] == NULL) {
				error = TRUE;
				break;
				}

			sort_option = atoi(variables[x]);
			}

		/* we found the embed option */
		else if(!strcmp(variables[x], "embedded"))
			embedded = TRUE;

		/* we found the noheader option */
		else if(!strcmp(variables[x], "noheader"))
			display_header = FALSE;
		}

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
	}



void show_process_info(void) {
	char date_time[MAX_DATETIME_LENGTH];
	time_t current_time;
	unsigned long run_time;
	char run_time_string[240];
	int days = 0;
	int hours = 0;
	int minutes = 0;
	int seconds = 0;

	/* make sure the user has rights to view system information */
	if(is_authorized_for_system_information(&current_authdata) == FALSE) {

		printf("<P><DIV CLASS='errorMessage'>プロセス情報を閲覧する権限がありません。</DIV></P>\n");
		printf("<P><DIV CLASS='errorDescription'>このメッセージが何らかのエラーである場合はHTTPサーバのこのCGIに対するアクセス権限の設定かNagiosのCGI用設定ファイルの認証に関するオプションを調べてみてください。</DIV></P>\n");

		return;
		}

	printf("<BR />\n");
	printf("<DIV ALIGN=CENTER>\n");

	printf("<TABLE BORDER=0 CELLPADDING=20>\n");
	printf("<TR><TD VALIGN=TOP>\n");

	printf("<DIV CLASS='dataTitle'>プロセス情報</DIV>\n");

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0 CLASS='data'>\n");
	printf("<TR><TD class='stateInfoTable1'>\n");
	printf("<TABLE BORDER=0>\n");

	/* program version */
	printf("<TR><TD CLASS='dataVar'>プログラムバージョン:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", PROGRAM_VERSION);

	/* program start time */
	get_time_string(&program_start, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
	printf("<TR><TD CLASS='dataVar'>プログラム起動時間:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", date_time);

	/* total running time */
	time(&current_time);
	run_time = (unsigned long)(current_time - program_start);
	get_time_breakdown(run_time, &days, &hours, &minutes, &seconds);
	sprintf(run_time_string, "%d日間と %d時間 %d分 %d秒", days, hours, minutes, seconds);
	printf("<TR><TD CLASS='dataVar'>稼動時間:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", run_time_string);

	/* last log file rotation */
	get_time_string(&last_log_rotation, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
	printf("<TR><TD CLASS='dataVar'>最終ログファイルローテーション:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (last_log_rotation == (time_t)0) ? "N/A" : date_time);

	/* PID */
	printf("<TR><TD CLASS='dataVar'>NagiosプロセスID</TD><TD CLASS='dataVal'>%d</TD></TR>\n", nagios_pid);

	/* notifications enabled */
	printf("<TR><TD CLASS='dataVar'>通知の有効状態</TD><TD CLASS='dataVal'><DIV CLASS='notifications%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (enable_notifications == TRUE) ? "ENABLED" : "DISABLED", (enable_notifications == TRUE) ? "有効" : "無効");

	/* service check execution enabled */
	printf("<TR><TD CLASS='dataVar'>サービスチェックの実行状態</TD><TD CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (execute_service_checks == TRUE) ? "ENABLED" : "DISABLED", (execute_service_checks == TRUE) ? "有効" : "無効");

	/* passive service check acceptance */
	printf("<TR><TD CLASS='dataVar'>パッシブサービスチェックの実行状態</TD><TD CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (accept_passive_service_checks == TRUE) ? "ENABLED" : "DISABLED", (accept_passive_service_checks == TRUE) ? "有効" : "無効");

	/* host check execution enabled */
	printf("<TR><TD CLASS='dataVar'>ホストチェックの実行状態</TD><TD CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (execute_host_checks == TRUE) ? "ENABLED" : "DISABLED", (execute_host_checks == TRUE) ? "有効" : "無効");

	/* passive host check acceptance */
	printf("<TR><TD CLASS='dataVar'>パッシブホストチェックの実行状態</TD><TD CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (accept_passive_host_checks == TRUE) ? "ENABLED" : "DISABLED", (accept_passive_host_checks == TRUE) ? "有効" : "無効");

	/* event handlers enabled */
	printf("<TR><TD CLASS='dataVar'>イベントハンドラの有効状態</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (enable_event_handlers == TRUE) ? "有効" : "無効");

	/* obsessing over services */
	printf("<TR><TD CLASS='dataVar'>Obsessing Overサービス</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (obsess_over_services == TRUE) ? "有効" : "無効");

	/* obsessing over hosts */
	printf("<TR><TD CLASS='dataVar'>Obsessing Overホスト</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (obsess_over_hosts == TRUE) ? "有効" : "無効");

	/* flap detection enabled */
	printf("<TR><TD CLASS='dataVar'>フラップ検知の有効状態</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (enable_flap_detection == TRUE) ? "有効" : "無効");

	/* process performance data */
	printf("<TR><TD CLASS='dataVar'>パフォーマンスデータ処理の実行状態</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (process_performance_data == TRUE) ? "有効" : "無効");

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");


	printf("</TD><TD VALIGN=TOP>\n");

	printf("<DIV CLASS='commandTitle'>プロセスコマンド</DIV>\n");

	printf("<TABLE BORDER=1 CELLPADDING=0 CELLSPACING=0 CLASS='command'>\n");
	printf("<TR><TD>\n");

	if(nagios_process_state == STATE_OK) {
		printf("<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0 CLASS='command'>\n");

#ifndef DUMMY_INSTALL
		printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Nagiosプロセスを停止' TITLE='Nagiosプロセスを停止'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Nagiosプロセスを停止</a></td></tr>\n", url_images_path, STOP_ICON, COMMAND_CGI, CMD_SHUTDOWN_PROCESS);
		printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Nagiosプロセスを再起動' TITLE='Nagiosプロセスを再起動'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Nagiosプロセスを再起動</a></td></tr>\n", url_images_path, RESTART_ICON, COMMAND_CGI, CMD_RESTART_PROCESS);
#endif

		if(enable_notifications == TRUE)
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='通知を無効' TITLE='通知を無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>通知を無効</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_NOTIFICATIONS);
		else
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='通知を有効' TITLE='通知を有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>通知を有効</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_NOTIFICATIONS);

		if(execute_service_checks == TRUE)
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='サービスチェックを停止' TITLE='サービスチェックを停止'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>サービスチェックを停止</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_STOP_EXECUTING_SVC_CHECKS);
		else
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='サービスチェックを開始' TITLE='サービスチェックを開始'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>サービスチェックを開始</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_START_EXECUTING_SVC_CHECKS);

		if(accept_passive_service_checks == TRUE)
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='パッシブサービスチェックを停止' TITLE='パッシブサービスチェックを停止'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>パッシブサービスチェックを停止</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_STOP_ACCEPTING_PASSIVE_SVC_CHECKS);
		else
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='パッシブサービスチェックを開始' TITLE='パッシブサービスチェックを開始'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>パッシブサービスチェックを開始</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_START_ACCEPTING_PASSIVE_SVC_CHECKS);

		if(execute_host_checks == TRUE)
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='ホストチェックを停止' TITLE='ホストチェックを停止'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>ホストチェックを停止</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_STOP_EXECUTING_HOST_CHECKS);
		else
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='ホストチェックを開始' TITLE='ホストチェックを開始'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>ホストチェックを開始</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_START_EXECUTING_HOST_CHECKS);

		if(accept_passive_host_checks == TRUE)
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='パッシブホストチェックを停止' TITLE='パッシブホストチェックを停止'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>パッシブホストチェックを停止</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_STOP_ACCEPTING_PASSIVE_HOST_CHECKS);
		else
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='パッシブホストチェックを開始' TITLE='パッシブホストチェックを開始'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>パッシブホストチェックを開始</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_START_ACCEPTING_PASSIVE_HOST_CHECKS);

		if(enable_event_handlers == TRUE)
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='イベントハンドラを無効' TITLE='イベントハンドラを無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>イベントハンドラを無効</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_EVENT_HANDLERS);
		else
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='イベントハンドラを有効' TITLE='イベントハンドラを有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>イベントハンドラを有効</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_EVENT_HANDLERS);

		if(obsess_over_services == TRUE)
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Obsessing Overサービスを停止' TITLE='Obsessing Overサービスを停止'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Obsessing Overサービスを停止</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_STOP_OBSESSING_OVER_SVC_CHECKS);
		else
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Obsessing Overサービスを開始' TITLE='Obsessing Overサービスを開始'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Obsessing Overサービスを開始</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_START_OBSESSING_OVER_SVC_CHECKS);

		if(obsess_over_hosts == TRUE)
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Obsessing Overホストを停止' TITLE='Obsessing Overホストを停止'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Obsessing Overホストを停止</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_STOP_OBSESSING_OVER_HOST_CHECKS);
		else
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='Obsessing Overホストを開始' TITLE='Obsessing Overホストを開始'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>Obsessing Overホストを開始</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_START_OBSESSING_OVER_HOST_CHECKS);

		if(enable_flap_detection == TRUE)
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='フラップ検知を無効' TITLE='フラップ検知を無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>フラップ検知を無効</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_FLAP_DETECTION);
		else
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='フラップ検知を有効' TITLE='フラップ検知を有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>フラップ検知を有効</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_FLAP_DETECTION);

		if(process_performance_data == TRUE)
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='パフォーマンスデータを無効' TITLE='パフォーマンスデータを無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>パフォーマンスデータを無効</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_PERFORMANCE_DATA);
		else
			printf("<TR CLASS='command'><TD><img src='%s%s' border=0 ALT='パフォーマンスデータを有効' TITLE='パフォーマンスデータを有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d'>パフォーマンスデータを有効</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_PERFORMANCE_DATA);

		printf("</TABLE>\n");
		}
	else {
		printf("<DIV ALIGN=CENTER CLASS='infoMessage'>Nagiosが動作していないため、コマンドは使えません。\n");
		printf("</DIV>\n");
		}

	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</TD></TR></TABLE>\n");
	printf("</DIV>\n");
	}


void show_host_info(void) {
	hoststatus *temp_hoststatus;
	host *temp_host;
	char date_time[MAX_DATETIME_LENGTH];
	char state_duration[48];
	char status_age[48];
	char state_string[MAX_INPUT_BUFFER];
	const char *bg_class = "";
	char *buf = NULL;
	int days;
	int hours;
	int minutes;
	int seconds;
	time_t current_time;
	time_t t;
	int duration_error = FALSE;


	/* get host info */
	temp_host = find_host(host_name);

	/* make sure the user has rights to view host information */
	if(is_authorized_for_host(temp_host, &current_authdata) == FALSE) {

		printf("<P><DIV CLASS='errorMessage'>このホストの情報を閲覧する権限がありません。</DIV></P>\n");
		printf("<P><DIV CLASS='errorDescription'>このメッセージが何らかのエラーである場合はHTTPサーバのこのCGIに対するアクセス権限の設定かNagiosのCGI用設定ファイルの認証に関するオプションを調べてみてください。</DIV></P>\n");

		return;
		}

	/* get host status info */
	temp_hoststatus = find_hoststatus(host_name);

	/* make sure host information exists */
	if(temp_host == NULL) {
		printf("<P><DIV CLASS='errorMessage'>エラー: ホストが見つかりませんでした。</DIV></P>");
		return;
		}
	if(temp_hoststatus == NULL) {
		printf("<P><DIV CLASS='errorMessage'>エラー: ホスト情報が見つかりませんでした。</DIV></P>");
		return;
		}


	printf("<DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 CELLPADDING=0 WIDTH=100%%>\n");
	printf("<TR>\n");

	printf("<TD ALIGN=CENTER VALIGN=TOP CLASS='stateInfoPanel'>\n");

	printf("<DIV CLASS='dataTitle'>ホスト情報</DIV>\n");

	if(temp_hoststatus->has_been_checked == FALSE)
		printf("<P><DIV ALIGN=CENTER>このホストはまだチェックされてないため情報がありません。ステータス情報は利用できません。</DIV></P>\n");

	else {

		printf("<TABLE BORDER=0>\n");
		printf("<TR><TD>\n");

		printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
		printf("<TR><TD class='stateInfoTable1'>\n");
		printf("<TABLE BORDER=0>\n");

		current_time = time(NULL);
		t = 0;
		duration_error = FALSE;
		if(temp_hoststatus->last_state_change == (time_t)0) {
			if(program_start > current_time)
				duration_error = TRUE;
			else
				t = current_time - program_start;
			}
		else {
			if(temp_hoststatus->last_state_change > current_time)
				duration_error = TRUE;
			else
				t = current_time - temp_hoststatus->last_state_change;
			}
		get_time_breakdown((unsigned long)t, &days, &hours, &minutes, &seconds);
		if(duration_error == TRUE)
			snprintf(state_duration, sizeof(state_duration) - 1, "???");
		else
			snprintf(state_duration, sizeof(state_duration) - 1, "%2d日間と %2d時間 %2d分 %2d秒%s", days, hours, minutes, seconds, (temp_hoststatus->last_state_change == (time_t)0) ? "+" : "");
		state_duration[sizeof(state_duration) - 1] = '\x0';

		if(temp_hoststatus->status == SD_HOST_UP) {
			strcpy(state_string, "稼働(UP)");
			bg_class = "hostUP";
			}
		else if(temp_hoststatus->status == SD_HOST_DOWN) {
			strcpy(state_string, "停止(DOWN)");
			bg_class = "hostDOWN";
			}
		else if(temp_hoststatus->status == SD_HOST_UNREACHABLE) {
			strcpy(state_string, "未到達(UNREACHABLE)");
			bg_class = "hostUNREACHABLE";
			}

		printf("<TR><TD CLASS='dataVar'>現在の状態:</td><td CLASS='dataVal'><DIV CLASS='%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV>&nbsp;(%s前より)%s</td></tr>\n", bg_class, state_string, state_duration, (temp_hoststatus->problem_has_been_acknowledged == TRUE) ? "&nbsp;&nbsp;(認知済)" : "");

		printf("<TR><TD CLASS='dataVar' VALIGN='top'>ステータス情報:</td><td CLASS='dataVal'>%s", (temp_hoststatus->plugin_output == NULL) ? "" : html_encode(temp_hoststatus->plugin_output, TRUE));
		if(enable_splunk_integration == TRUE) {
			printf("&nbsp;&nbsp;");
			asprintf(&buf, "%s %s", temp_host->name, temp_hoststatus->plugin_output);
			display_splunk_generic_url(buf, 1);
			free(buf);
			}
		if(temp_hoststatus->long_plugin_output != NULL)
			printf("<BR>%s", html_encode(temp_hoststatus->long_plugin_output, TRUE));
		printf("</TD></TR>\n");

		printf("<TR><TD CLASS='dataVar' VALIGN='top'>パフォーマンスデータ:</td><td CLASS='dataVal'>%s</td></tr>\n", (temp_hoststatus->perf_data == NULL) ? "" : html_encode(temp_hoststatus->perf_data, TRUE));

		printf("<TR><TD CLASS='dataVar'>現在の試行数:</TD><TD CLASS='dataVal'>%d/%d", temp_hoststatus->current_attempt, temp_hoststatus->max_attempts);
		printf("&nbsp;&nbsp;(%s状態)</TD></TR>\n", (temp_hoststatus->state_type == HARD_STATE) ? "ハード" : "ソフト");

		get_time_string(&temp_hoststatus->last_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<TR><TD CLASS='dataVar'>最終チェック時間:</td><td CLASS='dataVal'>%s</td></tr>\n", date_time);

		printf("<TR><TD CLASS='dataVar'>チェックタイプ:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (temp_hoststatus->check_type == CHECK_TYPE_ACTIVE) ? "アクティブ" : "パッシブ");

		printf("<TR><TD CLASS='dataVar' NOWRAP>チェックの待機時間/遅延時間:</TD><TD CLASS='dataVal'>");
		if(temp_hoststatus->check_type == CHECK_TYPE_ACTIVE)
			printf("%.3f", temp_hoststatus->latency);
		else
			printf("N/A");
		printf("&nbsp;/&nbsp;%.3f 秒", temp_hoststatus->execution_time);
		printf("</TD></TR>\n");

		get_time_string(&temp_hoststatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<TR><TD CLASS='dataVar'>次回動作チェックのスケジュール:&nbsp;&nbsp;</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (temp_hoststatus->checks_enabled && temp_hoststatus->next_check != (time_t)0 && temp_hoststatus->should_be_scheduled == TRUE) ? date_time : "N/A");

		get_time_string(&temp_hoststatus->last_state_change, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<TR><TD CLASS='dataVar'>最終ステータス変化時間:</td><td CLASS='dataVal'>%s</td></tr>\n", (temp_hoststatus->last_state_change == (time_t)0) ? "N/A" : date_time);

		get_time_string(&temp_hoststatus->last_notification, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<TR><TD CLASS='dataVar'>最終通知時間:</td><td CLASS='dataVal'>%s&nbsp;(通知回数 %d回)</td></tr>\n", (temp_hoststatus->last_notification == (time_t)0) ? "N/A" : date_time, temp_hoststatus->current_notification_number);

		printf("<TR><TD CLASS='dataVar'>フラッピングの実行状態</td><td CLASS='dataVal'>");
		if(temp_hoststatus->flap_detection_enabled == FALSE || enable_flap_detection == FALSE)
			printf("N/A");
		else
			printf("<DIV CLASS='%sflapping'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV>&nbsp;(状態変化率 %3.2f%%)", (temp_hoststatus->is_flapping == TRUE) ? "" : "not", (temp_hoststatus->is_flapping == TRUE) ? "はい" : "いいえ", temp_hoststatus->percent_state_change);
		printf("</td></tr>\n");

		printf("<TR><TD CLASS='dataVar'>ダウンタイムのスケジュール</td><td CLASS='dataVal'><DIV CLASS='downtime%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></td></tr>\n", (temp_hoststatus->scheduled_downtime_depth > 0) ? "ACTIVE" : "INACTIVE", (temp_hoststatus->scheduled_downtime_depth > 0) ? "はい" : "いいえ");

		t = 0;
		duration_error = FALSE;
		if(temp_hoststatus->last_check > current_time)
			duration_error = TRUE;
		else
			/*t=current_time-temp_hoststatus->last_check;*/
			t = current_time - temp_hoststatus->last_update;
		get_time_breakdown((unsigned long)t, &days, &hours, &minutes, &seconds);
		if(duration_error == TRUE)
			snprintf(status_age, sizeof(status_age) - 1, "???");
		else if(temp_hoststatus->last_check == (time_t)0)
			snprintf(status_age, sizeof(status_age) - 1, "N/A");
		else
			snprintf(status_age, sizeof(status_age) - 1, "%2d日間と %2d時間 %2d分 %2d秒", days, hours, minutes, seconds);
		status_age[sizeof(status_age) - 1] = '\x0';

		get_time_string(&temp_hoststatus->last_update, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<TR><TD CLASS='dataVar'>最終更新時間:</td><td CLASS='dataVal'>%s&nbsp;&nbsp;(%s 前)</td></tr>\n", (temp_hoststatus->last_update == (time_t)0) ? "N/A" : date_time, status_age);

		printf("</TABLE>\n");
		printf("</TD></TR>\n");
		printf("</TABLE>\n");

		printf("</TD></TR>\n");
		printf("<TR><TD>\n");

		printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
		printf("<TR><TD class='stateInfoTable2'>\n");
		printf("<TABLE BORDER=0>\n");

		printf("<TR><TD CLASS='dataVar'>アクティブチェック:</TD><TD CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_hoststatus->checks_enabled == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->checks_enabled == TRUE) ? "有効" : "無効");

		printf("<TR><TD CLASS='dataVar'>パッシブチェック:</TD><td CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_hoststatus->accept_passive_checks == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->accept_passive_checks) ? "有効" : "無効");

		printf("<TR><TD CLASS='dataVar'>Obsessing:</TD><td CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_hoststatus->obsess == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->obsess) ? "有効" : "無効");

		printf("<TR><TD CLASS='dataVar'>通知:</td><td CLASS='dataVal'><DIV CLASS='notifications%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></td></tr>\n", (temp_hoststatus->notifications_enabled) ? "ENABLED" : "DISABLED", (temp_hoststatus->notifications_enabled) ? "有効" : "無効");

		printf("<TR><TD CLASS='dataVar'>イベントハンドラ:</td><td CLASS='dataVal'><DIV CLASS='eventhandlers%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></td></tr>\n", (temp_hoststatus->event_handler_enabled) ? "ENABLED" : "DISABLED", (temp_hoststatus->event_handler_enabled) ? "有効" : "無効");

		printf("<TR><TD CLASS='dataVar'>フラップ検知:</td><td CLASS='dataVal'><DIV CLASS='flapdetection%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></td></tr>\n", (temp_hoststatus->flap_detection_enabled == TRUE) ? "ENABLED" : "DISABLED", (temp_hoststatus->flap_detection_enabled == TRUE) ? "有効" : "無効");

		printf("</TABLE>\n");
		printf("</TD></TR>\n");
		printf("</TABLE>\n");

		printf("</TD></TR>\n");
		printf("</TABLE>\n");
		}

	printf("</TD>\n");

	printf("<TD ALIGN=CENTER VALIGN=TOP>\n");
	printf("<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0><TR>\n");

	printf("<TD ALIGN=CENTER VALIGN=TOP CLASS='commandPanel'>\n");

	printf("<DIV CLASS='commandTitle'>ホストコマンド</DIV>\n");

	printf("<TABLE BORDER='1' CELLPADDING=0 CELLSPACING=0><TR><TD>\n");

	if(nagios_process_state == STATE_OK && is_authorized_for_read_only(&current_authdata) == FALSE) {

		printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 CLASS='command'>\n");
#ifdef USE_STATUSMAP
		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='マップ上のホストの位置を確認' TITLE='マップ上のホストの位置を確認'></td><td CLASS='command'><a href='%s?host=%s&root=%s'>マップ上のホストの位置を確認</a></td></tr>\n", url_images_path, STATUSMAP_ICON, STATUSMAP_CGI, url_encode(host_name), url_encode(host_name));
#endif
		if(temp_hoststatus->checks_enabled == TRUE) {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストの動作チェックを無効' TITLE='このホストの動作チェックを無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストの動作チェックを無効</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_HOST_CHECK, url_encode(host_name));
			}
		else
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストの動作チェックを有効' TITLE='このホストの動作チェックを有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストの動作チェックを有効</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_HOST_CHECK, url_encode(host_name));
		printf("<tr CLASS='data'><td><img src='%s%s' border=0 ALT='このホストを次回スケジュールに追加' TITLE='このホストを次回スケジュールに追加'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s%s'>このホストを次回スケジュールに追加</a></td></tr>\n", url_images_path, DELAY_ICON, COMMAND_CGI, CMD_SCHEDULE_HOST_CHECK, url_encode(host_name), (temp_hoststatus->checks_enabled == TRUE) ? "&force_check" : "");

		if(temp_hoststatus->accept_passive_checks == TRUE) {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストのパッシブチェックの結果を送信' TITLE='このホストのパッシブチェックの結果を送信'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストのパッシブチェックの結果を送信</a></td></tr>\n", url_images_path, PASSIVE_ICON, COMMAND_CGI, CMD_PROCESS_HOST_CHECK_RESULT, url_encode(host_name));
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストのパッシブチェックを停止' TITLE='このホストのパッシブチェックを停止'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストのパッシブチェックを停止</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_PASSIVE_HOST_CHECKS, url_encode(host_name));
			}
		else
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストのパッシブチェックを開始' TITLE='このホストのパッシブチェックを開始'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストのパッシブチェックを開始</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_PASSIVE_HOST_CHECKS, url_encode(host_name));

		if(temp_hoststatus->obsess == TRUE)
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストのObsessing Overを停止' TITLE='このホストのObsessing Overを停止'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストのObsessing Overを停止</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_STOP_OBSESSING_OVER_HOST, url_encode(host_name));
		else
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストのObsessing Overを開始' TITLE='このホストのObsessing Overを開始'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストのObsessing Overを開始</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_START_OBSESSING_OVER_HOST, url_encode(host_name));

		if(temp_hoststatus->status == SD_HOST_DOWN || temp_hoststatus->status == SD_HOST_UNREACHABLE) {
			if(temp_hoststatus->problem_has_been_acknowledged == FALSE)
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='認知済' TITLE='認知済'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>認知済</a></td></tr>\n", url_images_path, ACKNOWLEDGEMENT_ICON, COMMAND_CGI, CMD_ACKNOWLEDGE_HOST_PROBLEM, url_encode(host_name));
			else
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='認知済を解除' TITLE='認知済を解除'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>認知済を解除</a></td></tr>\n", url_images_path, REMOVE_ACKNOWLEDGEMENT_ICON, COMMAND_CGI, CMD_REMOVE_HOST_ACKNOWLEDGEMENT, url_encode(host_name));
			}

		if(temp_hoststatus->notifications_enabled == TRUE)
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストの通知を無効' TITLE='このホストの通知を無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストの通知を無効</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_HOST_NOTIFICATIONS, url_encode(host_name));
		else
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストの通知を有効' TITLE='このホストの通知を有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストの通知を有効</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_HOST_NOTIFICATIONS, url_encode(host_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='今すぐ通知する' TITLE='今すぐ通知する'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>今すぐ通知する</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, COMMAND_CGI, CMD_SEND_CUSTOM_HOST_NOTIFICATION, url_encode(host_name));

		if(temp_hoststatus->status != SD_HOST_UP)
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='次のホスト通知を遅らせる' TITLE='次のホスト通知を遅らせる'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>次のホスト通知を遅らせる</a></td></tr>\n", url_images_path, DELAY_ICON, COMMAND_CGI, CMD_DELAY_HOST_NOTIFICATION, url_encode(host_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストのダウンタイムをスケジュール' TITLE='このホストのダウンタイムをスケジュール'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストのダウンタイムをスケジュール</a></td></tr>\n", url_images_path, DOWNTIME_ICON, COMMAND_CGI, CMD_SCHEDULE_HOST_DOWNTIME, url_encode(host_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホスト上の全サービスのダウンタイムをスケジュール' TITLE='このホスト上の全サービスのダウンタイムをスケジュール'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホスト上の全サービスのダウンタイムをスケジュール</a></td></tr>\n", url_images_path, DOWNTIME_ICON, COMMAND_CGI, CMD_SCHEDULE_HOST_SVC_DOWNTIME, url_encode(host_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストの全サービスの通知を無効' TITLE='このホストの全サービスの通知を無効'></td><td CLASS='command' NOWRAP><a href='%s?cmd_typ=%d&host=%s'>このホストの全サービスの通知を無効</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_HOST_SVC_NOTIFICATIONS, url_encode(host_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストの全サービスの通知を有効' TITLE='このホストの全サービスの通知を有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストの全サービスの通知を有効</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_HOST_SVC_NOTIFICATIONS, url_encode(host_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストの全サービスのチェックをスケジュール' TITLE='このホストの全サービスのチェックをスケジュール'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストの全サービスのチェックをスケジュール</a></td></tr>\n", url_images_path, DELAY_ICON, COMMAND_CGI, CMD_SCHEDULE_HOST_SVC_CHECKS, url_encode(host_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストの全サービスのチェックを無効' TITLE='このホストの全サービスのチェックを無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストの全サービスのチェックを無効</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_HOST_SVC_CHECKS, url_encode(host_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストの全サービスのチェックを有効' TITLE='このホストの全サービスのチェックを有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストの全サービスのチェックを有効</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_HOST_SVC_CHECKS, url_encode(host_name));

		if(temp_hoststatus->event_handler_enabled == TRUE)
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストのイベントハンドラを無効' TITLE='このホストのイベントハンドラを無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストのイベントハンドラを無効</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_HOST_EVENT_HANDLER, url_encode(host_name));
		else
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストのイベントハンドラを有効' TITLE='このホストのイベントハンドラを有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストのイベントハンドラを有効</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_HOST_EVENT_HANDLER, url_encode(host_name));
		if(temp_hoststatus->flap_detection_enabled == TRUE) {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストのフラップ検知を無効' TITLE='このホストのフラップ検知を無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストのフラップ検知を無効</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_HOST_FLAP_DETECTION, url_encode(host_name));
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストのフラッピング状態をクリア' TITLE='このホストのフラッピング状態をクリア'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストのフラッピング状態をクリア</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_CLEAR_HOST_FLAPPING_STATE, url_encode(host_name));
		} else
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストのフラップ検知を有効' TITLE='このホストのフラップ検知を有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s'>このホストのフラップ検知を有効</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_HOST_FLAP_DETECTION, url_encode(host_name));

		printf("</TABLE>\n");
		}
	else if(is_authorized_for_read_only(&current_authdata) == TRUE) {
		printf("<DIV ALIGN=CENTER CLASS='infoMessage'>あなたのアカウントはコマンドを実行する権限を持っていません。<br>\n");
		}
	else {
		printf("<DIV ALIGN=CENTER CLASS='infoMessage'>Nagiosが動作していないため、コマンドは使えません。<br>\n");
		printf("<a href='%s?type=%d'>ここをクリック</a>してNagiosプロセス情報を確認してください。</DIV>\n", EXTINFO_CGI, DISPLAY_PROCESS_INFO);
		}
	printf("</TD></TR></TABLE>\n");

	printf("</TD>\n");

	printf("</TR>\n");
	printf("</TABLE></TR>\n");

	printf("<TR>\n");

	printf("<TD COLSPAN=2 ALIGN=CENTER VALIGN=TOP CLASS='commentPanel'>\n");

	/* display comments */
	display_comments(HOST_COMMENT);

	printf("</TD>\n");

	printf("</TR>\n");
	printf("</TABLE>\n");
	printf("</DIV>\n");

	return;
	}


void show_service_info(void) {
	service *temp_service;
	char date_time[MAX_DATETIME_LENGTH];
	char status_age[48];
	char state_duration[48];
	servicestatus *temp_svcstatus;
	char state_string[MAX_INPUT_BUFFER];
	const char *bg_class = "";
	char *buf = NULL;
	int days;
	int hours;
	int minutes;
	int seconds;
	time_t t;
	time_t current_time;
	int duration_error = FALSE;

	/* find the service */
	temp_service = find_service(host_name, service_desc);

	/* make sure the user has rights to view service information */
	if(is_authorized_for_service(temp_service, &current_authdata) == FALSE) {

		printf("<P><DIV CLASS='errorMessage'>このサービス情報を閲覧する権限が無いようです。</DIV></P>\n");
		printf("<P><DIV CLASS='errorDescription'>このメッセージが何らかのエラーである場合はHTTPサーバのこのCGIに対するアクセス権限の設定かNagiosのCGI用設定ファイルの認証に関するオプションを調べてみてください</DIV></P>\n");

		return;
		}

	/* get service status info */
	temp_svcstatus = find_servicestatus(host_name, service_desc);

	/* make sure service information exists */
	if(temp_service == NULL) {
		printf("<P><DIV CLASS='errorMessage'>エラー: サービスが見つかりません。</DIV></P>");
		return;
		}
	if(temp_svcstatus == NULL) {
		printf("<P><DIV CLASS='errorMessage'>エラー: サービスステータスが見つかりません。</DIV></P>");
		return;
		}


	printf("<DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0 WIDTH=100%%>\n");
	printf("<TR>\n");

	printf("<TD ALIGN=CENTER VALIGN=TOP CLASS='stateInfoPanel'>\n");

	printf("<DIV CLASS='dataTitle'>サービスステータス情報</DIV>\n");

	if(temp_svcstatus->has_been_checked == FALSE)
		printf("<P><DIV ALIGN=CENTER>このサービスはまだチェックされていないのでステータス情報と統計結果はまだありません。</DIV></P>\n");

	else {

		printf("<TABLE BORDER=0>\n");

		printf("<TR><TD>\n");

		printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
		printf("<TR><TD class='stateInfoTable1'>\n");
		printf("<TABLE BORDER=0>\n");


		current_time = time(NULL);
		t = 0;
		duration_error = FALSE;
		if(temp_svcstatus->last_state_change == (time_t)0) {
			if(program_start > current_time)
				duration_error = TRUE;
			else
				t = current_time - program_start;
			}
		else {
			if(temp_svcstatus->last_state_change > current_time)
				duration_error = TRUE;
			else
				t = current_time - temp_svcstatus->last_state_change;
			}
		get_time_breakdown((unsigned long)t, &days, &hours, &minutes, &seconds);
		if(duration_error == TRUE)
			snprintf(state_duration, sizeof(state_duration) - 1, "???");
		else
			snprintf(state_duration, sizeof(state_duration) - 1, "%2d日間と %2d時間 %2d分 %2d秒%s", days, hours, minutes, seconds, (temp_svcstatus->last_state_change == (time_t)0) ? "+" : "");
		state_duration[sizeof(state_duration) - 1] = '\x0';

		if(temp_svcstatus->status == SERVICE_OK) {
			strcpy(state_string, "正常(OK)");
			bg_class = "serviceOK";
			}
		else if(temp_svcstatus->status == SERVICE_WARNING) {
			strcpy(state_string, "警告(WARNING)");
			bg_class = "serviceWARNING";
			}
		else if(temp_svcstatus->status == SERVICE_CRITICAL) {
			strcpy(state_string, "異常(CRITICAL)");
			bg_class = "serviceCRITICAL";
			}
		else {
			strcpy(state_string, "不明(UNKNOWN)");
			bg_class = "serviceUNKNOWN";
			}
		printf("<TR><TD CLASS='dataVar'>現在の状態:</TD><TD CLASS='dataVal'><DIV CLASS='%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV>&nbsp;(%s前より)%s</TD></TR>\n", bg_class, state_string, state_duration, (temp_svcstatus->problem_has_been_acknowledged == TRUE) ? "&nbsp;&nbsp;(認知済)" : "");

		printf("<TR><TD CLASS='dataVar' VALIGN='top'>ステータス情報:</TD><TD CLASS='dataVal'>%s", (temp_svcstatus->plugin_output == NULL) ? "" : html_encode(temp_svcstatus->plugin_output, TRUE));
		if(enable_splunk_integration == TRUE) {
			printf("&nbsp;&nbsp;");
			asprintf(&buf, "%s %s %s", temp_service->host_name, temp_service->description, temp_svcstatus->plugin_output);
			display_splunk_generic_url(buf, 1);
			free(buf);
			}
		if(temp_svcstatus->long_plugin_output != NULL)
			printf("<BR>%s", html_encode(temp_svcstatus->long_plugin_output, TRUE));
		printf("</TD></TR>\n");

		printf("<TR><TD CLASS='dataVar' VALIGN='top'>パフォーマンスデータ:</td><td CLASS='dataVal'>%s</td></tr>\n", (temp_svcstatus->perf_data == NULL) ? "" : html_encode(temp_svcstatus->perf_data, TRUE));

		printf("<TR><TD CLASS='dataVar'>現在の試行数:</TD><TD CLASS='dataVal'>%d/%d", temp_svcstatus->current_attempt, temp_svcstatus->max_attempts);
		printf("&nbsp;&nbsp;(%s 状態)</TD></TR>\n", (temp_svcstatus->state_type == HARD_STATE) ? "ハード" : "ソフト");

		get_time_string(&temp_svcstatus->last_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<TR><TD CLASS='dataVar'>最終チェック時間:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", date_time);

		printf("<TR><TD CLASS='dataVar'>チェックタイプ:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (temp_svcstatus->check_type == CHECK_TYPE_ACTIVE) ? "アクティブ" : "パッシブ");

		printf("<TR><TD CLASS='dataVar' NOWRAP>チェックの待機時間/遅延時間:</TD><TD CLASS='dataVal'>");
		if(temp_svcstatus->check_type == CHECK_TYPE_ACTIVE)
			printf("%.3f", temp_svcstatus->latency);
		else
			printf("N/A");
		printf("&nbsp;/&nbsp;%.3f 秒", temp_svcstatus->execution_time);
		printf("</TD></TR>\n");

		get_time_string(&temp_svcstatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<TR><TD CLASS='dataVar'>次回動作チェックのスケジュール:&nbsp;&nbsp;</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (temp_svcstatus->checks_enabled && temp_svcstatus->next_check != (time_t)0 && temp_svcstatus->should_be_scheduled == TRUE) ? date_time : "N/A");

		get_time_string(&temp_svcstatus->last_state_change, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<TR><TD CLASS='dataVar'>最終ステータス変化時間:</TD><TD CLASS='dataVal'>%s</TD></TR>\n", (temp_svcstatus->last_state_change == (time_t)0) ? "N/A" : date_time);

		get_time_string(&temp_svcstatus->last_notification, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<TR><TD CLASS='dataVar'>最終通知時間:</TD><TD CLASS='dataVal'>%s&nbsp;(通知回数 %d回)</TD></TR>\n", (temp_svcstatus->last_notification == (time_t)0) ? "N/A" : date_time, temp_svcstatus->current_notification_number);

		printf("<TR><TD CLASS='dataVar'>フラッピングの実行状態</TD><TD CLASS='dataVal'>");
		if(temp_svcstatus->flap_detection_enabled == FALSE || enable_flap_detection == FALSE)
			printf("N/A");
		else
			printf("<DIV CLASS='%sflapping'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV>&nbsp;(状態変化率 %3.2f%%)", (temp_svcstatus->is_flapping == TRUE) ? "" : "not", (temp_svcstatus->is_flapping == TRUE) ? "はい" : "いいえ", temp_svcstatus->percent_state_change);
		printf("</TD></TR>\n");

		printf("<TR><TD CLASS='dataVar'>ダウンタイムのスケジュール</TD><TD CLASS='dataVal'><DIV CLASS='downtime%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_svcstatus->scheduled_downtime_depth > 0) ? "ACTIVE" : "INACTIVE", (temp_svcstatus->scheduled_downtime_depth > 0) ? "はい" : "いいえ");

		t = 0;
		duration_error = FALSE;
		if(temp_svcstatus->last_check > current_time)
			duration_error = TRUE;
		else
			/*t=current_time-temp_svcstatus->last_check;*/
			t = current_time - temp_svcstatus->last_update;
		get_time_breakdown((unsigned long)t, &days, &hours, &minutes, &seconds);
		if(duration_error == TRUE)
			snprintf(status_age, sizeof(status_age) - 1, "???");
		else if(temp_svcstatus->last_check == (time_t)0)
			snprintf(status_age, sizeof(status_age) - 1, "N/A");
		else
			snprintf(status_age, sizeof(status_age) - 1, "%2d日間と %2d時間 %2d分 %2d秒", days, hours, minutes, seconds);
		status_age[sizeof(status_age) - 1] = '\x0';

		get_time_string(&temp_svcstatus->last_update, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<TR><TD CLASS='dataVar'>最終更新時間:</TD><TD CLASS='dataVal'>%s&nbsp;&nbsp;(%s 前)</TD></TR>\n", (temp_svcstatus->last_update == (time_t)0) ? "N/A" : date_time, status_age);


		printf("</TABLE>\n");
		printf("</TD></TR>\n");
		printf("</TABLE>\n");

		printf("</TD></TR>\n");

		printf("<TR><TD>\n");

		printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
		printf("<TR><TD class='stateInfoTable2'>\n");
		printf("<TABLE BORDER=0>\n");

		printf("<TR><TD CLASS='dataVar'>アクティブチェック:</TD><td CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_svcstatus->checks_enabled) ? "ENABLED" : "DISABLED", (temp_svcstatus->checks_enabled) ? "有効" : "無効");

		printf("<TR><TD CLASS='dataVar'>パッシブチェック:</TD><td CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_svcstatus->accept_passive_checks == TRUE) ? "ENABLED" : "DISABLED", (temp_svcstatus->accept_passive_checks) ? "有効" : "無効");

		printf("<TR><TD CLASS='dataVar'>Obsessing:</TD><td CLASS='dataVal'><DIV CLASS='checks%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_svcstatus->obsess == TRUE) ? "ENABLED" : "DISABLED", (temp_svcstatus->obsess) ? "有効" : "無効");

		printf("<TR><td CLASS='dataVar'>通知:</TD><td CLASS='dataVal'><DIV CLASS='notifications%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_svcstatus->notifications_enabled) ? "ENABLED" : "DISABLED", (temp_svcstatus->notifications_enabled) ? "有効" : "無効");

		printf("<TR><TD CLASS='dataVar'>イベントハンドラ:</TD><td CLASS='dataVal'><DIV CLASS='eventhandlers%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_svcstatus->event_handler_enabled) ? "ENABLED" : "DISABLED", (temp_svcstatus->event_handler_enabled) ? "有効" : "無効");

		printf("<TR><TD CLASS='dataVar'>フラップ検知:</TD><td CLASS='dataVal'><DIV CLASS='flapdetection%s'>&nbsp;&nbsp;%s&nbsp;&nbsp;</DIV></TD></TR>\n", (temp_svcstatus->flap_detection_enabled == TRUE) ? "ENABLED" : "DISABLED", (temp_svcstatus->flap_detection_enabled == TRUE) ? "有効" : "無効");


		printf("</TABLE>\n");
		printf("</TD></TR>\n");
		printf("</TABLE>\n");

		printf("</TD></TR>\n");

		printf("</TABLE>\n");
		}


	printf("</TD>\n");

	printf("<TD ALIGN=CENTER VALIGN=TOP>\n");
	printf("<TABLE BORDER='0' CELLPADDING=0 CELLSPACING=0><TR>\n");

	printf("<TD ALIGN=CENTER VALIGN=TOP CLASS='commandPanel'>\n");

	printf("<DIV CLASS='dataTitle'>サービスコマンド</DIV>\n");

	printf("<TABLE BORDER='1' CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD>\n");

	if(nagios_process_state == STATE_OK &&  is_authorized_for_read_only(&current_authdata) == FALSE) {
		printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 CLASS='command'>\n");

		if(temp_svcstatus->checks_enabled) {

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスの動作チェックを無効' TITLE='このサービスの動作チェックを無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_SVC_CHECK, url_encode(host_name));
			printf("&service=%s'>このサービスの動作チェックを無効</a></td></tr>\n", url_encode(service_desc));
			}
		else {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスの動作チェックを有効' TITLE='このサービスの動作チェックを有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_SVC_CHECK, url_encode(host_name));
			printf("&service=%s'>このサービスの動作チェックを有効</a></td></tr>\n", url_encode(service_desc));
			}
		printf("<tr CLASS='data'><td><img src='%s%s' border=0 ALT='このサービスの動作チェックを次回スケジュールに追加' TITLE='このサービスの動作チェックを次回スケジュールに追加'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DELAY_ICON, COMMAND_CGI, CMD_SCHEDULE_SVC_CHECK, url_encode(host_name));
		printf("&service=%s%s'>このサービスの動作チェックを次回スケジュールに追加</a></td></tr>\n", url_encode(service_desc), (temp_svcstatus->checks_enabled == TRUE) ? "&force_check" : "");

		if(temp_svcstatus->accept_passive_checks == TRUE) {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスのパッシブチェックの結果を送信' TITLE='このサービスのパッシブチェックの結果を送信'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, PASSIVE_ICON, COMMAND_CGI, CMD_PROCESS_SERVICE_CHECK_RESULT, url_encode(host_name));
			printf("&service=%s'>このサービスのパッシブチェックの結果を送信</a></td></tr>\n", url_encode(service_desc));

			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスのパッシブチェックを停止' TITLE='このサービスのパッシブチェックを停止'></td><td CLASS='command' NOWRAP><a href='%s?cmd_typ=%d&host=%s", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_PASSIVE_SVC_CHECKS, url_encode(host_name));
			printf("&service=%s'>このサービスのパッシブチェックを停止</a></td></tr>\n", url_encode(service_desc));
			}
		else {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスのパッシブチェックを開始' TITLE='このサービスのパッシブチェックを開始'></td><td CLASS='command' NOWRAP><a href='%s?cmd_typ=%d&host=%s", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_PASSIVE_SVC_CHECKS, url_encode(host_name));
			printf("&service=%s'>このサービスのパッシブチェックを開始</a></td></tr>\n", url_encode(service_desc));
			}

		if(temp_svcstatus->obsess == TRUE) {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスのObsessing Overを停止' TITLE='このサービスのObsessing Overを停止'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_STOP_OBSESSING_OVER_SVC, url_encode(host_name));
			printf("&service=%s'>このサービスのObsessing Overを停止</a></td></tr>\n", url_encode(service_desc));
			}
		else {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスのObsessing Overを開始' TITLE='このサービスのObsessing Overを開始'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_START_OBSESSING_OVER_SVC, url_encode(host_name));
			printf("&service=%s'>このサービスのObsessing Overを開始</a></td></tr>\n", url_encode(service_desc));
			}

		if((temp_svcstatus->status == SERVICE_WARNING || temp_svcstatus->status == SERVICE_UNKNOWN || temp_svcstatus->status == SERVICE_CRITICAL) && temp_svcstatus->state_type == HARD_STATE) {
			if(temp_svcstatus->problem_has_been_acknowledged == FALSE) {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='この問題を認知済にする' TITLE='この問題を認知済にする'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, ACKNOWLEDGEMENT_ICON, COMMAND_CGI, CMD_ACKNOWLEDGE_SVC_PROBLEM, url_encode(host_name));
				printf("&service=%s'>この問題を認知済にする</a></td></tr>\n", url_encode(service_desc));
				}
			else {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='認知済を解除' TITLE='認知済を解除'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, REMOVE_ACKNOWLEDGEMENT_ICON, COMMAND_CGI, CMD_REMOVE_SVC_ACKNOWLEDGEMENT, url_encode(host_name));
				printf("&service=%s'>認知済を解除</a></td></tr>\n", url_encode(service_desc));
				}
			}
		if(temp_svcstatus->notifications_enabled == TRUE) {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスの通知を無効' TITLE='このサービスの通知を無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_SVC_NOTIFICATIONS, url_encode(host_name));
			printf("&service=%s'>このサービスの通知を無効</a></td></tr>\n", url_encode(service_desc));
			if(temp_svcstatus->status != SERVICE_OK) {
				printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='次のサービス通知を遅らせる' TITLE='次のサービス通知を遅らせる'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DELAY_ICON, COMMAND_CGI, CMD_DELAY_SVC_NOTIFICATION, url_encode(host_name));
				printf("&service=%s'>次のサービス通知を遅らせる</a></td></tr>\n", url_encode(service_desc));
				}
			}
		else {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスの通知を有効' TITLE='このサービスの通知を有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_SVC_NOTIFICATIONS, url_encode(host_name));
			printf("&service=%s'>このサービスの通知を有効</a></td></tr>\n", url_encode(service_desc));
			}

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='今すぐ通知する' TITLE='今すぐ通知する'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, NOTIFICATION_ICON, COMMAND_CGI, CMD_SEND_CUSTOM_SVC_NOTIFICATION, url_encode(host_name));
		printf("&service=%s'>今すぐ通知する</a></td></tr>\n", url_encode(service_desc));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスのダウンタイムをスケジュール' TITLE='このサービスのダウンタイムをスケジュール'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DOWNTIME_ICON, COMMAND_CGI, CMD_SCHEDULE_SVC_DOWNTIME, url_encode(host_name));
		printf("&service=%s'>このサービスのダウンタイムをスケジュール</a></td></tr>\n", url_encode(service_desc));

		/*
		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='Cancel Scheduled Downtime For This Service' TITLE='Cancel Scheduled Downtime For This Service'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s",url_images_path,SCHEDULED_DOWNTIME_ICON,COMMAND_CGI,CMD_CANCEL_SVC_DOWNTIME,url_encode(host_name));
		printf("&service=%s'>Cancel scheduled downtime for this service</a></td></tr>\n",url_encode(service_desc));
		*/

		if(temp_svcstatus->event_handler_enabled == TRUE) {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスのイベントハンドラを無効' TITLE='このサービスのイベントハンドラを無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_SVC_EVENT_HANDLER, url_encode(host_name));
			printf("&service=%s'>このサービスのイベントハンドラを無効</a></td></tr>\n", url_encode(service_desc));
			}
		else {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスのイベントハンドラを有効' TITLE='このサービスのイベントハンドラを有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_SVC_EVENT_HANDLER, url_encode(host_name));
			printf("&service=%s'>このサービスのイベントハンドラを有効</a></td></tr>\n", url_encode(service_desc));
			}

		if(temp_svcstatus->flap_detection_enabled == TRUE) {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスのフラップ検知を無効' TITLE='このサービスのフラップ検知を無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_SVC_FLAP_DETECTION, url_encode(host_name));
			printf("&service=%s'>このサービスのフラップ検知を無効</a></td></tr>\n", url_encode(service_desc));
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスのフラッピング状態をクリア' TITLE='このサービスのフラッピング状態をクリア'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_CLEAR_SVC_FLAPPING_STATE, url_encode(host_name));
			printf("&service=%s'>このサービスのフラッピング状態をクリア</a></td></tr>\n", url_encode(service_desc));
			}
		else {
			printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスのフラップ検知を有効' TITLE='このサービスのフラップ検知を有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&host=%s", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_SVC_FLAP_DETECTION, url_encode(host_name));
			printf("&service=%s'>このサービスのフラップ検知を有効</a></td></tr>\n", url_encode(service_desc));
			}

		printf("</table>\n");
		}
	else if(is_authorized_for_read_only(&current_authdata) == TRUE) {
		printf("<DIV ALIGN=CENTER CLASS='infoMessage'>あなたのアカウントはコマンドを実行する権限を持っていません。<br>\n");
		}
	else {
		printf("<DIV CLASS='infoMessage'>Nagiosが動作していないため、コマンドは使えません。<br>\n");
		printf("<a href='%s?type=%d'>ここをクリック</a>してNagiosプロセス情報を確認してください。</DIV>\n", EXTINFO_CGI, DISPLAY_PROCESS_INFO);
		}

	printf("</td></tr>\n");
	printf("</table>\n");

	printf("</TD>\n");

	printf("</TR></TABLE></TD>\n");
	printf("</TR>\n");

	printf("<TR><TD COLSPAN=2><BR></TD></TR>\n");

	printf("<TR>\n");
	printf("<TD COLSPAN=2 ALIGN=CENTER VALIGN=TOP CLASS='commentPanel'>\n");

	/* display comments */
	display_comments(SERVICE_COMMENT);

	printf("</TD>\n");
	printf("</TR>\n");

	printf("</TABLE>\n");
	printf("</DIV>\n");

	return;
	}




void show_hostgroup_info(void) {
	hostgroup *temp_hostgroup;


	/* get hostgroup info */
	temp_hostgroup = find_hostgroup(hostgroup_name);

	/* make sure the user has rights to view hostgroup information */
	if(is_authorized_for_hostgroup(temp_hostgroup, &current_authdata) == FALSE) {

		printf("<P><DIV CLASS='errorMessage'>ホストグループ情報を閲覧する権限が無いようです。</DIV></P>\n");
		printf("<P><DIV CLASS='errorDescription'>このメッセージが何らかのエラーである場合はHTTPサーバのこのCGIに対するアクセス権限の設定かNagiosのCGI用設定ファイルの認証に関するオプションを調べてみてください</DIV></P>\n");

		return;
		}

	/* make sure hostgroup information exists */
	if(temp_hostgroup == NULL) {
		printf("<P><DIV CLASS='errorMessage'>エラー: このホストグループは存在しません。</DIV></P>");
		return;
		}


	printf("<DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 WIDTH=100%%>\n");
	printf("<TR>\n");


	/* top left panel */
	printf("<TD ALIGN=CENTER VALIGN=TOP CLASS='stateInfoPanel'>\n");

	/* right top panel */
	printf("</TD><TD ALIGN=CENTER VALIGN=TOP CLASS='stateInfoPanel' ROWSPAN=2>\n");

	printf("<DIV CLASS='dataTitle'>ホストグループコマンド</DIV>\n");

	if(nagios_process_state == STATE_OK && is_authorized_for_read_only(&current_authdata) == FALSE) {

		printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0 CLASS='command'>\n");
		printf("<TR><TD>\n");

		printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 CLASS='command'>\n");

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストグループ上全ホストのダウンタイムをスケジュール' TITLE='このホストグループ上全ホストのダウンタイムをスケジュール'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>このホストグループ上全ホストのダウンタイムをスケジュール</a></td></tr>\n", url_images_path, DOWNTIME_ICON, COMMAND_CGI, CMD_SCHEDULE_HOSTGROUP_HOST_DOWNTIME, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストグループ上全サービスのダウンタイムをスケジュール' TITLE='このホストグループ上全サービスのダウンタイムをスケジュール'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>このホストグループ上全サービスのダウンタイムをスケジュール</a></td></tr>\n", url_images_path, DOWNTIME_ICON, COMMAND_CGI, CMD_SCHEDULE_HOSTGROUP_SVC_DOWNTIME, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストグループ上の全ホストの通知を有効' TITLE='このホストグループ上の全ホストの通知を有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>このホストグループ上の全ホストの通知を有効</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, COMMAND_CGI, CMD_ENABLE_HOSTGROUP_HOST_NOTIFICATIONS, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストグループ上の全ホストの通知を無効' TITLE='このホストグループ上の全ホストの通知を無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>このホストグループ上の全ホストの通知を無効</a></td></tr>\n", url_images_path, NOTIFICATIONS_DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_HOSTGROUP_HOST_NOTIFICATIONS, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストグループ上の全サービスの通知を有効' TITLE='このホストグループ上の全サービスの通知を有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>このホストグループ上の全サービスの通知を有効</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, COMMAND_CGI, CMD_ENABLE_HOSTGROUP_SVC_NOTIFICATIONS, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストグループ上の全サービスの通知を無効' TITLE='このホストグループ上の全サービスの通知を無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>このホストグループ上の全サービスの通知を無効</a></td></tr>\n", url_images_path, NOTIFICATIONS_DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_HOSTGROUP_SVC_NOTIFICATIONS, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストグループ上の全サービスのチェックを有効' TITLE='このホストグループ上の全サービスのチェックを有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>このホストグループ上の全サービスのチェックを有効</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_HOSTGROUP_SVC_CHECKS, url_encode(hostgroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このホストグループ上の全サービスのチェックを無効' TITLE='このホストグループ上の全サービスのチェックを無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&hostgroup=%s'>このホストグループ上の全サービスのチェックを無効</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_HOSTGROUP_SVC_CHECKS, url_encode(hostgroup_name));

		printf("</table>\n");

		printf("</TD></TR>\n");
		printf("</TABLE>\n");
		}
	else if(is_authorized_for_read_only(&current_authdata) == TRUE) {
		printf("<DIV ALIGN=CENTER CLASS='infoMessage'>あなたはコマンドを実行する権限を持っていません。<br>\n");
		}
	else {
		printf("<DIV CLASS='infoMessage'>Nagiosが動作していないため、コマンドは使えません。<br>\n");
		printf("<a href='%s?type=%d'>ここをクリック</a>してNagiosプロセス情報を確認してください。</DIV>\n", EXTINFO_CGI, DISPLAY_PROCESS_INFO);
		}

	printf("</TD></TR>\n");
	printf("<TR>\n");

	/* left bottom panel */
	printf("<TD ALIGN=CENTER VALIGN=TOP CLASS='stateInfoPanel'>\n");

	printf("</TD></TR>\n");
	printf("</TABLE>\n");
	printf("</DIV>\n");


	printf("</div>\n");

	printf("</TD>\n");



	return;
	}




void show_servicegroup_info() {
	servicegroup *temp_servicegroup;


	/* get servicegroup info */
	temp_servicegroup = find_servicegroup(servicegroup_name);

	/* make sure the user has rights to view servicegroup information */
	if(is_authorized_for_servicegroup(temp_servicegroup, &current_authdata) == FALSE) {

		printf("<P><DIV CLASS='errorMessage'>サービスグループ情報を閲覧する権限が無いようです。</DIV></P>\n");
		printf("<P><DIV CLASS='errorDescription'>このメッセージが何らかのエラーである場合はHTTPサーバのこのCGIに対するアクセス権限の設定かNagiosのCGI用設定ファイルの認証に関するオプションを調べてみてください</DIV></P>\n");

		return;
		}

	/* make sure servicegroup information exists */
	if(temp_servicegroup == NULL) {
		printf("<P><DIV CLASS='errorMessage'>エラー: サービスグループが見つかりませんでした。</DIV></P>");
		return;
		}


	printf("<DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 WIDTH=100%%>\n");
	printf("<TR>\n");


	/* top left panel */
	printf("<TD ALIGN=CENTER VALIGN=TOP CLASS='stateInfoPanel'>\n");

	/* right top panel */
	printf("</TD><TD ALIGN=CENTER VALIGN=TOP CLASS='stateInfoPanel' ROWSPAN=2>\n");

	printf("<DIV CLASS='dataTitle'>サービスグループコマンド</DIV>\n");

	if(nagios_process_state == STATE_OK) {

		printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0 CLASS='command'>\n");
		printf("<TR><TD>\n");

		printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 CLASS='command'>\n");

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスグループ上全ホストのダウンタイムをスケジュール' TITLE='このサービスグループ上全ホストのダウンタイムをスケジュール'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>このサービスグループ上全ホストのダウンタイムをスケジュール</a></td></tr>\n", url_images_path, DOWNTIME_ICON, COMMAND_CGI, CMD_SCHEDULE_SERVICEGROUP_HOST_DOWNTIME, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスグループ上全サービスのダウンタイムをスケジュール' TITLE='このサービスグループ上全サービスのダウンタイムをスケジュール'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>このサービスグループ上全サービスのダウンタイムをスケジュール</a></td></tr>\n", url_images_path, DOWNTIME_ICON, COMMAND_CGI, CMD_SCHEDULE_SERVICEGROUP_SVC_DOWNTIME, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスグループ上の全ホストの通知を有効' TITLE='このサービスグループ上の全ホストの通知を有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>このサービスグループ上の全ホストの通知を有効</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, COMMAND_CGI, CMD_ENABLE_SERVICEGROUP_HOST_NOTIFICATIONS, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスグループ上の全ホストの通知を無効' TITLE='このサービスグループ上の全ホストの通知を無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>このサービスグループ上の全ホストの通知を無効</a></td></tr>\n", url_images_path, NOTIFICATIONS_DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_SERVICEGROUP_HOST_NOTIFICATIONS, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスグループ上の全サービスの通知を有効' TITLE='このサービスグループ上の全サービスの通知を有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>このサービスグループ上の全サービスの通知を有効</a></td></tr>\n", url_images_path, NOTIFICATION_ICON, COMMAND_CGI, CMD_ENABLE_SERVICEGROUP_SVC_NOTIFICATIONS, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスグループ上の全サービスの通知を無効' TITLE='このサービスグループ上の全サービスの通知を無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>このサービスグループ上の全サービスの通知を無効</a></td></tr>\n", url_images_path, NOTIFICATIONS_DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_SERVICEGROUP_SVC_NOTIFICATIONS, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスグループ上の全サービスのチェックを有効' TITLE='このサービスグループ上の全サービスのチェックを有効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>このサービスグループ上の全サービスのチェックを有効</a></td></tr>\n", url_images_path, ENABLED_ICON, COMMAND_CGI, CMD_ENABLE_SERVICEGROUP_SVC_CHECKS, url_encode(servicegroup_name));

		printf("<tr CLASS='command'><td><img src='%s%s' border=0 ALT='このサービスグループ上の全サービスのチェックを無効' TITLE='このサービスグループ上の全サービスのチェックを無効'></td><td CLASS='command'><a href='%s?cmd_typ=%d&servicegroup=%s'>このサービスグループ上の全サービスのチェックを無効</a></td></tr>\n", url_images_path, DISABLED_ICON, COMMAND_CGI, CMD_DISABLE_SERVICEGROUP_SVC_CHECKS, url_encode(servicegroup_name));

		printf("</table>\n");

		printf("</TD></TR>\n");
		printf("</TABLE>\n");
		}
	else {
		printf("<DIV CLASS='infoMessage'>Nagiosが動作していないため、コマンドは使えません。<br>\n");
		printf("<a href='%s?type=%d'>ここをクリック</a>してNagiosプロセス情報を確認してください。</DIV>\n", EXTINFO_CGI, DISPLAY_PROCESS_INFO);
		}

	printf("</TD></TR>\n");
	printf("<TR>\n");

	/* left bottom panel */
	printf("<TD ALIGN=CENTER VALIGN=TOP CLASS='stateInfoPanel'>\n");

	printf("</TD></TR>\n");
	printf("</TABLE>\n");
	printf("</DIV>\n");


	printf("</div>\n");

	printf("</TD>\n");


	return;
	}



/* shows all service and host comments */
void show_all_comments(void) {
	int total_comments = 0;
	const char *bg_class = "";
	int odd = 0;
	char date_time[MAX_DATETIME_LENGTH];
	nagios_comment *temp_comment;
	host *temp_host;
	service *temp_service;
	char *comment_type;
	char expire_time[MAX_DATETIME_LENGTH];


	printf("<BR />\n");
	printf("<DIV CLASS='commentNav'>[&nbsp;<A HREF='#HOSTCOMMENTS' CLASS='commentNav'>ホストコメント</A>&nbsp;|&nbsp;<A HREF='#SERVICECOMMENTS' CLASS='commentNav'>サービスコメント</A>&nbsp;]</DIV>\n");
	printf("<BR />\n");

	printf("<A NAME=HOSTCOMMENTS></A>\n");
	printf("<DIV CLASS='commentTitle'>ホストコメント</DIV>\n");

	if(is_authorized_for_read_only(&current_authdata)==FALSE) {
		printf("<div CLASS='comment'><img src='%s%s' border=0>&nbsp;", url_images_path, COMMENT_ICON);
		printf("<a href='%s?cmd_typ=%d'>", COMMAND_CGI, CMD_ADD_HOST_COMMENT);
		printf("新規コメント追加</a></div>\n");
		}

	printf("<BR />\n");
	printf("<DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 CLASS='comment'>\n");
	if(is_authorized_for_read_only(&current_authdata)==FALSE)
		printf("<TR CLASS='comment'><TH CLASS='comment'>ホスト名</TH><TH CLASS='comment'>記入日</TH><TH CLASS='comment'>記入者</TH><TH CLASS='comment'>コメント</TH><TH CLASS='comment'>コメントID</TH><TH CLASS='comment'>保持設定</TH><TH CLASS='comment'>種類</TH><TH CLASS='comment'>期限</TH><TH CLASS='comment'>アクション</TH></TR>\n");
	else
		printf("<TR CLASS='comment'><TH CLASS='comment'>ホスト名</TH><TH CLASS='comment'>記入日</TH><TH CLASS='comment'>記入者</TH><TH CLASS='comment'>コメント</TH><TH CLASS='comment'>コメントID</TH><TH CLASS='comment'>保持設定</TH><TH CLASS='comment'>種類</TH><TH CLASS='comment'>期限</TH></TR>\n");

	/* display all the host comments */
	for(temp_comment = comment_list, total_comments = 0; temp_comment != NULL; temp_comment = temp_comment->next) {

		if(temp_comment->comment_type != HOST_COMMENT)
			continue;

		temp_host = find_host(temp_comment->host_name);

		/* make sure the user has rights to view host information */
		if(is_authorized_for_host(temp_host, &current_authdata) == FALSE)
			continue;

		total_comments++;

		if(odd) {
			odd = 0;
			bg_class = "commentOdd";
			}
		else {
			odd = 1;
			bg_class = "commentEven";
			}

		switch(temp_comment->entry_type) {
			case USER_COMMENT:
				comment_type = "ユーザー";
				break;
			case DOWNTIME_COMMENT:
				comment_type = "ダウンタイムスケジュール";
				break;
			case FLAPPING_COMMENT:
				comment_type = "フラップ検知";
				break;
			case ACKNOWLEDGEMENT_COMMENT:
				comment_type = "認知";
				break;
			default:
				comment_type = "未知";
			}

		get_time_string(&temp_comment->entry_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		get_time_string(&temp_comment->expire_time, expire_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<tr CLASS='%s'>", bg_class);
		printf("<td CLASS='%s'><A HREF='%s?type=%d&host=%s'>%s</A></td>", bg_class, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_comment->host_name), temp_comment->host_name);
		printf("<td CLASS='%s'>%s</td><td CLASS='%s'>%s</td><td CLASS='%s'>%s</td><td CLASS='%s'>%ld</td><td CLASS='%s'>%s</td><td CLASS='%s'>%s</td><td CLASS='%s'>%s</td>", bg_class, date_time, bg_class, temp_comment->author, bg_class, temp_comment->comment_data, bg_class, temp_comment->comment_id, bg_class, (temp_comment->persistent) ? "はい" : "いいえ", bg_class, comment_type, bg_class, (temp_comment->expires == TRUE) ? expire_time : "N/A");
		if(is_authorized_for_read_only(&current_authdata)==FALSE)
			printf("<td><a href='%s?cmd_typ=%d&com_id=%lu'><img src='%s%s' border=0 ALT='このコメントを削除' TITLE='このコメントを削除'></td>", COMMAND_CGI, CMD_DEL_HOST_COMMENT, temp_comment->comment_id, url_images_path, DELETE_ICON);
		printf("</tr>\n");
		}

	if(total_comments == 0)
		printf("<TR CLASS='commentOdd'><TD CLASS='commentOdd' COLSPAN=9>ホストのコメントはありません</TD></TR>");

	printf("</TABLE>\n");
	printf("</DIV>\n");

	printf("<BR /><BR /><BR />\n");


	printf("<A NAME=SERVICECOMMENTS></A>\n");
	printf("<DIV CLASS='commentTitle'>サービスコメント</DIV>\n");

	if(is_authorized_for_read_only(&current_authdata)==FALSE){
		printf("<div CLASS='comment'><img src='%s%s' border=0>&nbsp;", url_images_path, COMMENT_ICON);
		printf("<a href='%s?cmd_typ=%d'>", COMMAND_CGI, CMD_ADD_SVC_COMMENT);
		printf("新規コメント追加</a></div>\n");
		}

	printf("<BR />\n");
	printf("<DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 CLASS='comment'>\n");
	if(is_authorized_for_read_only(&current_authdata)==FALSE)
		printf("<TR CLASS='comment'><TH CLASS='comment'>ホスト名</TH><TH CLASS='comment'>サービス名</TH><TH CLASS='comment'>記入日</TH><TH CLASS='comment'>記入者</TH><TH CLASS='comment'>コメント</TH><TH CLASS='comment'>コメントID</TH><TH CLASS='comment'>保持設定</TH><TH CLASS='comment'>種類</TH><TH CLASS='comment'>期限</TH><TH CLASS='comment'>アクション</TH></TR>\n");
	else
		printf("<TR CLASS='comment'><TH CLASS='comment'>ホスト名</TH><TH CLASS='comment'>サービス名</TH><TH CLASS='comment'>記入日</TH><TH CLASS='comment'>記入者</TH><TH CLASS='comment'>コメント</TH><TH CLASS='comment'>コメントID</TH><TH CLASS='comment'>保持設定</TH><TH CLASS='comment'>種類</TH><TH CLASS='comment'>期限</TH></TR>\n");

	/* display all the service comments */
	for(temp_comment = comment_list, total_comments = 0; temp_comment != NULL; temp_comment = temp_comment->next) {

		if(temp_comment->comment_type != SERVICE_COMMENT)
			continue;

		temp_service = find_service(temp_comment->host_name, temp_comment->service_description);

		/* make sure the user has rights to view service information */
		if(is_authorized_for_service(temp_service, &current_authdata) == FALSE)
			continue;

		total_comments++;

		if(odd) {
			odd = 0;
			bg_class = "commentOdd";
			}
		else {
			odd = 1;
			bg_class = "commentEven";
			}

		switch(temp_comment->entry_type) {
			case USER_COMMENT:
				comment_type = "ユーザー";
				break;
			case DOWNTIME_COMMENT:
				comment_type = "ダウンタイムスケジュール";
				break;
			case FLAPPING_COMMENT:
				comment_type = "フラップ検知";
				break;
			case ACKNOWLEDGEMENT_COMMENT:
				comment_type = "認知済";
				break;
			default:
				comment_type = "未知";
			}

		get_time_string(&temp_comment->entry_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		get_time_string(&temp_comment->expire_time, expire_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<tr CLASS='%s'>", bg_class);
		printf("<td CLASS='%s'><A HREF='%s?type=%d&host=%s'>%s</A></td>", bg_class, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_comment->host_name), temp_comment->host_name);
		printf("<td CLASS='%s'><A HREF='%s?type=%d&host=%s", bg_class, EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_comment->host_name));
		printf("&service=%s'>%s</A></td>", url_encode(temp_comment->service_description), temp_comment->service_description);
		printf("<td CLASS='%s'>%s</td><td CLASS='%s'>%s</td><td CLASS='%s'>%s</td><td CLASS='%s'>%ld</td><td CLASS='%s'>%s</td><td CLASS='%s'>%s</td><td CLASS='%s'>%s</td>", bg_class, date_time, bg_class, temp_comment->author, bg_class, temp_comment->comment_data, bg_class, temp_comment->comment_id, bg_class, (temp_comment->persistent) ? "はい" : "いいえ", bg_class, comment_type, bg_class, (temp_comment->expires == TRUE) ? expire_time : "N/A");
		if(is_authorized_for_read_only(&current_authdata)==FALSE)
			printf("<td><a href='%s?cmd_typ=%d&com_id=%ld'><img src='%s%s' border=0 ALT='このコメントを削除' TITLE='このコメントを削除'></td>", COMMAND_CGI, CMD_DEL_SVC_COMMENT, temp_comment->comment_id, url_images_path, DELETE_ICON);
		printf("</tr>\n");
		}

	if(total_comments == 0)
		printf("<TR CLASS='commentOdd'><TD CLASS='commentOdd' COLSPAN=10>サービスのコメントはありません</TD></TR>");

	printf("</TABLE>\n");
	printf("</DIV>\n");

	return;
	}



void show_performance_data(void) {
	service *temp_service = NULL;
	servicestatus *temp_servicestatus = NULL;
	host *temp_host = NULL;
	hoststatus *temp_hoststatus = NULL;
	int total_active_service_checks = 0;
	int total_passive_service_checks = 0;
	double min_service_execution_time = 0.0;
	double max_service_execution_time = 0.0;
	double total_service_execution_time = 0.0;
	int have_min_service_execution_time = FALSE;
	int have_max_service_execution_time = FALSE;
	double min_service_latency = 0.0;
	double max_service_latency = 0.0;
	double long total_service_latency = 0.0;
	int have_min_service_latency = FALSE;
	int have_max_service_latency = FALSE;
	double min_host_latency = 0.0;
	double max_host_latency = 0.0;
	double total_host_latency = 0.0;
	int have_min_host_latency = FALSE;
	int have_max_host_latency = FALSE;
	double min_service_percent_change_a = 0.0;
	double max_service_percent_change_a = 0.0;
	double total_service_percent_change_a = 0.0;
	int have_min_service_percent_change_a = FALSE;
	int have_max_service_percent_change_a = FALSE;
	double min_service_percent_change_b = 0.0;
	double max_service_percent_change_b = 0.0;
	double total_service_percent_change_b = 0.0;
	int have_min_service_percent_change_b = FALSE;
	int have_max_service_percent_change_b = FALSE;
	int active_service_checks_1min = 0;
	int active_service_checks_5min = 0;
	int active_service_checks_15min = 0;
	int active_service_checks_1hour = 0;
	int active_service_checks_start = 0;
	int active_service_checks_ever = 0;
	int passive_service_checks_1min = 0;
	int passive_service_checks_5min = 0;
	int passive_service_checks_15min = 0;
	int passive_service_checks_1hour = 0;
	int passive_service_checks_start = 0;
	int passive_service_checks_ever = 0;
	int total_active_host_checks = 0;
	int total_passive_host_checks = 0;
	double min_host_execution_time = 0.0;
	double max_host_execution_time = 0.0;
	double total_host_execution_time = 0.0;
	int have_min_host_execution_time = FALSE;
	int have_max_host_execution_time = FALSE;
	double min_host_percent_change_a = 0.0;
	double max_host_percent_change_a = 0.0;
	double total_host_percent_change_a = 0.0;
	int have_min_host_percent_change_a = FALSE;
	int have_max_host_percent_change_a = FALSE;
	double min_host_percent_change_b = 0.0;
	double max_host_percent_change_b = 0.0;
	double total_host_percent_change_b = 0.0;
	int have_min_host_percent_change_b = FALSE;
	int have_max_host_percent_change_b = FALSE;
	int active_host_checks_1min = 0;
	int active_host_checks_5min = 0;
	int active_host_checks_15min = 0;
	int active_host_checks_1hour = 0;
	int active_host_checks_start = 0;
	int active_host_checks_ever = 0;
	int passive_host_checks_1min = 0;
	int passive_host_checks_5min = 0;
	int passive_host_checks_15min = 0;
	int passive_host_checks_1hour = 0;
	int passive_host_checks_start = 0;
	int passive_host_checks_ever = 0;
	time_t current_time;


	time(&current_time);

	/* check all services */
	for(temp_servicestatus = servicestatus_list; temp_servicestatus != NULL; temp_servicestatus = temp_servicestatus->next) {

		/* find the service */
		temp_service = find_service(temp_servicestatus->host_name, temp_servicestatus->description);

		/* make sure the user has rights to view service information */
		if(is_authorized_for_service(temp_service, &current_authdata) == FALSE)
			continue;

		/* is this an active or passive check? */
		if(temp_servicestatus->check_type == CHECK_TYPE_ACTIVE) {

			total_active_service_checks++;

			total_service_execution_time += temp_servicestatus->execution_time;
			if(have_min_service_execution_time == FALSE || temp_servicestatus->execution_time < min_service_execution_time) {
				have_min_service_execution_time = TRUE;
				min_service_execution_time = temp_servicestatus->execution_time;
				}
			if(have_max_service_execution_time == FALSE || temp_servicestatus->execution_time > max_service_execution_time) {
				have_max_service_execution_time = TRUE;
				max_service_execution_time = temp_servicestatus->execution_time;
				}

			total_service_percent_change_a += temp_servicestatus->percent_state_change;
			if(have_min_service_percent_change_a == FALSE || temp_servicestatus->percent_state_change < min_service_percent_change_a) {
				have_min_service_percent_change_a = TRUE;
				min_service_percent_change_a = temp_servicestatus->percent_state_change;
				}
			if(have_max_service_percent_change_a == FALSE || temp_servicestatus->percent_state_change > max_service_percent_change_a) {
				have_max_service_percent_change_a = TRUE;
				max_service_percent_change_a = temp_servicestatus->percent_state_change;
				}

			total_service_latency += temp_servicestatus->latency;
			if(have_min_service_latency == FALSE || temp_servicestatus->latency < min_service_latency) {
				have_min_service_latency = TRUE;
				min_service_latency = temp_servicestatus->latency;
				}
			if(have_max_service_latency == FALSE || temp_servicestatus->latency > max_service_latency) {
				have_max_service_latency = TRUE;
				max_service_latency = temp_servicestatus->latency;
				}

			if(temp_servicestatus->last_check >= (current_time - 60))
				active_service_checks_1min++;
			if(temp_servicestatus->last_check >= (current_time - 300))
				active_service_checks_5min++;
			if(temp_servicestatus->last_check >= (current_time - 900))
				active_service_checks_15min++;
			if(temp_servicestatus->last_check >= (current_time - 3600))
				active_service_checks_1hour++;
			if(temp_servicestatus->last_check >= program_start)
				active_service_checks_start++;
			if(temp_servicestatus->last_check != (time_t)0)
				active_service_checks_ever++;
			}

		else {
			total_passive_service_checks++;

			total_service_percent_change_b += temp_servicestatus->percent_state_change;
			if(have_min_service_percent_change_b == FALSE || temp_servicestatus->percent_state_change < min_service_percent_change_b) {
				have_min_service_percent_change_b = TRUE;
				min_service_percent_change_b = temp_servicestatus->percent_state_change;
				}
			if(have_max_service_percent_change_b == FALSE || temp_servicestatus->percent_state_change > max_service_percent_change_b) {
				have_max_service_percent_change_b = TRUE;
				max_service_percent_change_b = temp_servicestatus->percent_state_change;
				}

			if(temp_servicestatus->last_check >= (current_time - 60))
				passive_service_checks_1min++;
			if(temp_servicestatus->last_check >= (current_time - 300))
				passive_service_checks_5min++;
			if(temp_servicestatus->last_check >= (current_time - 900))
				passive_service_checks_15min++;
			if(temp_servicestatus->last_check >= (current_time - 3600))
				passive_service_checks_1hour++;
			if(temp_servicestatus->last_check >= program_start)
				passive_service_checks_start++;
			if(temp_servicestatus->last_check != (time_t)0)
				passive_service_checks_ever++;
			}
		}

	/* check all hosts */
	for(temp_hoststatus = hoststatus_list; temp_hoststatus != NULL; temp_hoststatus = temp_hoststatus->next) {

		/* find the host */
		temp_host = find_host(temp_hoststatus->host_name);

		/* make sure the user has rights to view host information */
		if(is_authorized_for_host(temp_host, &current_authdata) == FALSE)
			continue;

		/* is this an active or passive check? */
		if(temp_hoststatus->check_type == CHECK_TYPE_ACTIVE) {

			total_active_host_checks++;

			total_host_execution_time += temp_hoststatus->execution_time;
			if(have_min_host_execution_time == FALSE || temp_hoststatus->execution_time < min_host_execution_time) {
				have_min_host_execution_time = TRUE;
				min_host_execution_time = temp_hoststatus->execution_time;
				}
			if(have_max_host_execution_time == FALSE || temp_hoststatus->execution_time > max_host_execution_time) {
				have_max_host_execution_time = TRUE;
				max_host_execution_time = temp_hoststatus->execution_time;
				}

			total_host_percent_change_a += temp_hoststatus->percent_state_change;
			if(have_min_host_percent_change_a == FALSE || temp_hoststatus->percent_state_change < min_host_percent_change_a) {
				have_min_host_percent_change_a = TRUE;
				min_host_percent_change_a = temp_hoststatus->percent_state_change;
				}
			if(have_max_host_percent_change_a == FALSE || temp_hoststatus->percent_state_change > max_host_percent_change_a) {
				have_max_host_percent_change_a = TRUE;
				max_host_percent_change_a = temp_hoststatus->percent_state_change;
				}

			total_host_latency += temp_hoststatus->latency;
			if(have_min_host_latency == FALSE || temp_hoststatus->latency < min_host_latency) {
				have_min_host_latency = TRUE;
				min_host_latency = temp_hoststatus->latency;
				}
			if(have_max_host_latency == FALSE || temp_hoststatus->latency > max_host_latency) {
				have_max_host_latency = TRUE;
				max_host_latency = temp_hoststatus->latency;
				}

			if(temp_hoststatus->last_check >= (current_time - 60))
				active_host_checks_1min++;
			if(temp_hoststatus->last_check >= (current_time - 300))
				active_host_checks_5min++;
			if(temp_hoststatus->last_check >= (current_time - 900))
				active_host_checks_15min++;
			if(temp_hoststatus->last_check >= (current_time - 3600))
				active_host_checks_1hour++;
			if(temp_hoststatus->last_check >= program_start)
				active_host_checks_start++;
			if(temp_hoststatus->last_check != (time_t)0)
				active_host_checks_ever++;
			}

		else {
			total_passive_host_checks++;

			total_host_percent_change_b += temp_hoststatus->percent_state_change;
			if(have_min_host_percent_change_b == FALSE || temp_hoststatus->percent_state_change < min_host_percent_change_b) {
				have_min_host_percent_change_b = TRUE;
				min_host_percent_change_b = temp_hoststatus->percent_state_change;
				}
			if(have_max_host_percent_change_b == FALSE || temp_hoststatus->percent_state_change > max_host_percent_change_b) {
				have_max_host_percent_change_b = TRUE;
				max_host_percent_change_b = temp_hoststatus->percent_state_change;
				}

			if(temp_hoststatus->last_check >= (current_time - 60))
				passive_host_checks_1min++;
			if(temp_hoststatus->last_check >= (current_time - 300))
				passive_host_checks_5min++;
			if(temp_hoststatus->last_check >= (current_time - 900))
				passive_host_checks_15min++;
			if(temp_hoststatus->last_check >= (current_time - 3600))
				passive_host_checks_1hour++;
			if(temp_hoststatus->last_check >= program_start)
				passive_host_checks_start++;
			if(temp_hoststatus->last_check != (time_t)0)
				passive_host_checks_ever++;
			}
		}


	printf("<div align=center>\n");


	printf("<DIV CLASS='dataTitle'>監視のパフォーマンス情報</DIV>\n");

	printf("<table border='0' cellpadding='10'>\n");


	/***** ACTIVE SERVICE CHECKS *****/

	printf("<tr>\n");
	printf("<td valign=middle><div class='perfTypeTitle'>サービスの動作チェック:</div></td>\n");
	printf("<td valign=top>\n");

	/* fake this so we don't divide by zero for just showing the table */
	if(total_active_service_checks == 0)
		total_active_service_checks = 1;

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable1'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>間隔</th><th class='data'>件数(率)</th></tr>\n");
	printf("<tr><td class='dataVar'>&lt;= 1分(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_service_checks_1min, (double)(((double)active_service_checks_1min * 100.0) / (double)total_active_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 5分(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_service_checks_5min, (double)(((double)active_service_checks_5min * 100.0) / (double)total_active_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 15分(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_service_checks_15min, (double)(((double)active_service_checks_15min * 100.0) / (double)total_active_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 1時間(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_service_checks_1hour, (double)(((double)active_service_checks_1hour * 100.0) / (double)total_active_service_checks));
	printf("<tr><td class='dataVar'>プログラム開始以来:&nbsp;&nbsp;</td><td class='dataVal'>%d (%.1f%%)</td>", active_service_checks_start, (double)(((double)active_service_checks_start * 100.0) / (double)total_active_service_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td><td valign=top>\n");

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable2'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>メトリック</th><th class='data'>最小</th><th class='data'>最大</th><th class='data'>平均</th></tr>\n");

	printf("<tr><td class='dataVar'>チェック実行時間:&nbsp;&nbsp;</td><td class='dataVal'>%.2f 秒</td><td class='dataVal'>%.2f 秒</td><td class='dataVal'>%.3f 秒</td></tr>\n", min_service_execution_time, max_service_execution_time, (double)((double)total_service_execution_time / (double)total_active_service_checks));

	printf("<tr><td class='dataVar'>チェック遅延:</td><td class='dataVal'>%.2f 秒</td><td class='dataVal'>%.2f 秒</td><td class='dataVal'>%.3f 秒</td></tr>\n", min_service_latency, max_service_latency, (double)((double)total_service_latency / (double)total_active_service_checks));

	printf("<tr><td class='dataVar'>状態変化率:</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td></tr>\n", min_service_percent_change_a, max_service_percent_change_a, (double)((double)total_service_percent_change_a / (double)total_active_service_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");


	printf("</td>\n");
	printf("</tr>\n");


	/***** PASSIVE SERVICE CHECKS *****/

	printf("<tr>\n");
	printf("<td valign=middle><div class='perfTypeTitle'>サービスのパッシブチェック:</div></td>\n");
	printf("<td valign=top>\n");


	/* fake this so we don't divide by zero for just showing the table */
	if(total_passive_service_checks == 0)
		total_passive_service_checks = 1;

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable1'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>間隔</th><th class='data'>件数(率)</th></tr>\n");
	printf("<tr><td class='dataVar'>&lt;= 1分(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_1min, (double)(((double)passive_service_checks_1min * 100.0) / (double)total_passive_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 5分(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_5min, (double)(((double)passive_service_checks_5min * 100.0) / (double)total_passive_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 15分(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_15min, (double)(((double)passive_service_checks_15min * 100.0) / (double)total_passive_service_checks));
	printf("<tr><td class='dataVar'>&lt;= 1時間(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_1hour, (double)(((double)passive_service_checks_1hour * 100.0) / (double)total_passive_service_checks));
	printf("<tr><td class='dataVar'>プログラム開始以来:&nbsp;&nbsp;</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_service_checks_start, (double)(((double)passive_service_checks_start * 100.0) / (double)total_passive_service_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td><td valign=top>\n");

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable2'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>メトリック</th><th class='data'>最小</th><th class='data'>最大</th><th class='data'>平均</th></tr>\n");
	printf("<tr><td class='dataVar'>状態変化率:&nbsp;&nbsp;</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td></tr>\n", min_service_percent_change_b, max_service_percent_change_b, (double)((double)total_service_percent_change_b / (double)total_passive_service_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td>\n");
	printf("</tr>\n");


	/***** ACTIVE HOST CHECKS *****/

	printf("<tr>\n");
	printf("<td valign=middle><div class='perfTypeTitle'>ホストの動作チェック:</div></td>\n");
	printf("<td valign=top>\n");

	/* fake this so we don't divide by zero for just showing the table */
	if(total_active_host_checks == 0)
		total_active_host_checks = 1;

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable1'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>間隔</th><th class='data'>件数(率)</th></tr>\n");
	printf("<tr><td class='dataVar'>&lt;= 1分(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_host_checks_1min, (double)(((double)active_host_checks_1min * 100.0) / (double)total_active_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 5分(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_host_checks_5min, (double)(((double)active_host_checks_5min * 100.0) / (double)total_active_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 15分(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_host_checks_15min, (double)(((double)active_host_checks_15min * 100.0) / (double)total_active_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 1時間(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", active_host_checks_1hour, (double)(((double)active_host_checks_1hour * 100.0) / (double)total_active_host_checks));
	printf("<tr><td class='dataVar'>プログラム開始以来:&nbsp;&nbsp;</td><td class='dataVal'>%d (%.1f%%)</td>", active_host_checks_start, (double)(((double)active_host_checks_start * 100.0) / (double)total_active_host_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td><td valign=top>\n");

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable2'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>メトリック</th><th class='data'>最小</th><th class='data'>最大</th><th class='data'>平均</th></tr>\n");

	printf("<tr><td class='dataVar'>チェック実行時間:&nbsp;&nbsp;</td><td class='dataVal'>%.2f 秒</td><td class='dataVal'>%.2f 秒</td><td class='dataVal'>%.3f 秒</td></tr>\n", min_host_execution_time, max_host_execution_time, (double)((double)total_host_execution_time / (double)total_active_host_checks));

	printf("<tr><td class='dataVar'>チェック遅延:</td><td class='dataVal'>%.2f 秒</td><td class='dataVal'>%.2f 秒</td><td class='dataVal'>%.3f 秒</td></tr>\n", min_host_latency, max_host_latency, (double)((double)total_host_latency / (double)total_active_host_checks));

	printf("<tr><td class='dataVar'>状態変化率:</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td></tr>\n", min_host_percent_change_a, max_host_percent_change_a, (double)((double)total_host_percent_change_a / (double)total_active_host_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");


	printf("</td>\n");
	printf("</tr>\n");


	/***** PASSIVE HOST CHECKS *****/

	printf("<tr>\n");
	printf("<td valign=middle><div class='perfTypeTitle'>ホストのパッシブチェック:</div></td>\n");
	printf("<td valign=top>\n");


	/* fake this so we don't divide by zero for just showing the table */
	if(total_passive_host_checks == 0)
		total_passive_host_checks = 1;

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable1'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>間隔</th><th class='data'>件数(率)</th></tr>\n");
	printf("<tr><td class='dataVar'>&lt;= 1分(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_1min, (double)(((double)passive_host_checks_1min * 100.0) / (double)total_passive_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 5分(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_5min, (double)(((double)passive_host_checks_5min * 100.0) / (double)total_passive_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 15分(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_15min, (double)(((double)passive_host_checks_15min * 100.0) / (double)total_passive_host_checks));
	printf("<tr><td class='dataVar'>&lt;= 1時間(以前):</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_1hour, (double)(((double)passive_host_checks_1hour * 100.0) / (double)total_passive_host_checks));
	printf("<tr><td class='dataVar'>プログラム開始以来:&nbsp;&nbsp;</td><td class='dataVal'>%d (%.1f%%)</td></tr>", passive_host_checks_start, (double)(((double)passive_host_checks_start * 100.0) / (double)total_passive_host_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td><td valign=top>\n");

	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable2'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>メトリック</th><th class='data'>最小</th><th class='data'>最大</th><th class='data'>平均</th></tr>\n");
	printf("<tr><td class='dataVar'>状態変化率:&nbsp;&nbsp;</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td><td class='dataVal'>%.2f%%</td></tr>\n", min_host_percent_change_b, max_host_percent_change_b, (double)((double)total_host_percent_change_b / (double)total_passive_host_checks));

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td>\n");
	printf("</tr>\n");



	/***** CHECK STATS *****/

	printf("<tr>\n");
	printf("<td valign=center><div class='perfTypeTitle'>統計データ:</div></td>\n");
	printf("<td valign=top colspan='2'>\n");


	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable1'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>タイプ</th><th class='data'>1分前</th><th class='data'>5分前</th><th class='data'>15分前</th></tr>\n");
	printf("<tr><td class='dataVar'>スケジュールホストチェック</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[ACTIVE_SCHEDULED_HOST_CHECK_STATS][0], program_stats[ACTIVE_SCHEDULED_HOST_CHECK_STATS][1], program_stats[ACTIVE_SCHEDULED_HOST_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>オンデマンドホストチェック</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[ACTIVE_ONDEMAND_HOST_CHECK_STATS][0], program_stats[ACTIVE_ONDEMAND_HOST_CHECK_STATS][1], program_stats[ACTIVE_ONDEMAND_HOST_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>並列ホストチェック</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[PARALLEL_HOST_CHECK_STATS][0], program_stats[PARALLEL_HOST_CHECK_STATS][1], program_stats[PARALLEL_HOST_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>連続ホストチェック</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[SERIAL_HOST_CHECK_STATS][0], program_stats[SERIAL_HOST_CHECK_STATS][1], program_stats[SERIAL_HOST_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>キャッシュホストチェック</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[ACTIVE_CACHED_HOST_CHECK_STATS][0], program_stats[ACTIVE_CACHED_HOST_CHECK_STATS][1], program_stats[ACTIVE_CACHED_HOST_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>パッシブホストチェック</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[PASSIVE_HOST_CHECK_STATS][0], program_stats[PASSIVE_HOST_CHECK_STATS][1], program_stats[PASSIVE_HOST_CHECK_STATS][2]);

	printf("<tr><td class='dataVar'>スケジュールサービスチェック</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[ACTIVE_SCHEDULED_SERVICE_CHECK_STATS][0], program_stats[ACTIVE_SCHEDULED_SERVICE_CHECK_STATS][1], program_stats[ACTIVE_SCHEDULED_SERVICE_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>オンデマンドサービスチェック</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[ACTIVE_ONDEMAND_SERVICE_CHECK_STATS][0], program_stats[ACTIVE_ONDEMAND_SERVICE_CHECK_STATS][1], program_stats[ACTIVE_ONDEMAND_SERVICE_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>キャッシュサービスチェック</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[ACTIVE_CACHED_SERVICE_CHECK_STATS][0], program_stats[ACTIVE_CACHED_SERVICE_CHECK_STATS][1], program_stats[ACTIVE_CACHED_SERVICE_CHECK_STATS][2]);
	printf("<tr><td class='dataVar'>パッシブサービスチェック</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[PASSIVE_SERVICE_CHECK_STATS][0], program_stats[PASSIVE_SERVICE_CHECK_STATS][1], program_stats[PASSIVE_SERVICE_CHECK_STATS][2]);

	printf("<tr><td class='dataVar'>外部コマンド</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", program_stats[EXTERNAL_COMMAND_STATS][0], program_stats[EXTERNAL_COMMAND_STATS][1], program_stats[EXTERNAL_COMMAND_STATS][2]);

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td>\n");
	printf("</tr>\n");



	/***** BUFFER STATS *****/

	printf("<tr>\n");
	printf("<td valign=center><div class='perfTypeTitle'>バッファリング:</div></td>\n");
	printf("<td valign=top colspan='2'>\n");


	printf("<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=0>\n");
	printf("<TR><TD class='stateInfoTable1'>\n");
	printf("<TABLE BORDER=0>\n");

	printf("<tr class='data'><th class='data'>種類</th><th class='data'>使用数</th><th class='data'>最大使用可能数</th><th class='data'>合計</th></tr>\n");
	printf("<tr><td class='dataVar'>外部コマンド&nbsp;</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td><td class='dataVal'>%d</td></tr>", buffer_stats[0][1], buffer_stats[0][2], buffer_stats[0][0]);

	printf("</TABLE>\n");
	printf("</TD></TR>\n");
	printf("</TABLE>\n");

	printf("</td>\n");
	printf("</tr>\n");



	printf("</table>\n");


	printf("</div>\n");

	return;
	}



void display_comments(int type) {
	host *temp_host = NULL;
	service *temp_service = NULL;
	int total_comments = 0;
	int display_comment = FALSE;
	const char *bg_class = "";
	int odd = 1;
	char date_time[MAX_DATETIME_LENGTH];
	nagios_comment *temp_comment;
	scheduled_downtime *temp_downtime;
	char *comment_type;
	char expire_time[MAX_DATETIME_LENGTH];


	/* find the host or service */
	if(type == HOST_COMMENT) {
		temp_host = find_host(host_name);
		if(temp_host == NULL)
			return;
		}
	else {
		temp_service = find_service(host_name, service_desc);
		if(temp_service == NULL)
			return;
		}


	printf("<A NAME=comments></A>\n");
	printf("<DIV CLASS='commentTitle'>%sコメント</DIV>\n", (type == HOST_COMMENT) ? "ホスト" : "サービス");

	if(is_authorized_for_read_only(&current_authdata)==FALSE){
		printf("<TABLE BORDER=0>\n");

		printf("<tr>\n");
		printf("<td valign=middle><img src='%s%s' border=0 align=center></td><td CLASS='comment'>", url_images_path, COMMENT_ICON);
		if(type == HOST_COMMENT)
			printf("<a href='%s?cmd_typ=%d&host=%s' CLASS='comment'>", COMMAND_CGI, CMD_ADD_HOST_COMMENT, url_encode(host_name));
		else {
			printf("<a href='%s?cmd_typ=%d&host=%s&", COMMAND_CGI, CMD_ADD_SVC_COMMENT, url_encode(host_name));
			printf("service=%s' CLASS='comment'>", url_encode(service_desc));
			}
		printf("コメントを追加する</a></td>\n");

		printf("<td valign=middle><img src='%s%s' border=0 align=center></td><td CLASS='comment'>", url_images_path, DELETE_ICON);
		if(type == HOST_COMMENT)
			printf("<a href='%s?cmd_typ=%d&host=%s' CLASS='comment'>", COMMAND_CGI, CMD_DEL_ALL_HOST_COMMENTS, url_encode(host_name));
		else {
			printf("<a href='%s?cmd_typ=%d&host=%s&", COMMAND_CGI, CMD_DEL_ALL_SVC_COMMENTS, url_encode(host_name));
			printf("service=%s' CLASS='comment'>", url_encode(service_desc));
			}
		printf("全てのコメントを削除する</a></td>\n");
		printf("</tr>\n");

		printf("</TABLE>\n");
		}


	printf("<DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 CLASS='comment'>\n");
	if(is_authorized_for_read_only(&current_authdata)==FALSE)
		printf("<TR CLASS='comment'><TH CLASS='comment'>記入日</TH><TH CLASS='comment'>記入者</TH><TH CLASS='comment'>コメント</TH><TH CLASS='comment'>コメントID</TH><TH CLASS='comment'>保持設定</TH><TH CLASS='comment'>種類</TH><TH CLASS='comment'>期限</TH><TH CLASS='comment'>アクション</TH></TR>\n");
	else
		printf("<TR CLASS='comment'><TH CLASS='comment'>記入日</TH><TH CLASS='comment'>記入者</TH><TH CLASS='comment'>コメント</TH><TH CLASS='comment'>コメントID</TH><TH CLASS='comment'>保持設定</TH><TH CLASS='comment'>種類</TH><TH CLASS='comment'>期限</TH></TR>\n");

	/* check all the comments to see if they apply to this host or service */
	/* Comments are displayed in the order they are read from the status.dat file */
	for(temp_comment = get_first_comment_by_host(host_name); temp_comment != NULL; temp_comment = get_next_comment_by_host(host_name, temp_comment)) {

		display_comment = FALSE;

		if(type == HOST_COMMENT && temp_comment->comment_type == HOST_COMMENT)
			display_comment = TRUE;

		else if(type == SERVICE_COMMENT && temp_comment->comment_type == SERVICE_COMMENT && !strcmp(temp_comment->service_description, service_desc))
			display_comment = TRUE;

		if(display_comment == TRUE) {

			if(odd) {
				odd = 0;
				bg_class = "commentOdd";
				}
			else {
				odd = 1;
				bg_class = "commentEven";
				}

			switch(temp_comment->entry_type) {
				case USER_COMMENT:
					comment_type = "ユーザー";
					break;
				case DOWNTIME_COMMENT:
					comment_type = "ダウンタイムスケジュール";
					break;
				case FLAPPING_COMMENT:
					comment_type = "フラップ検知";
					break;
				case ACKNOWLEDGEMENT_COMMENT:
					comment_type = "認知済";
					break;
				default:
					comment_type = "未知";
				}

			if (temp_comment->entry_type == DOWNTIME_COMMENT) {
				for(temp_downtime = scheduled_downtime_list; temp_downtime != NULL; temp_downtime = temp_downtime->next) {
					if (temp_downtime->comment_id == temp_comment->comment_id)
						break;
					}
				}
			else
				temp_downtime = NULL;

			get_time_string(&temp_comment->entry_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			get_time_string(&temp_comment->expire_time, expire_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<tr CLASS='%s'>", bg_class);
			printf("<td CLASS='%s'>%s</td><td CLASS='%s'>%s</td>", bg_class, date_time, bg_class, temp_comment->author);
			printf("<td CLASS='%s'>%s", bg_class, temp_comment->comment_data);
			if (temp_downtime)
				printf("<hr>%s", temp_downtime->comment);
			printf("</td><td CLASS='%s'>%lu</td><td CLASS='%s'>%s</td><td CLASS='%s'>%s</td><td CLASS='%s'>%s</td>",
				bg_class, temp_comment->comment_id, bg_class, (temp_comment->persistent) ? "はい" : "いいえ",
				bg_class, comment_type, bg_class, (temp_comment->expires == TRUE) ? expire_time : "N/A");
			if(is_authorized_for_read_only(&current_authdata)==FALSE)
				printf("<td><a href='%s?cmd_typ=%d&com_id=%lu'><img src='%s%s' border=0 ALT='このコメントを削除' TITLE='このコメントを削除'></td>", COMMAND_CGI, (type == HOST_COMMENT) ? CMD_DEL_HOST_COMMENT : CMD_DEL_SVC_COMMENT, temp_comment->comment_id, url_images_path, DELETE_ICON);
			printf("</tr>\n");

			total_comments++;

			}
		}

	/* see if this host or service has any comments associated with it */
	if(total_comments == 0)
		printf("<TR CLASS='commentOdd'><TD CLASS='commentOdd' COLSPAN='%d'>この%sにはコメントがありません</TD></TR>", (type == HOST_COMMENT) ? 9 : 10, (type == HOST_COMMENT) ? "ホスト" : "サービス");

	printf("</TABLE></DIV>\n");

	return;
	}




/* shows all service and host scheduled downtime */
void show_all_downtime(void) {
	int total_downtime = 0;
	const char *bg_class = "";
	int odd = 0;
	char date_time[MAX_DATETIME_LENGTH];
	scheduled_downtime *temp_downtime;
	host *temp_host;
	service *temp_service;
	int days;
	int hours;
	int minutes;
	int seconds;


	printf("<BR />\n");
	printf("<DIV CLASS='downtimeNav'>[&nbsp;<A HREF='#HOSTDOWNTIME' CLASS='downtimeNav'>ホストのダウンタイム</A>&nbsp;|&nbsp;<A HREF='#SERVICEDOWNTIME' CLASS='downtimeNav'>サービスのダウンタイム</A>&nbsp;]</DIV>\n");
	printf("<BR />\n");

	printf("<A NAME=HOSTDOWNTIME></A>\n");
	printf("<DIV CLASS='downtimeTitle'>ホストのダウンタイムのスケジュール設定</DIV>\n");

	printf("<div CLASS='comment'><img src='%s%s' border=0>&nbsp;", url_images_path, DOWNTIME_ICON);
	printf("<a href='%s?cmd_typ=%d'>", COMMAND_CGI, CMD_SCHEDULE_HOST_DOWNTIME);
	printf("ホストのダウンタイムをスケジュールする</a></div>\n");

	printf("<BR />\n");
	printf("<DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 CLASS='downtime'>\n");
	printf("<TR CLASS='downtime'><TH CLASS='downtime'>ホスト名</TH><TH CLASS='downtime'>登録日</TH><TH CLASS='downtime'>登録者</TH><TH CLASS='downtime'>コメント</TH><TH CLASS='downtime'>開始時間</TH><TH CLASS='downtime'>終了時間</TH><TH CLASS='downtime'>種類</TH><TH CLASS='downtime'>期間</TH><TH CLASS='downtime'>ダウンタイムID</TH><TH CLASS='downtime'>トリガーID</TH><TH CLASS='downtime'>アクション</TH></TR>\n");

	/* display all the host downtime */
	for(temp_downtime = scheduled_downtime_list, total_downtime = 0; temp_downtime != NULL; temp_downtime = temp_downtime->next) {

		if(temp_downtime->type != HOST_DOWNTIME)
			continue;

		temp_host = find_host(temp_downtime->host_name);

		/* make sure the user has rights to view host information */
		if(is_authorized_for_host(temp_host, &current_authdata) == FALSE)
			continue;

		total_downtime++;

		if(odd) {
			odd = 0;
			bg_class = "downtimeOdd";
			}
		else {
			odd = 1;
			bg_class = "downtimeEven";
			}

		printf("<tr CLASS='%s'>", bg_class);
		printf("<td CLASS='%s'><A HREF='%s?type=%d&host=%s'>%s</A></td>", bg_class, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_downtime->host_name), temp_downtime->host_name);
		get_time_string(&temp_downtime->entry_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<td CLASS='%s'>%s</td>", bg_class, date_time);
		printf("<td CLASS='%s'>%s</td>", bg_class, (temp_downtime->author == NULL) ? "N/A" : temp_downtime->author);
		printf("<td CLASS='%s'>%s</td>", bg_class, (temp_downtime->comment == NULL) ? "N/A" : temp_downtime->comment);
		get_time_string(&temp_downtime->start_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<td CLASS='%s'>%s</td>", bg_class, date_time);
		get_time_string(&temp_downtime->end_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<td CLASS='%s'>%s</td>", bg_class, date_time);
		printf("<td CLASS='%s'>%s</td>", bg_class, (temp_downtime->fixed == TRUE) ? "固定" : "非固定");
		get_time_breakdown(temp_downtime->duration, &days, &hours, &minutes, &seconds);
		printf("<td CLASS='%s'>%d日間と %d時間 %d分 %d秒</td>", bg_class, days, hours, minutes, seconds);
		printf("<td CLASS='%s'>%lu</td>", bg_class, temp_downtime->downtime_id);
		printf("<td CLASS='%s'>", bg_class);
		if(temp_downtime->triggered_by == 0)
			printf("N/A");
		else
			printf("%lu", temp_downtime->triggered_by);
		printf("</td>\n");
		printf("<td><a href='%s?cmd_typ=%d&down_id=%lu'><img src='%s%s' border=0 ALT='このスケジュールエントリを削除もしくはキャンセル' TITLE='このスケジュールエントリを削除もしくはキャンセル'></td>", COMMAND_CGI, CMD_DEL_HOST_DOWNTIME, temp_downtime->downtime_id, url_images_path, DELETE_ICON);
		printf("</tr>\n");
		}

	if(total_downtime == 0)
		printf("<TR CLASS='downtimeOdd'><TD CLASS='downtimeOdd' COLSPAN=11>ダウンタイムがスケジュールされたホストはありません</TD></TR>");

	printf("</TABLE>\n");
	printf("</DIV>\n");

	printf("<BR /><BR /><BR />\n");


	printf("<A NAME=SERVICEDOWNTIME></A>\n");
	printf("<DIV CLASS='downtimeTitle'>サービスのダウンタイムのスケジュール設定</DIV>\n");

	printf("<div CLASS='comment'><img src='%s%s' border=0>&nbsp;", url_images_path, DOWNTIME_ICON);
	printf("<a href='%s?cmd_typ=%d'>", COMMAND_CGI, CMD_SCHEDULE_SVC_DOWNTIME);
	printf("サービスのダウンタイムをスケジュールする</a></div>\n");

	printf("<BR />\n");
	printf("<DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 CLASS='downtime'>\n");
	printf("<TR CLASS='downtime'><TH CLASS='downtime'>ホスト名</TH><TH CLASS='downtime'>サービス名</TH><TH CLASS='downtime'>登録日</TH><TH CLASS='downtime'>登録者</TH><TH CLASS='downtime'>コメント</TH><TH CLASS='downtime'>開始時間</TH><TH CLASS='downtime'>終了時間</TH><TH CLASS='downtime'>種類</TH><TH CLASS='downtime'>期間</TH><TH CLASS='downtime'>ダウンタイムID</TH><TH CLASS='downtime'>トリガーID</TH><TH CLASS='downtime'>アクション</TH></TR>\n");

	/* display all the service downtime */
	for(temp_downtime = scheduled_downtime_list, total_downtime = 0; temp_downtime != NULL; temp_downtime = temp_downtime->next) {

		if(temp_downtime->type != SERVICE_DOWNTIME)
			continue;

		temp_service = find_service(temp_downtime->host_name, temp_downtime->service_description);

		/* make sure the user has rights to view service information */
		if(is_authorized_for_service(temp_service, &current_authdata) == FALSE)
			continue;

		total_downtime++;

		if(odd) {
			odd = 0;
			bg_class = "downtimeOdd";
			}
		else {
			odd = 1;
			bg_class = "downtimeEven";
			}

		printf("<tr CLASS='%s'>", bg_class);
		printf("<td CLASS='%s'><A HREF='%s?type=%d&host=%s'>%s</A></td>", bg_class, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_downtime->host_name), temp_downtime->host_name);
		printf("<td CLASS='%s'><A HREF='%s?type=%d&host=%s", bg_class, EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_downtime->host_name));
		printf("&service=%s'>%s</A></td>", url_encode(temp_downtime->service_description), temp_downtime->service_description);
		get_time_string(&temp_downtime->entry_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<td CLASS='%s'>%s</td>", bg_class, date_time);
		printf("<td CLASS='%s'>%s</td>", bg_class, (temp_downtime->author == NULL) ? "N/A" : temp_downtime->author);
		printf("<td CLASS='%s'>%s</td>", bg_class, (temp_downtime->comment == NULL) ? "N/A" : temp_downtime->comment);
		get_time_string(&temp_downtime->start_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<td CLASS='%s'>%s</td>", bg_class, date_time);
		get_time_string(&temp_downtime->end_time, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
		printf("<td CLASS='%s'>%s</td>", bg_class, date_time);
		printf("<td CLASS='%s'>%s</td>", bg_class, (temp_downtime->fixed == TRUE) ? "固定" : "非固定");
		get_time_breakdown(temp_downtime->duration, &days, &hours, &minutes, &seconds);
		printf("<td CLASS='%s'>%d日間と %d時間 %d分 %d秒</td>", bg_class, days, hours, minutes, seconds);
		printf("<td CLASS='%s'>%lu</td>", bg_class, temp_downtime->downtime_id);
		printf("<td CLASS='%s'>", bg_class);
		if(temp_downtime->triggered_by == 0)
			printf("N/A");
		else
			printf("%lu", temp_downtime->triggered_by);
		printf("</td>\n");
		printf("<td><a href='%s?cmd_typ=%d&down_id=%lu'><img src='%s%s' border=0 ALT='このスケジュールエントリを削除もしくはキャンセル' TITLE='このスケジュールエントリを削除もしくはキャンセル'></td>", COMMAND_CGI, CMD_DEL_SVC_DOWNTIME, temp_downtime->downtime_id, url_images_path, DELETE_ICON);
		printf("</tr>\n");
		}

	if(total_downtime == 0)
		printf("<TR CLASS='downtimeOdd'><TD CLASS='downtimeOdd' COLSPAN=12>ダウンタイムがスケジュールされたサービスはありません</TD></TR>");

	printf("</TABLE>\n");
	printf("</DIV>\n");

	return;
	}



/* shows check scheduling queue */
void show_scheduling_queue(void) {
	sortdata *temp_sortdata;
	servicestatus *temp_svcstatus = NULL;
	hoststatus *temp_hststatus = NULL;
	char date_time[MAX_DATETIME_LENGTH];
	char temp_url[MAX_INPUT_BUFFER];
	int odd = 0;
	const char *bgclass = "";


	/* make sure the user has rights to view system information */
	if(is_authorized_for_system_information(&current_authdata) == FALSE) {

		printf("<P><DIV CLASS='errorMessage'>プロセス情報を閲覧する権限がありません。</DIV></P>\n");
		printf("<P><DIV CLASS='errorDescription'>このメッセージが何らかのエラーである場合はHTTPサーバのこのCGIに対するアクセス権限の設定かNagiosのCGI用設定ファイルの認証に関するオプションを調べてみてください</DIV></P>\n");

		return;
		}

	/* sort hosts and services */
	sort_data(sort_type, sort_option);

	printf("<DIV ALIGN=CENTER CLASS='statusSort'><b>");
	if(sort_option == SORT_HOSTNAME)
		printf("ホスト名");
	else if(sort_option == SORT_SERVICENAME)
		printf("サービス名");
	else if(sort_option == SORT_SERVICESTATUS)
		printf("サービスステータス");
	else if(sort_option == SORT_LASTCHECKTIME)
		printf("最終チェック時刻");
	else if(sort_option == SORT_NEXTCHECKTIME)
		printf("次回チェック時間");
	printf("</b> (%s) でエントリーを並び替える\n", (sort_type == SORT_ASCENDING) ? "昇順" : "降順");
	printf("</DIV>\n");

	printf("<P>\n");
	printf("<DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 CLASS='queue'>\n");
	printf("<TR CLASS='queue'>");

	snprintf(temp_url, sizeof(temp_url) - 1, "%s?type=%d", EXTINFO_CGI, DISPLAY_SCHEDULING_QUEUE);
	temp_url[sizeof(temp_url) - 1] = '\x0';

	printf("<TH CLASS='queue'>ホスト名&nbsp;<A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='ホスト名で並び替え(昇順)' TITLE='ホスト名で並び替え(昇順)'></A><A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='ホスト名で並び替え(降順)' TITLE='ホスト名で並び替え(降順)'></A></TH>", temp_url, SORT_ASCENDING, SORT_HOSTNAME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_HOSTNAME, url_images_path, DOWN_ARROW_ICON);

	printf("<TH CLASS='queue'>サービス名&nbsp;<A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='サービス名で並び替え(昇順)' TITLE='サービス名で並び替え(昇順)'></A><A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='サービス名で並び替え(降順)' TITLE='サービス名で並び替え(降順)'></A></TH>", temp_url, SORT_ASCENDING, SORT_SERVICENAME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_SERVICENAME, url_images_path, DOWN_ARROW_ICON);

	printf("<TH CLASS='queue'>最終チェック時刻&nbsp;<A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='最終チェック時刻で並び替え(昇順)' TITLE='最終チェック時刻で並び替え(昇順)'></A><A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='最終チェック時刻で並び替え(降順)' TITLE='最終チェック時刻で並び替え(降順)'></A></TH>", temp_url, SORT_ASCENDING, SORT_LASTCHECKTIME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_LASTCHECKTIME, url_images_path, DOWN_ARROW_ICON);

	printf("<TH CLASS='queue'>次回チェック時間&nbsp;<A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='次回チェック時間で並び替え(昇順)' TITLE='次回チェック時間で並び替え(昇順)'></A><A HREF='%s&sorttype=%d&sortoption=%d'><IMG SRC='%s%s' BORDER=0 ALT='次回チェック時間で並び替え(降順)' TITLE='次回チェック時間で並び替え(降順)'></A></TH>", temp_url, SORT_ASCENDING, SORT_NEXTCHECKTIME, url_images_path, UP_ARROW_ICON, temp_url, SORT_DESCENDING, SORT_NEXTCHECKTIME, url_images_path, DOWN_ARROW_ICON);


	printf("<TH CLASS='queue'>種類</TH><TH CLASS='queue'>動作チェック</TH><TH CLASS='queue'>アクション</TH></TR>\n");


	/* display all services and hosts */
	for(temp_sortdata = sortdata_list; temp_sortdata != NULL; temp_sortdata = temp_sortdata->next) {

		/* skip hosts and services that shouldn't be scheduled */
		if(temp_sortdata->is_service == TRUE) {
			temp_svcstatus = temp_sortdata->svcstatus;
			if(temp_svcstatus->should_be_scheduled == FALSE) {
				/* passive-only checks should appear if they're being forced */
				if(!(temp_svcstatus->checks_enabled == FALSE && temp_svcstatus->next_check != (time_t)0L && (temp_svcstatus->check_options & CHECK_OPTION_FORCE_EXECUTION)))
					continue;
				}
			}
		else {
			temp_hststatus = temp_sortdata->hststatus;
			if(temp_hststatus->should_be_scheduled == FALSE) {
				/* passive-only checks should appear if they're being forced */
				if(!(temp_hststatus->checks_enabled == FALSE && temp_hststatus->next_check != (time_t)0L && (temp_hststatus->check_options & CHECK_OPTION_FORCE_EXECUTION)))
					continue;
				}
			}

		if(odd) {
			odd = 0;
			bgclass = "Even";
			}
		else {
			odd = 1;
			bgclass = "Odd";
			}

		printf("<TR CLASS='queue%s'>", bgclass);

		/* get the service status */
		if(temp_sortdata->is_service == TRUE) {

			printf("<TD CLASS='queue%s'><A HREF='%s?type=%d&host=%s'>%s</A></TD>", bgclass, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_svcstatus->host_name), temp_svcstatus->host_name);

			printf("<TD CLASS='queue%s'><A HREF='%s?type=%d&host=%s", bgclass, EXTINFO_CGI, DISPLAY_SERVICE_INFO, url_encode(temp_svcstatus->host_name));
			printf("&service=%s'>%s</A></TD>", url_encode(temp_svcstatus->description), temp_svcstatus->description);

			get_time_string(&temp_svcstatus->last_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TD CLASS='queue%s'>%s</TD>", bgclass, (temp_svcstatus->last_check == (time_t)0) ? "N/A" : date_time);

			get_time_string(&temp_svcstatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TD CLASS='queue%s'>%s</TD>", bgclass, (temp_svcstatus->next_check == (time_t)0) ? "N/A" : date_time);

			printf("<TD CLASS='queue%s'>", bgclass);
			if(temp_svcstatus->check_options == CHECK_OPTION_NONE)
				printf("通常 ");
			else {
				if(temp_svcstatus->check_options & CHECK_OPTION_FORCE_EXECUTION)
					printf("強制 ");
				if(temp_svcstatus->check_options & CHECK_OPTION_FRESHNESS_CHECK)
					printf("Freshness ");
				if(temp_svcstatus->check_options & CHECK_OPTION_ORPHAN_CHECK)
					printf("Orphan ");
				}
			printf("</TD>");

			printf("<TD CLASS='queue%s'>%s</TD>", (temp_svcstatus->checks_enabled == TRUE) ? "ENABLED" : "DISABLED", (temp_svcstatus->checks_enabled == TRUE) ? "有効" : "無効");

			printf("<TD CLASS='queue%s'>", bgclass);
			if(temp_svcstatus->checks_enabled == TRUE) {
				printf("<a href='%s?cmd_typ=%d&host=%s", COMMAND_CGI, CMD_DISABLE_SVC_CHECK, url_encode(temp_svcstatus->host_name));
				printf("&service=%s'><img src='%s%s' border=0 ALT='このサービスの動作チェックを無効' TITLE='このサービスの動作チェックを無効'></a>\n", url_encode(temp_svcstatus->description), url_images_path, DISABLED_ICON);
				}
			else {
				printf("<a href='%s?cmd_typ=%d&host=%s", COMMAND_CGI, CMD_ENABLE_SVC_CHECK, url_encode(temp_svcstatus->host_name));
				printf("&service=%s'><img src='%s%s' border=0 ALT='このサービスの動作チェックを有効' TITLE='このサービスの動作チェックを有効'></a>\n", url_encode(temp_svcstatus->description), url_images_path, ENABLED_ICON);
				}
			printf("<a href='%s?cmd_typ=%d&host=%s", COMMAND_CGI, CMD_SCHEDULE_SVC_CHECK, url_encode(temp_svcstatus->host_name));
			printf("&service=%s%s'><img src='%s%s' border=0 ALT='このサービスのチェックを次回スケジュールに追加する' TITLE='このサービスのチェックを次回スケジュールに追加する'></a>\n", url_encode(temp_svcstatus->description), (temp_svcstatus->checks_enabled == TRUE) ? "&force_check" : "", url_images_path, DELAY_ICON);
			printf("</TD>\n");
			}

		/* get the host status */
		else {

			printf("<TD CLASS='queue%s'><A HREF='%s?type=%d&host=%s'>%s</A></TD>", bgclass, EXTINFO_CGI, DISPLAY_HOST_INFO, url_encode(temp_hststatus->host_name), temp_hststatus->host_name);

			printf("<TD CLASS='queue%s'>&nbsp;</TD>", bgclass);

			get_time_string(&temp_hststatus->last_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TD CLASS='queue%s'>%s</TD>", bgclass, (temp_hststatus->last_check == (time_t)0) ? "N/A" : date_time);

			get_time_string(&temp_hststatus->next_check, date_time, (int)sizeof(date_time), SHORT_DATE_TIME);
			printf("<TD CLASS='queue%s'>%s</TD>", bgclass, (temp_hststatus->next_check == (time_t)0) ? "N/A" : date_time);

			printf("<TD CLASS='queue%s'>", bgclass);
			if(temp_hststatus->check_options == CHECK_OPTION_NONE)
				printf("通常 ");
			else {
				if(temp_hststatus->check_options & CHECK_OPTION_FORCE_EXECUTION)
					printf("強制 ");
				if(temp_hststatus->check_options & CHECK_OPTION_FRESHNESS_CHECK)
					printf("Freshness ");
				if(temp_hststatus->check_options & CHECK_OPTION_ORPHAN_CHECK)
					printf("Orphan ");
				}
			printf("</TD>");

			printf("<TD CLASS='queue%s'>%s</TD>", (temp_hststatus->checks_enabled == TRUE) ? "ENABLED" : "DISABLED", (temp_hststatus->checks_enabled == TRUE) ? "有効" : "無効");

			printf("<TD CLASS='queue%s'>", bgclass);
			if(temp_hststatus->checks_enabled == TRUE) {
				printf("<a href='%s?cmd_typ=%d&host=%s", COMMAND_CGI, CMD_DISABLE_HOST_CHECK, url_encode(temp_hststatus->host_name));
				printf("'><img src='%s%s' border=0 ALT='このホストの動作チェックを無効' TITLE='このホストの動作チェックを無効'></a>\n", url_images_path, DISABLED_ICON);
				}
			else {
				printf("<a href='%s?cmd_typ=%d&host=%s", COMMAND_CGI, CMD_ENABLE_HOST_CHECK, url_encode(temp_hststatus->host_name));
				printf("'><img src='%s%s' border=0 ALT='このホストの動作チェックを有効' TITLE='このホストの動作チェックを有効'></a>\n", url_images_path, ENABLED_ICON);
				}
			printf("<a href='%s?cmd_typ=%d&host=%s%s", COMMAND_CGI, CMD_SCHEDULE_HOST_CHECK, url_encode(temp_hststatus->host_name), (temp_hststatus->checks_enabled == TRUE) ? "&force_check" : "");
			printf("'><img src='%s%s' border=0 ALT='このホストのチェックを次回スケジュールに追加する' TITLE='このホストのチェックを次回スケジュールに追加する'></a>\n", url_images_path, DELAY_ICON);
			printf("</TD>\n");
			}

		printf("</TR>\n");

		}

	printf("</TABLE>\n");
	printf("</DIV>\n");
	printf("</P>\n");


	/* free memory allocated to sorted data list */
	free_sortdata_list();

	return;
	}



/* sorts host and service data */
int sort_data(int s_type, int s_option) {
	sortdata *new_sortdata;
	sortdata *last_sortdata;
	sortdata *temp_sortdata;
	servicestatus *temp_svcstatus;
	hoststatus *temp_hststatus;

	if(s_type == SORT_NONE)
		return ERROR;

	/* sort all service status entries */
	for(temp_svcstatus = servicestatus_list; temp_svcstatus != NULL; temp_svcstatus = temp_svcstatus->next) {

		/* allocate memory for a new sort structure */
		new_sortdata = (sortdata *)malloc(sizeof(sortdata));
		if(new_sortdata == NULL)
			return ERROR;

		new_sortdata->is_service = TRUE;
		new_sortdata->svcstatus = temp_svcstatus;
		new_sortdata->hststatus = NULL;

		last_sortdata = sortdata_list;
		for(temp_sortdata = sortdata_list; temp_sortdata != NULL; temp_sortdata = temp_sortdata->next) {

			if(compare_sortdata_entries(s_type, s_option, new_sortdata, temp_sortdata) == TRUE) {
				new_sortdata->next = temp_sortdata;
				if(temp_sortdata == sortdata_list)
					sortdata_list = new_sortdata;
				else
					last_sortdata->next = new_sortdata;
				break;
				}
			else
				last_sortdata = temp_sortdata;
			}

		if(sortdata_list == NULL) {
			new_sortdata->next = NULL;
			sortdata_list = new_sortdata;
			}
		else if(temp_sortdata == NULL) {
			new_sortdata->next = NULL;
			last_sortdata->next = new_sortdata;
			}
		}

	/* sort all host status entries */
	for(temp_hststatus = hoststatus_list; temp_hststatus != NULL; temp_hststatus = temp_hststatus->next) {

		/* allocate memory for a new sort structure */
		new_sortdata = (sortdata *)malloc(sizeof(sortdata));
		if(new_sortdata == NULL)
			return ERROR;

		new_sortdata->is_service = FALSE;
		new_sortdata->svcstatus = NULL;
		new_sortdata->hststatus = temp_hststatus;

		last_sortdata = sortdata_list;
		for(temp_sortdata = sortdata_list; temp_sortdata != NULL; temp_sortdata = temp_sortdata->next) {

			if(compare_sortdata_entries(s_type, s_option, new_sortdata, temp_sortdata) == TRUE) {
				new_sortdata->next = temp_sortdata;
				if(temp_sortdata == sortdata_list)
					sortdata_list = new_sortdata;
				else
					last_sortdata->next = new_sortdata;
				break;
				}
			else
				last_sortdata = temp_sortdata;
			}

		if(sortdata_list == NULL) {
			new_sortdata->next = NULL;
			sortdata_list = new_sortdata;
			}
		else if(temp_sortdata == NULL) {
			new_sortdata->next = NULL;
			last_sortdata->next = new_sortdata;
			}
		}

	return OK;
	}


int compare_sortdata_entries(int s_type, int s_option, sortdata *new_sortdata, sortdata *temp_sortdata) {
	hoststatus *temp_hststatus = NULL;
	servicestatus *temp_svcstatus = NULL;
	time_t last_check[2];
	time_t next_check[2];
	int current_attempt[2];
	int status[2];
	char *hname[2];
	char *service_description[2];

	if(new_sortdata->is_service == TRUE) {
		temp_svcstatus = new_sortdata->svcstatus;
		last_check[0] = temp_svcstatus->last_check;
		next_check[0] = temp_svcstatus->next_check;
		status[0] = temp_svcstatus->status;
		hname[0] = temp_svcstatus->host_name;
		service_description[0] = temp_svcstatus->description;
		current_attempt[0] = temp_svcstatus->current_attempt;
		}
	else {
		temp_hststatus = new_sortdata->hststatus;
		last_check[0] = temp_hststatus->last_check;
		next_check[0] = temp_hststatus->next_check;
		status[0] = temp_hststatus->status;
		hname[0] = temp_hststatus->host_name;
		service_description[0] = "";
		current_attempt[0] = temp_hststatus->current_attempt;
		}
	if(temp_sortdata->is_service == TRUE) {
		temp_svcstatus = temp_sortdata->svcstatus;
		last_check[1] = temp_svcstatus->last_check;
		next_check[1] = temp_svcstatus->next_check;
		status[1] = temp_svcstatus->status;
		hname[1] = temp_svcstatus->host_name;
		service_description[1] = temp_svcstatus->description;
		current_attempt[1] = temp_svcstatus->current_attempt;
		}
	else {
		temp_hststatus = temp_sortdata->hststatus;
		last_check[1] = temp_hststatus->last_check;
		next_check[1] = temp_hststatus->next_check;
		status[1] = temp_hststatus->status;
		hname[1] = temp_hststatus->host_name;
		service_description[1] = "";
		current_attempt[1] = temp_hststatus->current_attempt;
		}

	if(s_type == SORT_ASCENDING) {

		if(s_option == SORT_LASTCHECKTIME) {
			if(last_check[0] <= last_check[1])
				return TRUE;
			else
				return FALSE;
			}
		if(s_option == SORT_NEXTCHECKTIME) {
			if(next_check[0] <= next_check[1])
				return TRUE;
			else
				return FALSE;
			}
		else if(s_option == SORT_CURRENTATTEMPT) {
			if(current_attempt[0] <= current_attempt[1])
				return TRUE;
			else
				return FALSE;
			}
		else if(s_option == SORT_SERVICESTATUS) {
			if(status[0] <= status[1])
				return TRUE;
			else
				return FALSE;
			}
		else if(s_option == SORT_HOSTNAME) {
			if(strcasecmp(hname[0], hname[1]) < 0)
				return TRUE;
			else
				return FALSE;
			}
		else if(s_option == SORT_SERVICENAME) {
			if(strcasecmp(service_description[0], service_description[1]) < 0)
				return TRUE;
			else
				return FALSE;
			}
		}
	else {
		if(s_option == SORT_LASTCHECKTIME) {
			if(last_check[0] > last_check[1])
				return TRUE;
			else
				return FALSE;
			}
		if(s_option == SORT_NEXTCHECKTIME) {
			if(next_check[0] > next_check[1])
				return TRUE;
			else
				return FALSE;
			}
		else if(s_option == SORT_CURRENTATTEMPT) {
			if(current_attempt[0] > current_attempt[1])
				return TRUE;
			else
				return FALSE;
			}
		else if(s_option == SORT_SERVICESTATUS) {
			if(status[0] > status[1])
				return TRUE;
			else
				return FALSE;
			}
		else if(s_option == SORT_HOSTNAME) {
			if(strcasecmp(hname[0], hname[1]) > 0)
				return TRUE;
			else
				return FALSE;
			}
		else if(s_option == SORT_SERVICENAME) {
			if(strcasecmp(service_description[0], service_description[1]) > 0)
				return TRUE;
			else
				return FALSE;
			}
		}

	return TRUE;
	}



/* free all memory allocated to the sortdata structures */
void free_sortdata_list(void) {
	sortdata *this_sortdata;
	sortdata *next_sortdata;

	/* free memory for the sortdata list */
	for(this_sortdata = sortdata_list; this_sortdata != NULL; this_sortdata = next_sortdata) {
		next_sortdata = this_sortdata->next;
		free(this_sortdata);
		}

	return;
	}
