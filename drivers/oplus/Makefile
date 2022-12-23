# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2018-2020 Oplus. All rights reserved.
obj-$(CONFIG_OPLUS_FEATURE_DUMP_DEVICE_INFO)	+=  dump_device_info.o
obj-$(CONFIG_OPLUS_FEATURE_OPROJECT) 	+= oplus_project.o
obj-y     += system/cmdline_parser/
obj-y     += system/project_info/
obj-$(CONFIG_OPLUS_FEATURE_FDLEAK_CHECK) += oplus_fdleak_check.o
obj-$(CONFIG_OPLUS_ION_BOOSTPOOL) += oplus_performance/ion_boost_pool/oplus_ion_boost_pool.o
obj-y += wakelock_profiler/
obj-$(CONFIG_OPLUS_FEATURE_DUMP_DEVICE_INFO) += system/dump_device_info

#Add for NandSwap
obj-$(CONFIG_NANDSWAP)		+= oplus_performance/oplus_nandswap/
