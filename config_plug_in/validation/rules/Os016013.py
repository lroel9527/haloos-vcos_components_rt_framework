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

class Os016013Rule(BasicRule):
    """Alarm Callbacks 只能在ScalabilityClass1时使用"""
    ruleset_name = "Os016013"
    validation_result = False

    def __init__(self, logs, validation_data) -> None:
        super().__init__()
        self.logs = logs
        self.ecuc_os_dict = validation_data.bsw_ecuc_dict['Os']['ArPackages']['ActiveEcuC']['Os']
        self.validation_code = self.__class__.ruleset_name
        self.validation_summary = "Alarm Callbacks shall only be allowed in Scalability Class 1"
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
                @when_all((m.AlarmActionType == "ALARM_ACTION_ALARMCALLBACK") & (m.ScalabilityClass == "SC1"))
                def action_pass(c):
                    self.__class__.validation_result = True
                @when_all((m.AlarmActionType == "ALARM_ACTION_ALARMCALLBACK") & (m.ScalabilityClass != "SC1"))
                def action_fail(c):
                    self.__class__.validation_result = False
            pass

    @timing_decorator
    def validation_run(self):
        scalability_class = ""

        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue
            if "/Os/OsOS" in os_instance["DefinitionRef"] and "OsScalabilityClass" in os_instance:
                scalability_class = os_instance["OsScalabilityClass"].get("Value", "")
                break

        # Check each alarm for callback usage
        for os_key, os_instance in self.ecuc_os_dict.items():
            if not isinstance(os_instance, dict):
                continue

            if "/Os/OsAlarm" in os_instance["DefinitionRef"]:
                param_path = "/ActiveEcuC/Os/" + os_key

                # Check alarm actions for callback
                has_callback = False
                callback_name = ""

                for subkey, subvalue in os_instance.items():
                    if isinstance(subvalue, dict) and "DefinitionRef" in subvalue:
                        if "/Os/OsAlarm/OsAlarmAction" in subvalue["DefinitionRef"]:
                            for action_key, action_value in subvalue.items():
                                if isinstance(action_value, dict) and "DefinitionRef" in action_value:
                                    if "/Os/OsAlarm/OsAlarmAction/OsAlarmCallback" in action_value["DefinitionRef"]:
                                        has_callback = True
                                        if "OsAlarmCallbackName" in action_value and action_value["OsAlarmCallbackName"]["Value"]:
                                            callback_name = action_value["OsAlarmCallbackName"]["Value"]
                                        else:
                                            callback_name = "<not specified>"
                                        break
                            if has_callback:
                                break

                # Validate if alarm has callback configured
                if has_callback:
                    validation_data = {
                        "AlarmName": os_key,
                        "AlarmActionType": "ALARM_ACTION_ALARMCALLBACK",
                        "ScalabilityClass": scalability_class
                    }

                    try:
                        self.__class__.validation_result = False
                        post(self.__class__.ruleset_name, validation_data)
                    except Exception as e:
                        pass

                    # Log validation result
                    if self.__class__.validation_result:
                        self.vlog(self.validation_code, self.validation_summary,
                                f"Alarm '{os_key}' with callback '{callback_name}' is correctly configured in {scalability_class}",
                                param_path, "pass")
                    else:
                        if scalability_class:
                            error_msg = f"Alarm '{os_key}' with callback '{callback_name}' is configured in {scalability_class}, but Alarm Callbacks shall only be used in SC1"
                        else:
                            error_msg = f"Alarm '{os_key}' with callback '{callback_name}' is configured, but ScalabilityClass is not specified. Alarm Callbacks shall only be used in SC1"
                        self.vlog(self.validation_code, self.validation_summary, error_msg, param_path, "fail")