/* OCaml language support definitions for GDB, the GNU debugger.

   Copyright (C) 2025 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef GDB_OCAML_LANG_H
#define GDB_OCAML_LANG_H

#include "symtab.h"

/* Language specific builtin types for OCaml.  Any additional types added
   should be kept in sync with enum ocaml_primitive_types, where these
   types are documented.  */

struct builtin_ocaml_type
{
  struct type *builtin_void = nullptr;
  struct type *builtin_unit = nullptr;
  struct type *builtin_bool = nullptr;
  struct type *builtin_int = nullptr;
  struct type *builtin_char = nullptr;
  struct type *builtin_float = nullptr;
  struct type *builtin_string = nullptr;
  struct type *builtin_int32 = nullptr;
  struct type *builtin_int64 = nullptr;
  struct type *builtin_nativeint = nullptr;
};

/* Defined in ocaml-exp.y (when parser is implemented).  */

extern int ocaml_parse (struct parser_state *);

/* Defined in ocaml-lang.c  */

extern const char *ocaml_main_name (void);

extern gdb::unique_xmalloc_ptr<char> ocaml_demangle (const char *mangled,
						     int options);

extern const struct builtin_ocaml_type *builtin_ocaml_type (struct gdbarch *);

/* Implement la_value_print_inner for OCaml.  */

extern void ocaml_value_print_inner (struct value *val,
				     struct ui_file *stream, int recurse,
				     const struct value_print_options *options);

#endif /* GDB_OCAML_LANG_H */
