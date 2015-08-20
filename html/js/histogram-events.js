angular.module("histogramEvents", [])

	.service("histogramEventsService", function() {
		// Raw events values
		this.hostUp = 1;
		this.hostDown = 2;
		this.hostUnreachable = 4;
		this.serviceOk = 8;
		this.serviceWarning = 16;
		this.serviceUnknown = 32;
		this.serviceCritical = 64;

		// Calculated events values
		this.hostProblems = this.hostDown + this.hostUnreachable;
		this.hostAll = this.hostUp + this.hostProblems;
		this.serviceProblems = this.serviceWarning +
				this.serviceUnknown + this.serviceCritical;
		this.serviceAll = this.serviceOK + this.serviceProblems;

		return {
			// Host events list
			hostEvents: [
				{
					value: this.hostAll,
					label: "全ホストイベント",
					states: "up down unreachable"
				},
				{
					value: this.hostProblems,
					label: "ホスト障害イベント",
					states: "down unreachable"
				},
				{
					value: this.hostUp,
					label: "ホスト稼働(UP)イベント",
					states: "up"
				},
				{
					value: this.hostDown,
					label: "ホスト停止(DOWN)イベント",
					states: "down"
				},
				{
					value: this.hostUnreachable,
					label: "ホスト未到達(UNREACHABLE)イベント",
					states: "unreachable"
				}
			],

			// Service events list
			serviceEvents: [
				{
					value: this.serviceAll,
					label: "全サービスイベント",
					states: "ok warning unknown critical"
				},
				{
					value: this.serviceProblems,
					label: "サービス障害イベント",
					states: "warning unknown critical"
				},
				{
					value: this.serviceOk,
					label: "サービス正常(OK)イベント",
					states: "ok"
				},
				{
					value: this.serviceWarning,
					label: "サービス警告(WARNING)イベント",
					states: "warning"
				},
				{
					value: this.serviceUnknown,
					label: "サービス不明(UNKNOWN)イベント",
					states: "unknown"
				},
				{
					value: this.serviceCritical,
					label: "サービス障害(CRITICAL)イベント",
					states: "critical"
				},
			]
		};
	});

