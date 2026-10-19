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
from .Os_ruleutils import OsRuleutils

class Os016006Rule(BasicRule):
    """Alarm只能关联本核的counter"""
    ruleset_name = "Os016006"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "Alarm can only associate Counter with the same Core and OsAlarmCounterRef must be config"
        self.durable_init()

    def check_validation_run(self,bsw_ecuc_cfg_class_dict):
        if not self.modules_are_exist(["Os"],bsw_ecuc_cfg_class_dict):
            self.vlog(self.validation_code, self.validation_summary, "Data required for validation is missing",
                self.validation_summary, "fail")
            return False
        return True

    def durable_init(self):
        try:
            get_host().get_ruleset(self.__class__.ruleset_name)
        except Exception as e:
            with ruleset(self.__class__.ruleset_name):
                @when_all((m.OsAlarmCounterRefCore == m.OsAlarmCore) & (+m.OsAlarmCounter))
                def action(c):
                    self.__class__.validation_result = True
            pass

    @timing_decorator
    def validation_run(self):
        ecuc_os_dict = self.ecuc_os_dict
        for os_key, os_instance in ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            # 遍历OsAlarm获取数据
            if "/Os/OsAlarm" in os_instance.get("DefinitionRef", ""):
                OsAlarmCounter = ""
                OsAlarmCore,OsAlarmCounterRefCore = "",""
                counter_refs = OsRuleutils().get_ref_list(os_instance.get("OsAlarmCounterRef", {}))
                if len(counter_refs) == 1 and isinstance(counter_refs[0], str):
                    OsAlarmCounter = counter_refs[0]
                app_name_alarm,app_name_counter = [],[]
                for app_name, app_cfg in ecuc_os_dict.items():
                    if not isinstance(app_cfg, dict) or "/Os/OsApplication" not in app_cfg.get("DefinitionRef", ""):
                        continue
                    alarm_refs = OsRuleutils().get_ref_list(app_cfg.get("OsAppAlarmRef", {}))
                    counter_refs = OsRuleutils().get_ref_list(app_cfg.get("OsAppCounterRef", {}))
                    app_name_alarm.extend([app_name] * alarm_refs.count(os_key))
                    app_name_counter.extend([app_name] * counter_refs.count(OsAlarmCounter))
                if len(app_name_counter) == 1:
                    OsAlarmCounterRefCore = ecuc_os_dict[app_name_counter[0]].get("OsApplicationCoreRef", {}).get("ValueRef", "")
                if len(app_name_alarm) == 1:
                    OsAlarmCore = ecuc_os_dict[app_name_alarm[0]].get("OsApplicationCoreRef", {}).get("ValueRef", "")
                counter_cfg = ecuc_os_dict.get(OsAlarmCounter, {})
                OsAlarmCounter_valid = isinstance(counter_cfg, dict) and counter_cfg.get("DefinitionRef", "").endswith("/Os/OsCounter")
                validation_data = {}
                param_path = "/ActiveEcuC/Os/" + os_key
                validation_data.update({"OsAlarm": os_key})
                validation_data.update({"OsAlarmCounter": OsAlarmCounter})
                validation_data.update({"OsAlarmCounterRefCore": OsAlarmCounterRefCore})
                validation_data.update({"OsAlarmCore": OsAlarmCore})
                try:
                    self.__class__.validation_result = False
                    if OsAlarmCounter_valid and isinstance(OsAlarmCore, str) and \
                       isinstance(OsAlarmCounterRefCore, str) and \
                       OsAlarmCore.strip() not in ("", "<empty>", "null") and \
                       OsAlarmCounterRefCore.strip() not in ("", "<empty>", "null"):
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