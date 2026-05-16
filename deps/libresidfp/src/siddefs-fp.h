/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2026 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 1999 Dag Lem <resid@nimrod.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SIDDEFS_FP_H
#define SIDDEFS_FP_H

#define HAVE_CXX17
#define MAYBE_UNUSED [[ maybe_unused ]]

#ifdef _MSC_VER
#define _unreachable    __assume(false);
#define likely(x)       (x)
#define unlikely(x)     (x)
#else
#define _unreachable    __builtin_unreachable();
#define likely(x)	    __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#endif



// Inlining on/off.
#define RESIDFP_INLINING 1
#define RESIDFP_INLINE inline

#endif // SIDDEFS_FP_H
