# This Python file uses the following encoding: utf-8
############################################################################
# Copyright (c) 2025 Li Auto Inc. and its affiliates
# Licensed under the Apache License, Version 2.0(the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
############################################################################
from func_module.validation.common.basic_rule import BasicRule, timing_decorator
from durable.lang import *
from durable.engine import *

class Os014001Rule(BasicRule):
    """当OsAlarmAutostartType为RELATIVE,OsAlarmAlarmTime必须大于0 """
    ruleset_name = "Os014001"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "OsAlarmAutostartType=RELATIVE,OsAlarmAlarmTime>0"
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):

                @when_all(m.OsAlarmAutostartType != 'RELATIVE')
                def output1(c):
                    self.__class__.validation_result = True

                @when_all(
                  (m.OsAlarmAutostartType == 'RELATIVE') & (m.OsAlarmAlarmTimeValid == True))
                def output2(c):
                    self.__class__.validation_result = True

    @timing_decorator
    def validation_run(self):
        """014表示:附加约束,001表示:os模块子校验码(001~999)"""
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历OsAlarm获取数据
            if "/Os/OsAlarm" in os_instance.get("DefinitionRef", ""):
                OsAlarmAutostartType = "ABSOLUTE"
                OsAlarmAlarmTime = 0
                OsAlarmAutostart_valid = True
                for alarm_name, alarm_item in os_instance.items():
                    if not isinstance(alarm_item, dict):
                        continue
                    if alarm_item.get("DefinitionRef", "").endswith("/Os/OsAlarm/OsAlarmAutostart"):
                        OsAlarmAutostartType = alarm_item.get("OsAlarmAutostartType", {}).get("Value")
                        OsAlarmAlarmTime = alarm_item.get("OsAlarmAlarmTime", {}).get("Value")
                        if OsAlarmAutostartType not in ("ABSOLUTE", "RELATIVE"):
                            OsAlarmAutostart_valid = False
                        if OsAlarmAutostartType == "RELATIVE" and not (
                            type(OsAlarmAlarmTime) is int and 0 < OsAlarmAlarmTime < 4294967296):
                            OsAlarmAutostart_valid = False
                validation_data = {}
                param_path = "/ActiveEcuC/Os/" + os_key
                validation_data.update({"OsAlarmAutostartType": OsAlarmAutostartType})
                validation_data.update({"OsAlarmAlarmTime": OsAlarmAlarmTime})
                # 时间范围在 Python 中判断，避免规则引擎对 uint32 上界的整数转换。
                validation_data.update({"OsAlarmAlarmTimeValid": OsAlarmAutostart_valid})
                try:
                    self.__class__.validation_result = False
                    if OsAlarmAutostart_valid:
                        post(self.__class__.ruleset_name, validation_data)
                except Exception as e:
                    pass
                # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
                if self.__class__.validation_result:
                    self.vlog(self.validation_code, self.validation_summary, f"validation_data:{validation_data}",
                              param_path, "pass")
                else:
                    self.vlog(self.validation_code, self.validation_summary, f"validation_data:{validation_data}",
                              param_path, "fail")
                # retract_facts(self.__class__.ruleset_name, validation_data)
