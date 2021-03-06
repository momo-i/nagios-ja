angular.module("nagiosTime", [])

	.filter("duration", function () {
        return function (duration) {
			var d, h, m, s;
			var timeLeft = duration;

			d = Math.floor(timeLeft / 86400000);
			timeLeft -= d * 86400000;
			h = Math.floor(timeLeft / 3600000);
			timeLeft -= h * 3600000;
			m = Math.floor(timeLeft / 60000);
			timeLeft -= m * 60000;
			s = Math.floor(timeLeft / 1000);

			return d + "日 " + h + "時間 " + m + "分 " + s + "秒";
		}
	})

	.service("nagiosTimeService", function() {
		return {
			cannedTimeperiods: [
				{ "value": "today",			"label": "今日" },
				{ "value": "last24hours",	"label": "過去24時間" },
				{ "value": "yesterday",		"label": "昨日" },
				{ "value": "thisweek",		"label": "今週" },
				{ "value": "last7days",		"label": "過去7日間" },
				{ "value": "lastweek",		"label": "先週" },
				{ "value": "thismonth",		"label": "今月" },
				{ "value": "last31days",	"label": "過去31日間" },
				{ "value": "lastmonth",		"label": "先月" },
				{ "value": "thisyear",		"label": "今年" },
				{ "value": "lastyear",		"label": "去年" }
			],
			timeperiodlist: function() {
				// TODO: Only define this once
				var list = this.cannedTimeperiods.concat([]);
				list.push({
					value: "custom",
					label: "* カスタム期間設定 *"
				});
				return list;
			},
			isCannedTimeperiod: function(timeperiod) {
				for (var i = 0; i < this.cannedTimeperiods.length; i++) {
					if (timeperiod == this.cannedTimeperiods[i].value) {
						return true;
					}
				}
				return false;
			},
			calculateReportTimes: function(now, period) {
				// Calculate report start and end times based on current
				// time and the pre-defined period selected

				// Initialize the start and end times to something
				// reasonable
				var starttime = new Date(now.getFullYear(), now.getMonth(), 
								now.getDate(), 0, 0, 0);
				var endtime = new Date(now);

				// For canned report periods, calculate the start and
				// end times
				switch(period) {
				case "today":
					starttime = new Date(now.getFullYear(), now.getMonth(), 
							now.getDate(), 0, 0, 0);
					endtime = new Date(now);
					break;
				case "last24hours":
					starttime = new Date(now.getTime() -
							(24 * 60 * 60 * 1000));
					endtime = new Date(now);
					break;
				case "yesterday":
					// This is a change from the legacy trends in that
					// it accounts for DST changes. The legacy trends
					// used the 24 hour period ending at midnight today.
					var oneDayAgo = new Date(now.getTime() -
							(24 * 60 * 60 * 1000));
					starttime = new Date(oneDayAgo.getFullYear(),
							oneDayAgo.getMonth(), oneDayAgo.getDate(),
							0, 0, 0);
					endtime = new Date(now.getFullYear(), now.getMonth(),
							now.getDate(), 0, 0, 0);
					break;
				case "thisweek":
					var thisTimeSunday = new Date(now.getTime() -
							(24 * 60 * 60 * 1000 * now.getDay()));
					starttime = new Date(thisTimeSunday.getFullYear(),
							thisTimeSunday.getMonth(),
							thisTimeSunday.getDate(), 0, 0, 0);
					endtime = new Date(now);
					break;
				case "last7days":
					// This is a change from the legacy trends in that
					// it accounts for DST changes. The legacy trends
					// set the start time to be a week's worth of
					// seconds (7*24*60*60) before now.
					var thisTimeLastWeek = new Date(now.getTime() -
							(7 * 24 * 60 * 60 * 1000));
					starttime = new Date(thisTimeLastWeek.getFullYear(),
							thisTimeLastWeek.getMonth(),
							thisTimeLastWeek.getDate(),
							now.getHours(), now.getMinutes(),
							now.getSeconds());
					endtime = new Date(now);
					break;
				case "lastweek":
					// This is a change from the legacy trends in that
					// it accounts for DST changes. The legacy trends
					// set the start time to be a week's worth of seconds
					// (7*24*60*60) before last Sunday at midnight.
					var thisTimeLastSunday = new Date(now.getTime() -
							(24 * 60 * 60 * 1000 * (now.getDay() + 7)));
					var thisTimeSunday = new Date(now.getTime() -
							(24 * 60 * 60 * 1000 * now.getDay()));
					starttime = new Date(thisTimeLastSunday.getFullYear(),
							thisTimeLastSunday.getMonth(),
							thisTimeLastSunday.getDate(), 0, 0, 0);
					endtime = new Date(thisTimeSunday.getFullYear(),
							thisTimeSunday.getMonth(),
							thisTimeSunday.getDate(), 0, 0, 0);
					break;
				case "thismonth":
					starttime = new Date(now.getFullYear(), now.getMonth(), 
							1, 0, 0, 0);
					endtime = new Date(now);
					break;
				case "last31days":
					// This is a change from the legacy trends in that
					// it accounts for DST changes. The legacy trends
					// set the start time to be a months's worth of
					// seconds (31*24*60*60) before now.
					var daysAgo31 = new Date(now.getTime() -
							(31 * 24 * 60 * 60 * 1000));
					starttime = new Date(daysAgo31.getFullYear(),
							daysAgo31.getMonth(), daysAgo31.getDate(),
							now.getHours(), now.getMinutes(),
							now.getSeconds());
					endtime = new Date(now);
					break;
				case "lastmonth":
					endtime = new Date(now.getFullYear(), now.getMonth(), 
							1, 0, 0, 0);
					var lastMonthEnd = new Date(endtime.getTime() - 1);
					starttime = new Date(lastMonthEnd.getFullYear(),
							lastMonthEnd.getMonth(), 1, 0, 0, 0);
					break;
				case "thisyear":
					starttime = new Date(now.getFullYear(), 0, 1, 0, 0, 0);
					endtime = new Date(now);
					break;
				case "lastyear":
					starttime = new Date(now.getFullYear() - 1,
							0, 1, 0, 0, 0);
					endtime = new Date(now.getFullYear(), 0, 1, 0, 0, 0);
					break;
				}

				return {start: starttime, end: endtime};
			}
		};
	});
