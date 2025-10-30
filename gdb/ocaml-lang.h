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
  /* Core types */
  struct type *builtin_void = nullptr;
  struct type *builtin_unit = nullptr;
  struct type *builtin_bool = nullptr;
  struct type *builtin_int = nullptr;
  struct type *builtin_char = nullptr;
  struct type *builtin_float = nullptr;
  struct type *builtin_string = nullptr;

  /* Fixed-size integer types */
  struct type *builtin_int32 = nullptr;
  struct type *builtin_int64 = nullptr;
  struct type *builtin_nativeint = nullptr;

  /* Additional numeric types */
  struct type *builtin_uint8 = nullptr;   /* For bytes */
  struct type *builtin_uint16 = nullptr;

  /* OCaml runtime representation types */
  struct type *builtin_value = nullptr;   /* Generic OCaml value (tagged word) */
  struct type *builtin_block = nullptr;   /* Pointer to heap block */
};

/* Defined in ocaml-exp.y (when parser is implemented).  */

extern int ocaml_parse (struct parser_state *);

/* Defined in ocaml-lang.c  */

extern const char *ocaml_main_name (void);

extern gdb::unique_xmalloc_ptr<char> ocaml_demangle (const char *mangled,
						     int options);

extern const struct builtin_ocaml_type *builtin_ocaml_type (struct gdbarch *);

/* OCaml value representation helpers.  */

/* Check if a value is an immediate integer (LSB = 1).  */
extern bool ocaml_is_immediate_int (LONGEST val);

/* Check if a value is a pointer to a heap block (LSB = 0).  */
extern bool ocaml_is_block (LONGEST val);

/* Extract the integer value from an immediate int (shift right by 1).  */
extern LONGEST ocaml_immediate_int_val (LONGEST val);

/* Read the header of an OCaml block given its address.  Returns true on
   success, false on memory read error.  */
extern bool ocaml_read_block_header (struct gdbarch *gdbarch, CORE_ADDR addr,
				     ULONGEST *header);

/* Extract the tag from an OCaml block header.  */
extern int ocaml_header_tag (ULONGEST header);

/* Extract the size (in words) from an OCaml block header.  */
extern ULONGEST ocaml_header_size (ULONGEST header);

/* Read a field from an OCaml block.  Fields are stored sequentially after
   the block pointer.  Returns true on success.  */
extern bool ocaml_read_block_field (struct gdbarch *gdbarch, CORE_ADDR block_addr,
				    int field_index, LONGEST *value);

/* Print an OCaml string block.  */
extern void ocaml_print_string (struct gdbarch *gdbarch, CORE_ADDR addr,
				ULONGEST size, struct ui_file *stream);

/* OCaml block tags (used in heap blocks).

   TODO: Special Tag Handling
   - Tags 246-255 have special meanings in the OCaml runtime
   - Some tags need special handling that is not yet implemented:
     * OCAML_TAG_LAZY (246): Lazy value - could force evaluation or show as <lazy>
     * OCAML_TAG_INFIX (249): Infix pointer inside a closure - should follow to real closure
     * OCAML_TAG_FORWARD (250): Forwarding pointer after GC - should follow to new location
     * OCAML_TAG_NO_SCAN (251): Block containing no pointers - could show raw bytes
   - Currently these are shown as <special block tag=N>
   - Future work: Implement appropriate handling for each special tag */
#define OCAML_TAG_LAZY           246
#define OCAML_TAG_CLOSURE        247
#define OCAML_TAG_OBJECT         248
#define OCAML_TAG_INFIX          249
#define OCAML_TAG_FORWARD        250
#define OCAML_TAG_NO_SCAN        251
#define OCAML_TAG_ABSTRACT       251
#define OCAML_TAG_STRING         252
#define OCAML_TAG_DOUBLE         253
#define OCAML_TAG_DOUBLE_ARRAY   254
#define OCAML_TAG_CUSTOM         255

/* Special immediate values.

   TODO: Multiple OCaml values share the same runtime representation:
   - unit (), false, [], and None all have the value 1
   - Without DWARF type information, we cannot distinguish between them
   - Future work: Use DWARF DIE types to determine the actual type
   - This affects printing in ocaml_print_value() where we show these as "0" */
#define OCAML_VAL_UNIT           1      /* () */
#define OCAML_VAL_FALSE          1      /* false */
#define OCAML_VAL_TRUE           3      /* true */
#define OCAML_VAL_EMPTY_LIST     1      /* [] */
#define OCAML_VAL_NONE           1      /* None */

/* Implement la_value_print_inner for OCaml.  */

extern void ocaml_value_print_inner (struct value *val,
				     struct ui_file *stream, int recurse,
				     const struct value_print_options *options,
				     struct type *dwarf_type = nullptr);

#endif /* GDB_OCAML_LANG_H */
