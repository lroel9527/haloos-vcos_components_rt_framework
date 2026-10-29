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

class Os002001Rule(BasicRule):
    """OsAlarmAppModeRef在每个alarm中均需配置且值非空"""
    ruleset_name = "Os002001"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "OsAlarmAppModeRef is not configured, should be check"
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):

                @when_all(+m.OsAlarm)
                def action(c):
                    self.__class__.validation_result = True
            pass

    @timing_decorator
    def validation_run(self):
        ecuc_os_dict = self.ecuc_os_dict
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsAlarm" in os_instance.get("DefinitionRef", ""):
                OsAlarmAppModeRef_list = []
                OsAlarmAppModeRef_valid = True
                for alarm_name, alarm_item in os_instance.items():
                    if not isinstance(alarm_item, dict):
                        continue
                    if alarm_item.get("DefinitionRef", "").endswith("/Os/OsAlarm/OsAlarmAutostart"):
                        OsAlarmAppModeRef = alarm_item.get("OsAlarmAppModeRef", {}).get("ValueRef", "")
                        if not isinstance(OsAlarmAppModeRef, list):
                            OsAlarmAppModeRef = [OsAlarmAppModeRef]
                        OsAlarmAppModeRef_list.extend(OsAlarmAppModeRef)
                        if not OsAlarmAppModeRef:
                            OsAlarmAppModeRef_valid = False
                        for value in OsAlarmAppModeRef:
                            if not isinstance(value, str) or value.strip() in ("", "<empty>", "null"):
                                OsAlarmAppModeRef_valid = False
                validation_data = {}
                param_path = "/ActiveEcuC/Os/"+os_key
                validation_data.update({"OsAlarm": os_key})
                validation_data.update({"OsAlarmAppModeRef_list": OsAlarmAppModeRef_list})
                try:
                    self.__class__.validation_result = False
                    if OsAlarmAppModeRef_valid:
                        post(self.__class__.ruleset_name, validation_data)
                except Exception as e:
                    pass
                # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
                if self.__class__.validation_result:
                    self.vlog(self.validation_code, self.validation_summary,
                                f"validation_data:{validation_data}", param_path, "pass")
                else:
                    self.vlog(self.validation_code, self.validation_summary,
                                f"validation_data:{validation_data}", param_path, "fail")