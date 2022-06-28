// SPDX-License-Identifier: GPL-2.0
/*
 * Based On dragonheart kernel thanks the sources
 * Copyright (C) 2021 Dakkshesh <dakkshesh5@gmail.com>.
 */

#ifndef _VPROFILES_H_
#define _VPROFILES_H_

#include <linux/types.h>

#ifdef CONFIG_VPROFILES
void vprofiles_set_mode_rollback(unsigned int level, unsigned int duration_ms);
void vprofiles_set_mode(unsigned int level);
unsigned int active_mode(void);
#else
static inline void vprofiles_set_mode_rollback(unsigned int level, unsigned int duration_ms)
{
}
static inline void vprofiles_set_mode(unsigned int level)
{
}
static inline unsigned int active_mode(void)
{
	return 0;
}
#endif

#endif /* _VPROFILES_H_ */ 
