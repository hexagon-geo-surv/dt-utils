/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright 2013-2023 The DT-Utils Authors <oss-tools@pengutronix.de> */

#ifndef __BAREBOX_STATE__
#define __BAREBOX_STATE__

struct state *state_get(const char *name, const char *file, bool readonly, bool auth);
char *state_get_var(struct state *state, const char *var);

#endif /* __BAREBOX_STATE__ */
