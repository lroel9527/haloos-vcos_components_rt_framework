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

class Os013014Rule(BasicRule):
    """两项校验：OsAlarm的OsAlarmActivateTask,OsAlarmCallback,OsAlarmSetEvent互斥,OsAlarmAction只能三选一配置；
    OsAlarmActivateTask、OsAlarmSetEvent、OsAlarmCallback 这三项的shortname不允许修改 """
    ruleset_name = "Os013014"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "OsAlarmAction only must choose one in three"

    @timing_decorator
    def validation_run(self):
        """013表示:互斥配置校验,001表示:os模块子校验码(001~999)"""
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历OsAlarm获取数据
            if "/Os/OsAlarm" in os_instance.get("DefinitionRef", ""):
                OsAlarmAction_found = False
                for alarmcontent_k, alarmcontent_v in os_instance.items():
                    if not isinstance(alarmcontent_v, dict):
                        continue
                    if alarmcontent_v.get("DefinitionRef", "").endswith("/Os/OsAlarm/OsAlarmAction"):
                        OsAlarmAction_found = True
                        OsAlarmAction_list = [key for key, value in alarmcontent_v.items()
                                              if isinstance(value, dict) and "DefinitionRef" in value]
                        validation_data = {}
                        param_path = "/ActiveEcuC/Os/" + os_key
                        validation_data.update({"OsAlarmActionlist": OsAlarmAction_list})
                        valid_items = {'OsAlarmActivateTask', 'OsAlarmCallback', 'OsAlarmSetEvent'}
                        try:
                            self.__class__.validation_result = False
                            if len(validation_data['OsAlarmActionlist']) == 1:
                                action_name = validation_data['OsAlarmActionlist'][0]
                                action_definition = alarmcontent_v[action_name].get("DefinitionRef", "")
                                if action_name in valid_items and isinstance(action_definition, str) and \
                                   action_definition.endswith("/OsAlarmAction/" + action_name):
                                    self.__class__.validation_result = True
                                else:
                                    self.__class__.validation_result = False
                            elif len(validation_data['OsAlarmActionlist']) == 0:
                                self.__class__.validation_result = False
                            else:
                                self.__class__.validation_result = False
                        except Exception as e:
                            pass
                        # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
                        if self.__class__.validation_result:
                            self.vlog(self.validation_code, self.validation_summary,
                                      f"validation_data:{validation_data}", param_path, "pass")
                        else:
                            self.vlog(self.validation_code, self.validation_summary,
                                      f"validation_data:{validation_data}", param_path, "fail")
                if not OsAlarmAction_found:
                    validation_data = {"OsAlarmActionlist": []}
                    param_path = "/ActiveEcuC/Os/" + os_key
                    # 格式："错误码", "摘要", "错误信息", "错误索引路径" "结果"
                    self.vlog(self.validation_code, self.validation_summary,
                              f"validation_data:{validation_data}", param_path, "fail")
