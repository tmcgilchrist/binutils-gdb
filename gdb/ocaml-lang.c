/* OCaml language support routines for GDB, the GNU debugger.

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

#include "symtab.h"
#include "language.h"
#include "varobj.h"
#include "ocaml-lang.h"
#include "c-lang.h"
#include "demangle.h"
#include "cp-support.h"
#include "gdbarch.h"
#include "parser-defs.h"

/* The name of the symbol to use to get the name of the main subprogram.  */
static const char OCAML_MAIN[] = "camlDune__exe__Main";

/* Function returning the special symbol name used by OCaml for the main
   procedure in the main program if it is found in minimal symbol list.
   This function tries to find minimal symbols so that it finds them even
   if the program was compiled without debugging information.  */

const char *
ocaml_main_name (void)
{
  bound_minimal_symbol msym
    = lookup_minimal_symbol (current_program_space, OCAML_MAIN);
  if (msym.minsym != NULL)
    return OCAML_MAIN;

  /* No known entry procedure found, the main program is probably not OCaml.  */
  return NULL;
}

/* Implements the la_demangle language_defn routine for language OCaml.  */

gdb::unique_xmalloc_ptr<char>
ocaml_demangle (const char *symbol, int options)
{
  /* OCaml symbols typically use the pattern Module__function.
     For now, we don't perform demangling and return the symbol as-is.
     This will be implemented in Stage 3.  */
  return gdb_demangle (symbol, options);
}

/* Class representing the OCaml language.  */

class ocaml_language : public language_defn
{
public:
  ocaml_language ()
    : language_defn (language_ocaml)
  { /* Nothing.  */ }

  /* See language.h.  */

  const char *name () const override
  { return "ocaml"; }

  /* See language.h.  */

  const char *natural_name () const override
  { return "OCaml"; }

  /* See language.h.  */

  const std::vector<const char *> &filename_extensions () const override
  {
    static const std::vector<const char *> extensions = { ".ml", ".mli" };
    return extensions;
  }

  /* See language.h.  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override
  {
    const struct builtin_ocaml_type *builtin = builtin_ocaml_type (gdbarch);

    /* Helper function to allow shorter lines below.  */
    auto add  = [&] (struct type * t)
    {
      lai->add_primitive_type (t);
    };

    add (builtin->builtin_void);
    add (builtin->builtin_unit);
    add (builtin->builtin_bool);
    add (builtin->builtin_int);
    add (builtin->builtin_char);
    add (builtin->builtin_float);
    add (builtin->builtin_string);
    add (builtin->builtin_int32);
    add (builtin->builtin_int64);
    add (builtin->builtin_nativeint);

    lai->set_string_char_type (builtin->builtin_char);
    lai->set_bool_type (builtin->builtin_bool, "bool");
  }

  /* See language.h.  */
  bool sniff_from_mangled_name
       (const char *mangled,
	gdb::unique_xmalloc_ptr<char> *demangled) const override
  {
    *demangled = ocaml_demangle (mangled, 0);
    return *demangled != NULL;
  }

  /* See language.h.  */

  gdb::unique_xmalloc_ptr<char> demangle_symbol (const char *mangled,
						 int options) const override
  {
    return ocaml_demangle (mangled, options);
  }

  /* See language.h.  */

  bool can_print_type_offsets () const override
  {
    return true;
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override
  {
    c_print_type (type, varstring, stream, show, level, la_language, flags);
  }

  /* See language.h.  */

  void value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const override
  {
    return ocaml_value_print_inner (val, stream, recurse, options);
  }

  /* See language.h.  */

  int parser (struct parser_state *ps) const override
  {
    /* No parser yet - will be implemented in Stage 6.
       Return 0 to indicate parsing not supported.  */
    return 0;
  }

  /* See language.h.  */

  const char *name_of_this () const override
  { return "self"; }
};

/* Single instance of the OCaml language class.  */

static ocaml_language ocaml_language_defn;

/* Build all OCaml language types for the specified architecture.  */

static struct builtin_ocaml_type *
build_ocaml_types (struct gdbarch *gdbarch)
{
  struct builtin_ocaml_type *builtin_ocaml_type = new struct builtin_ocaml_type;

  /* Basic types.  */
  type_allocator alloc (gdbarch);
  builtin_ocaml_type->builtin_void = builtin_type (gdbarch)->builtin_void;

  /* Unit type - OCaml's equivalent of void for functions with no return value.
     For now, we use the same as void.  */
  builtin_ocaml_type->builtin_unit = builtin_type (gdbarch)->builtin_void;

  /* Boolean type - represented as integer in OCaml runtime.  */
  builtin_ocaml_type->builtin_bool
    = init_boolean_type (alloc, 8, 1, "bool");

  /* Integer type - native int (31-bit on 32-bit, 63-bit on 64-bit systems
     due to tag bit).  */
  builtin_ocaml_type->builtin_int
    = init_integer_type (alloc, gdbarch_ptr_bit (gdbarch), 0, "int");

  /* Character type - 8-bit unsigned.  */
  builtin_ocaml_type->builtin_char
    = init_character_type (alloc, 8, 1, "char");

  /* Float type - always 64-bit in OCaml.  */
  builtin_ocaml_type->builtin_float
    = init_float_type (alloc, 64, "float", floatformats_ieee_double);

  /* String type - for now, represented as pointer to char.
     Will be refined in later stages to handle OCaml's actual string representation.  */
  builtin_ocaml_type->builtin_string
    = init_pointer_type (alloc, gdbarch_ptr_bit (gdbarch), "string",
			 builtin_ocaml_type->builtin_char);

  /* Fixed-size integer types.  */
  builtin_ocaml_type->builtin_int32
    = init_integer_type (alloc, 32, 0, "int32");

  builtin_ocaml_type->builtin_int64
    = init_integer_type (alloc, 64, 0, "int64");

  /* Native integer type - same as int but for C interop.  */
  builtin_ocaml_type->builtin_nativeint
    = init_integer_type (alloc, gdbarch_ptr_bit (gdbarch), 0, "nativeint");

  return builtin_ocaml_type;
}

static const registry<gdbarch>::key<struct builtin_ocaml_type> ocaml_type_data;

/* Return the OCaml type table for the specified architecture.  */

const struct builtin_ocaml_type *
builtin_ocaml_type (struct gdbarch *gdbarch)
{
  struct builtin_ocaml_type *result = ocaml_type_data.get (gdbarch);
  if (result == nullptr)
    {
      result = build_ocaml_types (gdbarch);
      ocaml_type_data.set (gdbarch, result);
    }

  return result;
}

/* Implement la_value_print_inner for OCaml.
   For Stage 1, we delegate to C-style printing.
   This will be enhanced in Stage 4 with OCaml-specific value representation.  */

void
ocaml_value_print_inner (struct value *val, struct ui_file *stream, int recurse,
			 const struct value_print_options *options)
{
  /* Delegate to C value printing for now.  */
  c_value_print_inner (val, stream, recurse, options);
}
