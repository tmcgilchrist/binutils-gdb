/* Demangler for the OxCaml compiler
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

This file is part of the libiberty library.
Libiberty is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

In addition to the permissions in the GNU Library General Public
License, the Free Software Foundation gives you unlimited permission
to link the compiled version of this file into combinations with other
programs, and to distribute those combinations without any restriction
coming from the use of this file.  (The Library Public License
restrictions do apply in other respects; for example, they cover
modification of the file, and distribution when not linked into a
combined executable.)

Libiberty is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with libiberty; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
Boston, MA 02110-1301, USA.  */

/* This file implements demangling for OCaml symbols, supporting both:
   - Flat mangling:       "caml<Name>..." (legacy OCaml compiler)
   - Structured mangling: "_Caml<path>"   (OxCaml compiler)

   The flat demangler and the dispatch entry point below are libiberty-licensed
   glue.  The structured demangling core lives in the companion file
   demangle-ocaml-structured.c. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "safe-ctype.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <demangle.h>
#include "libiberty.h"

#include "demangle-ocaml-structured.h"

/* safe-ctype.h poisons the standard isupper/isxdigit so that locale-dependent
   uses are caught at compile time.  The flat body uses them on plain ASCII, so
   redirect them — and the flat "$xx" decoder's hex() — to the safe,
   locale-independent libiberty equivalents.  */
#undef isupper
#undef isxdigit
#define isupper(c)  ISUPPER (c)
#define isxdigit(c) ISXDIGIT (c)
#define hex(c)      hex_value (c)

/*
 * Flat OCaml demangling
 *
 * Mangled symbols start with "caml" followed by an uppercase letter.
 * "__" encodes "." and "$xx" encodes character with hex value xx.
 */

static const char *caml_prefix = "caml";
static const size_t caml_prefix_len = 4;

/* mangled flat OCaml symbols start with "caml" followed by an upper-case letter */
static bool
is_flat(const char *sym)
{
	return 0 == strncmp(sym, caml_prefix, caml_prefix_len)
		&& isupper(sym[caml_prefix_len]);
}

static char *
ocaml_demangle_flat(const char *sym)
{
	char *result;
	int j = 0;
	int i;
	int len;

	len = strlen(sym);

	/* the demangled symbol is always smaller than the mangled symbol */
	result = malloc(len + 1);
	if (!result)
		return NULL;

	/* skip "caml" prefix */
	i = caml_prefix_len;

	while (i < len) {
		if (sym[i] == '_' && sym[i + 1] == '_') {
			/* "__" -> "." */
			result[j++] = '.';
			i += 2;
		} else if (sym[i] == '$' && isxdigit(sym[i + 1])
			   && isxdigit(sym[i + 2])) {
			/* "$xx" is a hex-encoded character */
			result[j++] = (hex(sym[i + 1]) << 4) | hex(sym[i + 2]);
			i += 3;
		} else {
			result[j++] = sym[i++];
		}
	}
	result[j] = '\0';

	return result;
}

/*
 * input:
 *     sym: a symbol which may have been mangled by the OCaml compiler
 * return:
 *     if the input doesn't look like a mangled OCaml symbol, NULL is returned
 *     otherwise, a newly allocated string containing the demangled symbol is returned
 *
 * Supports both:
 *   - Flat mangling:       "caml<Name>..." (OCaml compiler 4.* through to 5.*)
 *   - Structured mangling: "_Caml<path>"   (OxCaml compiler)
 */
static char *
ocaml_demangle_sym(const char *sym)
{
	char *result = ocaml_demangle_structured_sym(sym);

	if (result)
		return result;

	if (is_flat(sym))
		return ocaml_demangle_flat(sym);

	return NULL;
}

/* Demangle an OCaml symbol for libiberty's cplus_demangle dispatch.

   Returns a newly allocated string on success, or NULL if the input does not
   look like a mangled OCaml symbol.  OPTIONS is accepted for consistency with
   the other libiberty demanglers but is unused.  */

char *
oxcaml_demangle (const char *mangled, int options ATTRIBUTE_UNUSED)
{
  return ocaml_demangle_sym (mangled);
}
