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
#include "valprint.h"
#include "value.h"
#include "extract-store-integer.h"

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

/* Implements the la_demangle language_defn routine for language OCaml.

   OCaml uses a specific mangling scheme where symbols are prefixed with _O
   followed by encoded module paths and identifiers. The oxcaml_demangle
   function in libiberty handles the decoding.

   Examples:
   - _O4List3map -> List.map
   - _OModuleA__ModuleB__function -> ModuleA.ModuleB.function  */

gdb::unique_xmalloc_ptr<char>
ocaml_demangle (const char *symbol, int options)
{
  /* Try OCaml-specific demangling using libiberty's oxcaml_demangle.  */
  return gdb_demangle (symbol, options | DMGL_OXCAML);
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
    add (builtin->builtin_uint8);
    add (builtin->builtin_uint16);
    add (builtin->builtin_value);
    add (builtin->builtin_block);

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

  /* Additional numeric types for bytes and other uses.  */
  builtin_ocaml_type->builtin_uint8
    = init_integer_type (alloc, 8, 1, "uint8");

  builtin_ocaml_type->builtin_uint16
    = init_integer_type (alloc, 16, 1, "uint16");

  /* OCaml runtime value representation types.
     value: A tagged word - either an immediate int (LSB=1) or pointer (LSB=0).
     block: A pointer to a heap-allocated block with header.  */
  builtin_ocaml_type->builtin_value
    = init_integer_type (alloc, gdbarch_ptr_bit (gdbarch), 1, "value");

  builtin_ocaml_type->builtin_block
    = init_pointer_type (alloc, gdbarch_ptr_bit (gdbarch), "block",
			 builtin_ocaml_type->builtin_value);

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

/* OCaml value representation helper functions.

   OCaml uses a tagged representation for values:
   - Immediate integers have LSB = 1, actual value is shifted right by 1
   - Pointers to heap blocks have LSB = 0
   - Heap blocks have a header word containing size, color, and tag info

   This allows OCaml's garbage collector to distinguish pointers from ints
   without requiring separate type information at runtime.  */

/* Check if a value is an immediate integer (LSB = 1).  */

bool
ocaml_is_immediate_int (LONGEST val)
{
  return (val & 1) != 0;
}

/* Check if a value is a pointer to a heap block (LSB = 0).  */

bool
ocaml_is_block (LONGEST val)
{
  return (val & 1) == 0 && val != 0;
}

/* Extract the integer value from an immediate int (shift right by 1).

   OCaml represents integer n as (n << 1) | 1, so to get the actual value,
   we perform an arithmetic right shift by 1 to preserve the sign.  */

LONGEST
ocaml_immediate_int_val (LONGEST val)
{
  /* Arithmetic right shift preserves sign for negative numbers.  */
  return val >> 1;
}

/* Read the header of an OCaml block.

   OCaml blocks are stored in memory with a header word immediately before
   the block pointer. The header contains:
   - Bits 0-7: tag (identifies block type)
   - Bits 8-9: color (for garbage collector)
   - Bits 10+: size in words

   Returns true on success, false on memory read error.  */

bool
ocaml_read_block_header (struct gdbarch *gdbarch, CORE_ADDR addr,
			 ULONGEST *header)
{
  int ptr_size = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[8];  /* Enough for 64-bit pointers.  */

  if (ptr_size > sizeof (buf))
    return false;

  /* Read the header word at (addr - ptr_size).  */
  CORE_ADDR header_addr = addr - ptr_size;

  if (target_read_memory (header_addr, buf, ptr_size) != 0)
    return false;

  *header = extract_unsigned_integer (buf, ptr_size, byte_order);
  return true;
}

/* Extract the tag from an OCaml block header.
   The tag is stored in the lower 8 bits of the header.  */

int
ocaml_header_tag (ULONGEST header)
{
  return header & 0xFF;
}

/* Extract the size (in words) from an OCaml block header.
   The size is stored in bits 10 and above.  */

ULONGEST
ocaml_header_size (ULONGEST header)
{
  return header >> 10;
}

/* Implement la_value_print_inner for OCaml.

   OCaml uses a tagged value representation:
   - Immediate integers: LSB = 1, value = raw_value >> 1
   - Block pointers: LSB = 0, points to heap-allocated data
   - Special immediate values: (), true, false, [], None

   This function detects the value type and prints it appropriately.  */

void
ocaml_value_print_inner (struct value *val, struct ui_file *stream, int recurse,
			 const struct value_print_options *options)
{
  struct type *type = check_typedef (val->type ());
  struct gdbarch *gdbarch = type->arch ();

  /* For now, only handle integer-sized values that could be OCaml values.
     Other types (structs, arrays, etc.) delegate to C printing.  */
  if (type->code () != TYPE_CODE_INT && type->code () != TYPE_CODE_PTR)
    {
      c_value_print_inner (val, stream, recurse, options);
      return;
    }

  /* Extract the raw value.  */
  LONGEST raw_val = value_as_long (val);

  /* Check for special immediate values first.
     Note: unit, false, [], and None all have the same representation (1).
     We can't distinguish between them without type information.  */
  if (raw_val == OCAML_VAL_TRUE)
    {
      gdb_puts ("true", stream);
      return;
    }

  /* Check if this is an immediate integer.  */
  if (ocaml_is_immediate_int (raw_val))
    {
      LONGEST int_val = ocaml_immediate_int_val (raw_val);

      /* Could be unit, false, [], or None if value is 0 (raw 1).  */
      if (int_val == 0)
	{
	  /* Without type info, we'll just show it as an int.
	     In Stage 5, we'll use type information to distinguish these.  */
	  gdb_printf (stream, "%s", plongest (int_val));
	}
      else
	{
	  gdb_printf (stream, "%s", plongest (int_val));
	}
      return;
    }

  /* Check if this is a block pointer.  */
  if (ocaml_is_block (raw_val))
    {
      CORE_ADDR addr = (CORE_ADDR) raw_val;
      ULONGEST header;

      /* Try to read the block header.  */
      if (!ocaml_read_block_header (gdbarch, addr, &header))
	{
	  /* Can't read memory, fall back to showing the address.  */
	  gdb_printf (stream, "<block at %s>",
		      paddress (gdbarch, addr));
	  return;
	}

      int tag = ocaml_header_tag (header);
      ULONGEST size = ocaml_header_size (header);

      /* Handle different block types based on tag.  */
      switch (tag)
	{
	case OCAML_TAG_STRING:
	  /* OCaml string - try to print it.  */
	  {
	    /* Read the string data. For now, just show it's a string.
	       Full string printing will be improved in Stage 5.  */
	    gdb_printf (stream, "<string[%s]>", pulongest (size));
	  }
	  break;

	case OCAML_TAG_DOUBLE:
	  /* OCaml float (always 64-bit).  */
	  {
	    gdb_byte buf[8];
	    if (target_read_memory (addr, buf, 8) == 0)
	      {
		double d;
		memcpy (&d, buf, 8);
		gdb_printf (stream, "%g", d);
	      }
	    else
	      {
		gdb_puts ("<float>", stream);
	      }
	  }
	  break;

	case OCAML_TAG_DOUBLE_ARRAY:
	  gdb_printf (stream, "<float array[%s]>", pulongest (size));
	  break;

	case OCAML_TAG_CLOSURE:
	  gdb_printf (stream, "<closure>");
	  break;

	case OCAML_TAG_OBJECT:
	  gdb_printf (stream, "<object>");
	  break;

	case OCAML_TAG_CUSTOM:
	  gdb_printf (stream, "<custom>");
	  break;

	case OCAML_TAG_ABSTRACT:
	  gdb_printf (stream, "<abstract>");
	  break;

	default:
	  /* Structured block (tuple, record, variant, list, etc.).
	     Tags 0-245 are used for variants and structured data.
	     Full ADT printing will be implemented in Stage 5.  */
	  if (tag < OCAML_TAG_LAZY)
	    {
	      gdb_printf (stream, "<block tag=%d size=%s>",
			  tag, pulongest (size));
	    }
	  else
	    {
	      gdb_printf (stream, "<special block tag=%d>", tag);
	    }
	  break;
	}
      return;
    }

  /* Value is 0 (NULL pointer in OCaml, though rare).  */
  gdb_printf (stream, "<null>");
}
