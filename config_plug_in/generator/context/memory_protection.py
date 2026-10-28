############################################################################
#  Copyright (c) 2025 Li Auto Inc. and its affiliates
#  Licensed under the Apache License, Version 2.0(the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
############################################################################

from jinja2 import pass_context
from components.rt_framework.config_plug_in.generator import utils
# Compatibility with the original monolithic generator context.  Imported
# lazily by Function.get_mpu_cfg after Function has been defined.
from .functions import Function as RtfwBasics
from .functions import Function as RtfwCore
from .functions import Function as RtfwApplication


class RtfwMpu:
    # Region count and index keys
    _DATA_REGION_KEYS = ("data_region_cnt", "data_region_start", "data_region_end")
    _CODE_REGION_KEYS = ("code_region_cnt", "code_region_start", "code_region_end")

    @staticmethod
    def _is_data_region(cpu_core, access_rights):
        """Check if region is a data region based on CPU core and access rights"""
        if cpu_core != "TriCore":
            return True
        return '_R' in access_rights

    @staticmethod
    def _is_code_region(cpu_core, access_rights):
        """Check if region is a code region (TriCore only)"""
        if cpu_core != "TriCore":
            return False
        return '_X' in access_rights or '_RX' in access_rights

    @staticmethod
    def _init_region_dict():
        """Initialize a region dictionary with data and code regions"""
        return {
            "data_region": {},
            "code_region": {}
        }

    @staticmethod
    def _count_actual_regions(region_dict, region_type):
        """Count actual regions excluding metadata keys"""
        keys_to_exclude = RtfwMpu._DATA_REGION_KEYS if region_type == "data_region" else RtfwMpu._CODE_REGION_KEYS
        return len([k for k in region_dict[region_type].keys() if k not in keys_to_exclude])

    @staticmethod
    def _add_region_counts(region_dict, base_cfg):
        """Add region counts and start/end indices to region dictionary based on base configuration"""
        data_start = base_cfg["data_region"]["data_region_end"]
        code_start = base_cfg["code_region"]["code_region_end"]

        data_cnt = RtfwMpu._count_actual_regions(region_dict, "data_region")
        code_cnt = RtfwMpu._count_actual_regions(region_dict, "code_region")

        region_dict["data_region"].update({
            "data_region_cnt": data_cnt,
            "data_region_start": data_start,
            "data_region_end": data_start + data_cnt
        })
        region_dict["code_region"].update({
            "code_region_cnt": code_cnt,
            "code_region_start": code_start,
            "code_region_end": code_start + code_cnt
        })

    @staticmethod
    def _filter_regions_by_owner_and_core(region_cfg, owner, coreid, cpu_core):
        """Filter regions by owner and core, categorize into data/code regions"""
        filtered = RtfwMpu._init_region_dict()
        core_match = "Core" + str(coreid)

        for region_name, region_value in region_cfg.items():
            if region_value["MemoryRegionOwner"] != owner:
                continue
            if core_match not in region_value["MemoryRegionProtectionUnit"]:
                continue

            access_rights = region_value["MemoryRegionAccessRights"]
            if RtfwMpu._is_data_region(cpu_core, access_rights):
                filtered["data_region"][region_name] = region_value
            if RtfwMpu._is_code_region(cpu_core, access_rights):
                filtered["code_region"][region_name] = region_value

        return filtered

    @staticmethod
    def _update_region_metadata(region_dict, region_type, count, start):
        """Update region metadata (count, start, end) for a given region type"""
        metadata_key_prefix = region_type
        region_dict[region_type].update({
            f"{metadata_key_prefix}_cnt": count,
            f"{metadata_key_prefix}_start": start,
            f"{metadata_key_prefix}_end": start + count
        })

    @staticmethod
    @pass_context
    def get_core_mpu_cfg(context, coreid, region_cfg):
        """Get MPU configuration for core-owned regions"""
        cpu_core = RtfwBasics.get_cpu_core(context)
        core_mpu_cfg = RtfwMpu._filter_regions_by_owner_and_core(region_cfg, '', coreid, cpu_core)

        # Core data regions include one extra region (index 0)
        data_cnt = len(core_mpu_cfg["data_region"]) + 1
        code_cnt = len(core_mpu_cfg["code_region"])

        RtfwMpu._update_region_metadata(core_mpu_cfg, "data_region", data_cnt, 0)
        RtfwMpu._update_region_metadata(core_mpu_cfg, "code_region", code_cnt, 0)

        return core_mpu_cfg

    @staticmethod
    @pass_context
    def get_app_mpu_cfg(context, coreid, region_cfg, core_mpu_cfg):
        """Get MPU configuration for application-owned regions"""
        app_cfg = RtfwApplication.get_app_cfg(context)
        cpu_core = RtfwBasics.get_cpu_core(context)

        set0_mpu_cfg = RtfwMpu._init_region_dict()
        app_mpu_cfg = {}

        # Collect regions for each application on this core
        for app_name, app_value in app_cfg.items():
            if str(coreid) != app_value["CoreNumber"]:
                continue

            # Always create an entry in app_mpu_cfg even for ID 0 apps
            app_regions = RtfwMpu._filter_regions_by_owner_and_core(region_cfg, app_name, coreid, cpu_core)

            # App ID 0 regions go to set0, but app entry is still created
            if app_value["AppMemoryProtectionIdentifier"] == 0:
                set0_mpu_cfg["data_region"].update(app_regions["data_region"])
                set0_mpu_cfg["code_region"].update(app_regions["code_region"])
                # Create empty app entry for ID 0 apps
                app_mpu_cfg[app_name] = RtfwMpu._init_region_dict()
            else:
                app_mpu_cfg[app_name] = app_regions

        # Calculate set0 region counts and indices
        RtfwMpu._add_region_counts(set0_mpu_cfg, core_mpu_cfg)

        # Calculate app region counts and indices for all apps on this core
        for app_name, app_value in app_cfg.items():
            if str(coreid) == app_value["CoreNumber"]:
                RtfwMpu._add_region_counts(app_mpu_cfg[app_name], set0_mpu_cfg)

        return set0_mpu_cfg, app_mpu_cfg

    @staticmethod
    def _process_entity_regions(entity_list, region_cfg, coreid, cpu_core, base_mpu_cfg):
        """Process regions for a list of entities (tasks or ISRs)"""
        entity_mpu_cfg = {}
        for entity_name in entity_list:
            entity_regions = RtfwMpu._filter_regions_by_owner_and_core(region_cfg, entity_name, coreid, cpu_core)
            entity_mpu_cfg[entity_name] = entity_regions
            RtfwMpu._add_region_counts(entity_mpu_cfg[entity_name], base_mpu_cfg)
        return entity_mpu_cfg

    @staticmethod
    @pass_context
    def get_task_mpu_cfg(context, coreid, region_cfg, set0_mpu_cfg, app_mpu_cfg):
        """Get MPU configuration for task-owned regions"""
        app_cfg = RtfwApplication.get_app_cfg(context)
        cpu_core = RtfwBasics.get_cpu_core(context)
        task_mpu_cfg = {}

        # Process task regions for each application
        for app_name, app_value in app_cfg.items():
            if str(coreid) != app_value["CoreNumber"]:
                continue

            app_tasks_cfg = RtfwMpu._process_entity_regions(
                app_value["AppTaskList"], region_cfg, coreid, cpu_core, app_mpu_cfg[app_name]
            )
            task_mpu_cfg.update(app_tasks_cfg)

        # Add special PreStart task for this core
        prestart_task_name = "Os_PreStart_Task_Core" + str(coreid)
        prestart_task_cfg = RtfwMpu._process_entity_regions(
            [prestart_task_name], region_cfg, coreid, cpu_core, set0_mpu_cfg
        )
        task_mpu_cfg.update(prestart_task_cfg)

        return task_mpu_cfg

    @staticmethod
    @pass_context
    def get_isr_mpu_cfg(context, coreid, region_cfg, app_mpu_cfg):
        """Get MPU configuration for ISR-owned regions"""
        app_cfg = RtfwApplication.get_app_cfg(context)
        cpu_core = RtfwBasics.get_cpu_core(context)
        isr_mpu_cfg = {}

        # Process ISR regions for each application
        for app_name, app_value in app_cfg.items():
            if str(coreid) != app_value["CoreNumber"]:
                continue

            app_isrs_cfg = RtfwMpu._process_entity_regions(
                app_value["AppIsrList"], region_cfg, coreid, cpu_core, app_mpu_cfg[app_name]
            )
            isr_mpu_cfg.update(app_isrs_cfg)

        return isr_mpu_cfg

    @staticmethod
    def _parse_memory_address(addr_str):
        """Parse memory address string and return formatted address and extern declaration if needed"""
        if addr_str.startswith("0x") or addr_str.startswith("0X"):
            return "(uint32)" + addr_str + "uL", None
        else:
            return "(uint32)&" + addr_str, "extern uint8 " + addr_str + "[]"

    @staticmethod
    def _parse_region_config(region_list):
        """Parse raw region list into structured region configuration"""
        region_cfg = {}
        extern_addr_list = []

        for region_name, region_value in region_list.items():
            if region_name == "DefinitionRef":
                continue

            # Parse start address
            start_addr_str = region_value.get("OsMemoryRegionStartAddress", {}).get("Value", "")
            start_addr, start_extern = RtfwMpu._parse_memory_address(start_addr_str)
            if start_extern:
                extern_addr_list.append(start_extern)

            # Parse end address
            end_addr_str = region_value.get("OsMemoryRegionEndAddress", {}).get("Value", "")
            end_addr, end_extern = RtfwMpu._parse_memory_address(end_addr_str)
            if end_extern:
                extern_addr_list.append(end_extern)

            region_cfg[region_name] = {
                "MemoryRegionStartAddress": start_addr,
                "MemoryRegionEndAddress": end_addr,
                "MemoryRegionIndex": region_value.get("OsMemoryRegionIndex", {}).get("Value", ""),
                "MemoryRegionOwner": utils.get_last_item(region_value.get("OsMemoryRegionOwner", {}).get("ValueRef", "")),
                "MemoryRegionProtectionUnit": utils.get_last_item(region_value.get("OsMemoryRegionProtectionUnit", {}).get("ValueRef", "")),
                "MemoryRegionAccessRights": utils.get_last_item(region_value.get("OsMemoryRegionAccessRights", {}).get("ValueRef", "")),
                "MemoryRegionFlags": utils.get_last_item(region_value.get("OsMemoryRegionFlags", {}).get("ValueRef", ""))
            }

        return region_cfg, extern_addr_list

    @staticmethod
    @pass_context
    def _generate_core_mpu_config(context, coreid, region_cfg):
        """Generate complete MPU configuration for a single core"""
        core_mpu_cfg = RtfwMpu.get_core_mpu_cfg(context, coreid, region_cfg)
        set0_mpu_cfg, app_mpu_cfg = RtfwMpu.get_app_mpu_cfg(context, coreid, region_cfg, core_mpu_cfg)
        task_mpu_cfg = RtfwMpu.get_task_mpu_cfg(context, coreid, region_cfg, set0_mpu_cfg, app_mpu_cfg)
        isr_mpu_cfg = RtfwMpu.get_isr_mpu_cfg(context, coreid, region_cfg, app_mpu_cfg)

        return {
            "core_mpu_cfg": core_mpu_cfg,
            "set0_mpu_cfg": set0_mpu_cfg,
            "app_mpu_cfg": app_mpu_cfg,
            "task_mpu_cfg": task_mpu_cfg,
            "isr_mpu_cfg": isr_mpu_cfg
        }

    @staticmethod
    @pass_context
    @utils.cache_once(RtfwBasics._cache)
    def get_mpu_cfg(context):
        """Get complete MPU configuration for all cores"""
        region_list = RtfwBasics.get_os_module_list(context, "/Os/OsMemoryProtection")
        if region_list:
            region_list = region_list["OsMemoryProtection"]

        osos_cfg = RtfwBasics.get_osos_cfg(context)

        # Only generate MPU config for SC3/SC4 scalability classes
        if osos_cfg.get("ScalabilityClass", "") not in ("SC3", "SC4"):
            return {}

        # Parse memory region configuration
        region_cfg, extern_addr_list = RtfwMpu._parse_region_config(region_list)

        # Generate MPU config for each core
        mpu_cfg = {}
        core_cnt = RtfwCore.get_core_cnt(context)

        for coreid in range(core_cnt):
            mpu_cfg[coreid] = RtfwMpu._generate_core_mpu_config(context, coreid, region_cfg)

        mpu_cfg["extern_addr_list"] = sorted(list(set(extern_addr_list)))

        return mpu_cfg
