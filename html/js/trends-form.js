angular.module("trendsApp")
	.controller("trendsFormCtrl", function($scope, $modalInstance, $http,
			nagiosTimeService, params) {

		$scope.params = params;
		$scope.hostlist = [];
		$scope.servicelist = [];
		$scope.timeperiodlist = nagiosTimeService.timeperiodlist();

		$scope.initialAssumedStates = {
			"": [],
			"hosts": [
				{ value: "", label: "考慮しない" },
				{ value: "current", label: "現在の状態" },
				{ value: "up", label: "ホスト稼働(UP)" },
				{ value: "down", label: "ホスト停止(DOWN)" },
				{ value: "unreachable", label: "ホスト未到達(UNREACHABLE)" }
			],
			"services": [
				{ value: "", label: "考慮しない" },
				{ value: "current", label: "現在の状態" },
				{ value: "ok", label: "サービス正常(OK)" },
				{ value: "warning", label: "サービス警告(WARNING)" },
				{ value: "unknown", label: "サービス不明(UNKNOWN)" },
				{ value: "critical", label: "サービス異常(CRITICAL)" },
			]
		};

		if (!$scope.params.hasOwnProperty("t1")) {
			$scope.params.t1 = 0;
		}

		if (!$scope.params.hasOwnProperty("t2")) {
			$scope.params.t2 = 0;
		}

		$scope.apply = function () {
			if ($scope.params.timeperiod != "custom") {
				var times = nagiosTimeService.calculateReportTimes(new Date(),
						$scope.params.timeperiod);
				$scope.params.t1 = times.start.getTime() / 1000;
				$scope.params.t2 = times.end.getTime() / 1000;
			}
			$modalInstance.close($scope.params);
		};

		$scope.cancel = function () {
			$modalInstance.dismiss('cancel');
		};

		$scope.showHost = function() {
			return $scope.params.reporttype != "";
		};

		$scope.showService = function() {
			return $scope.params.reporttype == "services";
		};

		$scope.showTimeperiod = function() {
			switch ($scope.params.reporttype) {
			case "":
				return false;
				break;
			case "hosts":
				return $scope.params.host != "";
				break;
			case "services":
				return $scope.params.host != "" &&
						$scope.params.service != "";
				break;
			}
		};

		$scope.showDates = function() {
			return $scope.params.timeperiod == "custom";
		};

		var isTimeperiodValid = function() {
			switch ($scope.params.timeperiod) {
			case "":
				return false;
				break;
			case "custom":
				if ($scope.params.t1 == 0 ||
						$scope.params.t1 == "不正な日付" ||
						$scope.params.t2 == 0 ||
						$scope.params.t2 == "不正な日付") {
					return false;
				}
				else {
					return true;
				}
				break;
			default:
				return true;
				break;
			}
		};

		$scope.disableApply = function() {
			switch ($scope.params.reporttype) {
			case "":
				return true;
				break;
			case "hosts":
				if ($scope.params.host == "") {
					return true;
				}
				return !isTimeperiodValid();
				break;
			case "services":
				if ($scope.params.host == "" ||
						$scope.params.service == "") {
					return true;
				}
				return !isTimeperiodValid();
				break;
			}
		}

		var getHostList = function() {
			var url = $scope.params.cgiurl +
					"/objectjson.cgi?query=hostlist";
			$http.get(url, { withCredentials: true})
				.success(function(results) {
					$scope.hostlist = results.data.hostlist;
				})
				.error(function(err) {
					console.log(err);
				});
		};

		var getServiceList = function(hostname) {
			if (hostname != "") {
				var url = $scope.params.cgiurl +
						"/objectjson.cgi?query=servicelist&hostname=" +
						hostname;
				$http.get(url, { withCredentials: true})
					.success(function(results) {
						$scope.servicelist =
								results.data.servicelist[hostname];
					})
					.error(function(err) {
						console.log(err);
					});
			}
		};

		$scope.onBlurCgiurl = function(evt) {
			getHostList();
		};

		$scope.$watch('params.reporttype', function(newValue) {
			if (newValue != "") {
				getHostList();
			}
		});

		$scope.$watch('params.host', function(newValue) {
			if (newValue != "") {
				getServiceList($scope.params.host);
			}
		});

		$scope.$watch("params.timeperiod", function(newValue) {
			if (newValue != null || newValue != "custom") {
				var times = nagiosTimeService.calculateReportTimes(new Date, newValue);
				$scope.params.t1 = times.start / 1000;
				$scope.params.t2 = times.start / 1000;
			}
		});

		$scope.$watch('params.startDate', function(newValue) {
			if (newValue != undefined) {
				time = new Date(newValue);
				if (time != "不正な日付") {
					$scope.params.t1 = time.getTime() / 1000;
				}
			}
		});

		$scope.$watch('params.endDate', function(newValue) {
			if (newValue != undefined) {
				time = new Date(newValue);
				if (time != "不正な日付") {
					$scope.params.t2 = time.getTime() / 1000;
				}
			}
		});

	});

