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

class Os001004Rule(BasicRule):
    """子容器OsAlarmAction须存在且只能存在一个,无法添加两个OsAlarmAction子容器。"""
    ruleset_name = "Os001004"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "The child container OsAlarmAction must exist and only one can exist."
        self.durable_init()

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):

                @when_all(+m.OsAlarm)
                def output(c):
                    self.__class__.validation_result = True

    @timing_decorator
    def validation_run(self):
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历OsAlarm获取数据
            if "/Os/OsAlarm" in os_instance.get("DefinitionRef", ""):
                OsAlarmAction_number = 0
                OsAlarmAction_att = False
                for alarm_k,alarm_v in os_instance.items():
                    if not isinstance(alarm_v,dict):
                        continue
                    if alarm_v.get("DefinitionRef", "").endswith("/Os/OsAlarm/OsAlarmAction"):
                        OsAlarmAction_number = OsAlarmAction_number + 1
                        OsAlarmAction_att = True
                validation_data = {}
                param_path = "/ActiveEcuC/Os/" + os_key
                validation_data.update({"OsAlarm": os_key})
                try:
                    self.__class__.validation_result = False
                    if OsAlarmAction_number == 1 and OsAlarmAction_att:
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
