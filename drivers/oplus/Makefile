# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2018-2020 Oplus. All rights reserved.
obj-$(CONFIG_OPLUS_FEATURE_DUMP_DEVICE_INFO)	+=  dump_device_info.o
obj-$(CONFIG_OPLUS_FEATURE_OPROJECT) 	+= oplus_project.o
obj-$(CONFIG_OPLUS_FEATURE_SELINUX_CONTROL_LOG) += proc.o
#obj-y     += charger/
obj-y     += system/cmdline_parser/
obj-$(CONFIG_OPLUS_FEATURE_PMIC_MONITOR)     += system/oplus_pmic_monitor/
obj-$(CONFIG_OPLUS_FEATURE_CPU_JANKINFO)     += oplus_performance/cpu_jankinfo/
obj-$(CONFIG_OPLUS_HEALTHINFO)     += oplus_performance/healthinfo/main/
obj-$(CONFIG_PROCESS_RECLAIM_ENHANCE)     += oplus_performance/oplus_mm/process_reclaim/
obj-$(CONFIG_OPLUS_FEATURE_ZRAM_OPT)     += oplus_performance/oplus_mm/zram_opt/
obj-$(CONFIG_OPLUS_FEATURE_FDLEAK_CHECK) += oplus_fdleak_check.o
obj-$(CONFIG_COLOR_CTRL_V2) += colorctrl.o
obj-$(CONFIG_OPLUS_ION_BOOSTPOOL) += oplus_performance/ion_boost_pool/oplus_ion_boost_pool.o

obj-y	+= charger/oplus_chg_symbol.o
obj-y	+= oplus_performance/healthinfo/fs/cpuload.o oplus_performance/healthinfo/fs/fg_uid/fg_uid.o
