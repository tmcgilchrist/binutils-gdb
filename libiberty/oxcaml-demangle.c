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

/* This file implements demangling for OxCaml's structured mangling scheme:
   "_Caml<path>", where <path> is a sequence of tagged, length-prefixed
   identifiers, one per lexical scope.

   The dispatch entry point below is libiberty-licensed glue.  The structured
   demangling core lives in the companion file demangle-ocaml-structured.c. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <demangle.h>
#include "libiberty.h"

#include "demangle-ocaml-structured.h"

/* Demangle an OxCaml symbol for libiberty's cplus_demangle dispatch.

   Returns a newly allocated string on success, or NULL if the input does not
   look like a mangled OxCaml symbol.  OPTIONS is accepted for consistency with
   the other libiberty demanglers but is unused.  */

char *
oxcaml_demangle (const char *mangled, int options ATTRIBUTE_UNUSED)
{
  return ocaml_demangle_structured_sym (mangled);
}
