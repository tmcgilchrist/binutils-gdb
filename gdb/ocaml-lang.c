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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   TODO: DWARF Type Information Integration
   ========================================
   The current implementation uses heuristics to interpret OCaml's tagged
   value representation. Many improvements are possible with DWARF type info:

   1. VALUE DISAMBIGUATION
      - Distinguish (), false, [], None (all have runtime value 1)
      - Distinguish arrays from tuples (both use tag 0 blocks)
      - Identify which variant constructor a block represents

   2. SYMBOLIC NAMES
      - Print variant constructor names instead of tag numbers
      - Print record field names instead of positional fields
      - Show type names for custom blocks

   3. DWARF INTEGRATION APPROACH
      - Check value->type() for DWARF DIE information
      - For variant types: Look for DW_TAG_variant_part
      - For records: Extract field names from DW_TAG_member
      - For arrays/tuples: Check type structure to distinguish
      - Map block tags to constructor names via DWARF attributes

   4. EXAMPLE IMPROVEMENTS
      Before (current):  <block tag=0 size=1>
      After (with DWARF): Some 42

      Before (current):  (1, 2)
      After (with DWARF): [|1; 2|]  (if actually an array)

      Before (current):  <block tag=2 size=2>
      After (with DWARF): Point {x=10; y=20}

   See individual TODO comments throughout this file for specific areas.  */

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
    return ocaml_parse (ps);
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

/* Read a field from an OCaml block.

   Fields are stored sequentially after the block pointer, each occupying
   one word (pointer-sized).  Field 0 is at block_addr, field 1 is at
   block_addr + word_size, etc.

   Returns true on success, false on memory read error.  */

bool
ocaml_read_block_field (struct gdbarch *gdbarch, CORE_ADDR block_addr,
			int field_index, LONGEST *value)
{
  int ptr_size = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[8];  /* Enough for 64-bit pointers.  */

  if (ptr_size > sizeof (buf))
    return false;

  /* Read the field at block_addr + (field_index * ptr_size).  */
  CORE_ADDR field_addr = block_addr + (field_index * ptr_size);

  if (target_read_memory (field_addr, buf, ptr_size) != 0)
    return false;

  *value = extract_unsigned_integer (buf, ptr_size, byte_order);
  return true;
}

/* Print an OCaml string block.

   OCaml strings are stored as blocks with tag 252 (OCAML_TAG_STRING).
   The last byte of the last word contains the number of padding bytes
   (0-7), allowing strings to have byte-level precision despite being
   stored in word-sized blocks.  */

void
ocaml_print_string (struct gdbarch *gdbarch, CORE_ADDR addr,
		    ULONGEST size, struct ui_file *stream)
{
  if (size == 0)
    {
      gdb_puts ("\"\"", stream);
      return;
    }

  int ptr_size = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;

  /* Calculate the actual byte length.
     Last byte contains the number of padding bytes.  */
  ULONGEST byte_size = size * ptr_size;
  gdb_byte last_byte;

  if (target_read_memory (addr + byte_size - 1, &last_byte, 1) != 0)
    {
      gdb_printf (stream, "<string[%s]>", pulongest (size));
      return;
    }

  /* The actual string length excludes the padding bytes.  */
  ULONGEST string_len = byte_size - last_byte - 1;

  /* Limit the length to avoid excessive output.  */
  const ULONGEST max_print_len = 200;
  bool truncated = false;
  if (string_len > max_print_len)
    {
      string_len = max_print_len;
      truncated = true;
    }

  /* Read the string content.  */
  gdb::unique_xmalloc_ptr<gdb_byte> buffer
    ((gdb_byte *) xmalloc (string_len + 1));

  if (target_read_memory (addr, buffer.get (), string_len) != 0)
    {
      gdb_printf (stream, "<string[%s]>", pulongest (size));
      return;
    }

  /* Print the string with proper escaping.  */
  gdb_puts ("\"", stream);
  for (ULONGEST i = 0; i < string_len; i++)
    {
      gdb_byte ch = buffer.get ()[i];
      if (ch == '"')
	gdb_puts ("\\\"", stream);
      else if (ch == '\\')
	gdb_puts ("\\\\", stream);
      else if (ch == '\n')
	gdb_puts ("\\n", stream);
      else if (ch == '\t')
	gdb_puts ("\\t", stream);
      else if (ch == '\r')
	gdb_puts ("\\r", stream);
      else if (ch >= 32 && ch < 127)
	gdb_printf (stream, "%c", ch);
      else
	gdb_printf (stream, "\\x%02x", ch);
    }
  if (truncated)
    gdb_puts ("...", stream);
  gdb_puts ("\"", stream);
}

/* Forward declarations for recursive printing.  */
static void ocaml_print_value (struct gdbarch *gdbarch, LONGEST val_raw,
			       struct ui_file *stream, int recurse,
			       const struct value_print_options *options);

/* ========================================================================
   DWARF Type Information Support
   ========================================================================

   The following functions provide integration with DWARF 5 debug information
   for OCaml. They allow GDB to use type information from the compiler to
   provide symbolic output instead of raw memory dumps.

   Key capabilities:
   - Extract variant part information (DW_TAG_variant_part)
   - Read discriminant values from OCaml block headers
   - Match discriminant values to variant constructors
   - Get constructor and field names from DWARF DIEs  */

/* Get the variant part from a type.

   OCaml variant types (sum types) are encoded in DWARF using DW_TAG_variant_part.
   This function extracts the variant_part structure if it exists.

   Returns the variant_part array, or nullptr if the type has no variants.  */

static const gdb::array_view<variant_part> * __attribute__((unused))
ocaml_get_variant_parts (struct type *type)
{
  if (type == nullptr)
    return nullptr;

  /* Check if this type has variant parts attached as a dynamic property.  */
  dynamic_prop *variant_prop = type->dyn_prop (DYN_PROP_VARIANT_PARTS);
  if (variant_prop == nullptr)
    return nullptr;

  if (variant_prop->kind () != PROP_VARIANT_PARTS)
    return nullptr;

  return variant_prop->variant_parts ();
}

/* Read the discriminant value from an OCaml value.

   For OCaml variant types, the discriminant is stored in the block header's
   tag field. This function reads the tag and returns it as the discriminant.

   For immediate values (non-blocks), the discriminant is typically the
   immediate value itself (used for constant constructors).

   Returns the discriminant value, or -1 on error.  */

static LONGEST __attribute__((unused))
ocaml_read_discriminant_from_value (struct gdbarch *gdbarch, struct value *val,
				    const variant_part &part)
{
  LONGEST val_raw = value_as_long (val);

  /* Check if this is an immediate value.  */
  if (ocaml_is_immediate_int (val_raw))
    {
      /* For immediate values, the discriminant is the integer value.
	 Immediate variant constructors are encoded as integers.  */
      return ocaml_immediate_int_val (val_raw);
    }

  /* Check if this is a block pointer.  */
  if (ocaml_is_block (val_raw))
    {
      CORE_ADDR addr = (CORE_ADDR) val_raw;
      ULONGEST header;

      if (!ocaml_read_block_header (gdbarch, addr, &header))
	return -1;

      /* The discriminant is the tag field from the header.  */
      return ocaml_header_tag (header);
    }

  /* Neither immediate nor block - should not happen for variant types.  */
  return -1;
}

/* Find the matching variant for a given discriminant value.

   Iterates through the variants in the variant_part and returns the first
   one that matches the discriminant value.

   Returns the matching variant, or nullptr if no match found.  */

__attribute__((unused))
static const variant *
ocaml_find_matching_variant (const variant_part &part, LONGEST discr_value)
{
  for (const variant &v : part.variants)
    {
      /* Check if this variant matches the discriminant.
	 The is_unsigned parameter should match the discriminant type.  */
      if (v.matches (discr_value, part.is_unsigned))
	return &v;
    }

  /* Check if there's a default variant (no discriminants).  */
  for (const variant &v : part.variants)
    {
      if (v.is_default ())
	return &v;
    }

  return nullptr;
}

/* Get the constructor name for a variant.

   Extracts the name from the DWARF field information associated with
   the variant.

   Returns the constructor name, or nullptr if not available.  */

__attribute__((unused))
static const char *
ocaml_get_constructor_name (struct type *type, const variant &v)
{
  /* The variant controls a range of fields. For OCaml variants,
     the constructor name is typically stored in the first field's name
     or in a special naming convention.

     Note: The exact field that contains the name may vary depending on
     how the OCaml compiler emits DWARF. We need to examine the actual
     DWARF output to determine the correct field.  */

  if (v.first_field >= 0 && v.first_field < type->num_fields ())
    {
      const field &f = type->field (v.first_field);
      return f.name ();
    }

  return nullptr;
}

/* Check if a type is a record type (structure with named fields).

   In DWARF, OCaml records are represented as DW_TAG_structure_type with
   named members (DW_TAG_member). This function checks if the type has
   the characteristics of a record.

   Returns true if this appears to be a record type.  */

static bool __attribute__((unused))
ocaml_is_record_type (struct type *type)
{
  if (type == nullptr)
    return false;

  /* Records are struct types.  */
  if (type->code () != TYPE_CODE_STRUCT)
    return false;

  /* Check if at least one field has a non-empty name.
     Records have named fields, while tuples typically don't.  */
  for (int i = 0; i < type->num_fields (); i++)
    {
      const char *name = type->field (i).name ();
      if (name != nullptr && name[0] != '\0')
	return true;
    }

  return false;
}

/* Check if a type is a tuple type (structure with unnamed fields).

   Tuples are like records but have unnamed or empty field names.

   Returns true if this appears to be a tuple type.  */

static bool __attribute__((unused))
ocaml_is_tuple_type (struct type *type)
{
  if (type == nullptr)
    return false;

  /* Tuples are struct types.  */
  if (type->code () != TYPE_CODE_STRUCT)
    return false;

  /* If it has variant parts, it's a variant, not a tuple.  */
  if (ocaml_get_variant_parts (type) != nullptr)
    return false;

  /* Check if all fields are unnamed.
     If any field has a name, it's a record, not a tuple.  */
  for (int i = 0; i < type->num_fields (); i++)
    {
      const char *name = type->field (i).name ();
      if (name != nullptr && name[0] != '\0')
	return false;
    }

  return true;
}

/* Check if a variant type is unboxed.

   Unboxed variants are single-constructor variants where the payload is stored
   directly without OCaml's block wrapper. In OCaml source:

     type t = A of int [@@unboxed]

   At runtime, this is stored as just the int value, not a block.

   Detection: A variant type with exactly one variant and the variant has
   no discriminant range (or matches all values).  */

static bool
ocaml_is_unboxed_variant (struct type *type)
{
  const gdb::array_view<variant_part> *parts = ocaml_get_variant_parts (type);
  if (parts == nullptr || parts->empty ())
    return false;

  const variant_part &part = (*parts)[0];

  /* Unboxed variants have exactly one variant.  */
  if (part.variants.size () != 1)
    return false;

  /* The single variant should be the default (no specific discriminant).  */
  const variant &v = part.variants[0];
  return v.is_default ();
}

/* Check if a record type is unboxed.

   Unboxed records are single-field records where the field is stored directly
   without the OCaml block wrapper. In OCaml source:

     type t = { x: int } [@@unboxed]

   At runtime, this is stored as just the int value, not a block.

   Detection: A record type with exactly one field.  */

static bool
ocaml_is_unboxed_record (struct type *type)
{
  if (!ocaml_is_record_type (type))
    return false;

  /* Unboxed records have exactly one field.  */
  return type->num_fields () == 1;
}

/* Print an unboxed OCaml variant value using DWARF type information.

   Unboxed variants are single-constructor variants stored without wrapping:
     type t = A of int [@@unboxed]

   The value is stored directly as the payload type, not as an OCaml block.

   Example: For `type t = Wrapper of int [@@unboxed]`, the value 42 is
   stored as immediate int 42, not as a block.

   Returns true if successfully printed, false otherwise.  */

static bool
ocaml_print_unboxed_variant (struct value *val, struct type *type,
			      struct ui_file *stream, int recurse,
			      const struct value_print_options *options)
{
  struct gdbarch *gdbarch = type->arch ();
  const gdb::array_view<variant_part> *parts = ocaml_get_variant_parts (type);

  if (parts == nullptr || parts->empty ())
    return false;

  const variant_part &part = (*parts)[0];
  if (part.variants.size () != 1)
    return false;

  const variant &var = part.variants[0];

  /* Get the constructor name.  */
  const char *constructor_name = ocaml_get_constructor_name (type, var);
  if (constructor_name == nullptr)
    constructor_name = "<unboxed>";

  /* Print the constructor name.  */
  gdb_puts (constructor_name, stream);

  /* If the variant has a field, print the value directly.
     For unboxed variants, the value is NOT in a block - it's stored directly.  */
  if (var.first_field < var.last_field && var.first_field < type->num_fields ())
    {
      gdb_puts (" ", stream);

      /* The value is stored directly - just print it according to its type.  */
      LONGEST raw_val = value_as_long (val);
      ocaml_print_value (gdbarch, raw_val, stream, recurse + 1, options);
    }

  return true;
}

/* Print an OCaml variant value using DWARF type information.

   Variants are OCaml's sum types (type t = A | B of int | C of string).
   With DWARF info, we can print constructor names and fields instead of
   raw tags.

   This function handles both:
   - Regular variants: discriminant values are sequential (0, 1, 2, ...)
   - Polymorphic variants: discriminant values are hash-based (e.g., 0x3c4fc236)

   Example:
   Before: <block tag=0 size=1>
   After:  Some 42

   Polymorphic variant example:
   Before: <block tag=0x3c4fc236 size=1>
   After:  `Blue 42

   Returns true if successfully printed, false if no variant info available.  */

static bool
ocaml_print_variant_with_type (struct value *val, struct type *type,
				struct ui_file *stream, int recurse,
				const struct value_print_options *options)
{
  struct gdbarch *gdbarch = type->arch ();

  /* Get the variant parts from the type.  */
  const gdb::array_view<variant_part> *parts = ocaml_get_variant_parts (type);
  if (parts == nullptr || parts->empty ())
    return false;

  /* OCaml types should have exactly one variant_part.  */
  const variant_part &part = (*parts)[0];

  /* Read the discriminant value from the OCaml value.
     For regular variants, this is a small integer (0, 1, 2, ...).
     For polymorphic variants, this is a hash value (large integer).  */
  LONGEST discr = ocaml_read_discriminant_from_value (gdbarch, val, part);
  if (discr < 0)
    return false;

  /* Find the matching variant for this discriminant.
     The variant::matches() method handles both sequential and hash-based discriminants.  */
  const variant *var = ocaml_find_matching_variant (part, discr);
  if (var == nullptr)
    {
      /* No matching variant found - print raw discriminant.
	 This might be a polymorphic variant hash or an unknown constructor.  */
      gdb_printf (stream, "<variant tag=%s>", plongest (discr));
      return true;
    }

  /* Get the constructor name.  */
  const char *constructor_name = ocaml_get_constructor_name (type, *var);
  if (constructor_name == nullptr)
    constructor_name = "<unknown>";

  /* Print the constructor name.  */
  gdb_puts (constructor_name, stream);

  /* If this variant has fields, print them.  */
  if (var->first_field < var->last_field)
    {
      LONGEST val_raw = value_as_long (val);

      /* Check if this is a block value (not an immediate).  */
      if (ocaml_is_block (val_raw))
	{
	  CORE_ADDR addr = (CORE_ADDR) val_raw;
	  ULONGEST header;

	  if (ocaml_read_block_header (gdbarch, addr, &header))
	    {
	      /* Print fields for this variant.
		 For single-field constructors, print as: Constructor value
		 For multi-field constructors, print as: Constructor (field1, field2, ...)  */
	      int num_fields = var->last_field - var->first_field;

	      if (num_fields == 1)
		{
		  /* Single field - print without parentheses.  */
		  gdb_puts (" ", stream);

		  LONGEST field_val;
		  if (ocaml_read_block_field (gdbarch, addr, 0, &field_val))
		    ocaml_print_value (gdbarch, field_val, stream, recurse + 1, options);
		  else
		    gdb_puts ("<error>", stream);
		}
	      else
		{
		  /* Multiple fields - print with parentheses.  */
		  gdb_puts (" (", stream);

		  for (int i = 0; i < num_fields; i++)
		    {
		      if (i > 0)
			gdb_puts (", ", stream);

		      LONGEST field_val;
		      if (ocaml_read_block_field (gdbarch, addr, i, &field_val))
			ocaml_print_value (gdbarch, field_val, stream, recurse + 1, options);
		      else
			gdb_puts ("<error>", stream);
		    }

		  gdb_puts (")", stream);
		}
	    }
	}
    }

  return true;
}

/* Print an unboxed OCaml record value using DWARF type information.

   Unboxed records are single-field records stored without wrapping:
     type t = { x: int } [@@unboxed]

   The value is stored directly as the field value, not as an OCaml block.

   Example: For `type t = { value: int } [@@unboxed]`, the value 42 is
   stored as immediate int 42, not as a block.

   Returns true if successfully printed, false otherwise.  */

static bool
ocaml_print_unboxed_record (struct value *val, struct type *type,
			     struct ui_file *stream, int recurse,
			     const struct value_print_options *options)
{
  struct gdbarch *gdbarch = type->arch ();

  /* Unboxed records must have exactly one field.  */
  if (type->num_fields () != 1)
    return false;

  const field &f = type->field (0);
  const char *field_name = f.name ();

  /* Print as a record with the single field.  */
  gdb_puts ("{", stream);

  if (field_name != nullptr && field_name[0] != '\0')
    gdb_printf (stream, "%s = ", field_name);

  /* The value is stored directly - just print it.  */
  LONGEST raw_val = value_as_long (val);
  ocaml_print_value (gdbarch, raw_val, stream, recurse + 1, options);

  gdb_puts ("}", stream);
  return true;
}

/* Print an OCaml record value using DWARF type information.

   Records are OCaml's product types with named fields (type t = { x: int; y: float }).
   With DWARF info, we can print field names instead of positions.

   Example:
   Before: <block tag=0 size=2>
   After:  {x = 10; y = 20}

   Returns true if successfully printed, false if no record info available.  */

static bool
ocaml_print_record_with_type (struct value *val, struct type *type,
			       struct ui_file *stream, int recurse,
			       const struct value_print_options *options)
{
  struct gdbarch *gdbarch = type->arch ();
  LONGEST val_raw = value_as_long (val);

  /* Records must be blocks.  */
  if (!ocaml_is_block (val_raw))
    return false;

  CORE_ADDR addr = (CORE_ADDR) val_raw;

  /* Print record fields with their names.  */
  gdb_puts ("{", stream);

  int num_fields = type->num_fields ();
  bool first = true;

  for (int i = 0; i < num_fields; i++)
    {
      const field &f = type->field (i);
      const char *field_name = f.name ();

      /* Skip fields without names (shouldn't happen for records).  */
      if (field_name == nullptr || field_name[0] == '\0')
	continue;

      if (!first)
	gdb_puts ("; ", stream);
      first = false;

      gdb_printf (stream, "%s = ", field_name);

      /* Read field value from the block.  */
      LONGEST field_val;
      if (ocaml_read_block_field (gdbarch, addr, i, &field_val))
	ocaml_print_value (gdbarch, field_val, stream, recurse + 1, options);
      else
	gdb_puts ("<error>", stream);
    }

  gdb_puts ("}", stream);
  return true;
}

/* Print an OCaml array value using DWARF type information.

   Arrays are printed as [|elem0; elem1; elem2; ...|].
   With DWARF info, we can distinguish arrays from tuples.

   Returns true if successfully printed, false if not an array.  */

static bool
ocaml_print_array_with_type (struct value *val, struct type *type,
			      struct ui_file *stream, int recurse,
			      const struct value_print_options *options)
{
  struct gdbarch *gdbarch = type->arch ();
  LONGEST val_raw = value_as_long (val);

  /* Arrays must be blocks.  */
  if (!ocaml_is_block (val_raw))
    return false;

  CORE_ADDR addr = (CORE_ADDR) val_raw;
  ULONGEST header;

  if (!ocaml_read_block_header (gdbarch, addr, &header))
    return false;

  ULONGEST size = ocaml_header_size (header);

  /* Print array elements.  */
  if (size == 0)
    {
      gdb_puts ("[||]", stream);
      return true;
    }

  gdb_puts ("[|", stream);

  /* Limit array elements to avoid excessive output.  */
  ULONGEST max_elems = 10;
  bool truncated = false;
  ULONGEST print_size = size;
  if (size > max_elems)
    {
      print_size = max_elems;
      truncated = true;
    }

  for (ULONGEST i = 0; i < print_size; i++)
    {
      if (i > 0)
	gdb_puts ("; ", stream);

      LONGEST field_val;
      if (ocaml_read_block_field (gdbarch, addr, i, &field_val))
	ocaml_print_value (gdbarch, field_val, stream, recurse + 1, options);
      else
	gdb_puts ("<error>", stream);
    }

  if (truncated)
    gdb_printf (stream, "; ... (%s more)", pulongest (size - print_size));

  gdb_puts ("|]", stream);
  return true;
}

/* Print an OCaml tuple value using DWARF type information.

   Tuples are printed as (elem0, elem1, elem2, ...).
   With DWARF info, we can distinguish tuples from arrays and records.

   Returns true if successfully printed, false if not a tuple.  */

static bool
ocaml_print_tuple_with_type (struct value *val, struct type *type,
			      struct ui_file *stream, int recurse,
			      const struct value_print_options *options)
{
  struct gdbarch *gdbarch = type->arch ();
  LONGEST val_raw = value_as_long (val);

  /* Tuples must be blocks.  */
  if (!ocaml_is_block (val_raw))
    return false;

  CORE_ADDR addr = (CORE_ADDR) val_raw;
  int num_fields = type->num_fields ();

  if (num_fields == 0)
    {
      gdb_puts ("()", stream);
      return true;
    }

  gdb_puts ("(", stream);

  for (int i = 0; i < num_fields; i++)
    {
      if (i > 0)
	gdb_puts (", ", stream);

      LONGEST field_val;
      if (ocaml_read_block_field (gdbarch, addr, i, &field_val))
	ocaml_print_value (gdbarch, field_val, stream, recurse + 1, options);
      else
	gdb_puts ("<error>", stream);
    }

  gdb_puts (")", stream);
  return true;
}

/* Dispatch value printing based on DWARF type information.

   This function checks if DWARF type information is available and dispatches
   to the appropriate type-aware printing function.

   Returns true if the value was printed using type information, false otherwise.  */

static bool
ocaml_print_with_type (struct value *val, struct ui_file *stream, int recurse,
		       const struct value_print_options *options)
{
  struct type *type = val->type ();
  if (type == nullptr)
    return false;

  /* Resolve typedefs to get the actual type.  */
  type = check_typedef (type);

  /* Check for unboxed variant types FIRST.
     Unboxed variants need special handling since the value is stored
     directly without OCaml's block wrapper.  */
  if (ocaml_is_unboxed_variant (type))
    {
      return ocaml_print_unboxed_variant (val, type, stream, recurse, options);
    }

  /* Check for regular variant types (sum types with variant_part).  */
  if (ocaml_get_variant_parts (type) != nullptr)
    {
      return ocaml_print_variant_with_type (val, type, stream, recurse, options);
    }

  /* Check for array types.  */
  if (type->code () == TYPE_CODE_ARRAY)
    {
      return ocaml_print_array_with_type (val, type, stream, recurse, options);
    }

  /* Check for unboxed record types.
     Must check before regular records since unboxed records are a subset.  */
  if (ocaml_is_unboxed_record (type))
    {
      return ocaml_print_unboxed_record (val, type, stream, recurse, options);
    }

  /* Check for record types (product types with named fields).  */
  if (ocaml_is_record_type (type))
    {
      return ocaml_print_record_with_type (val, type, stream, recurse, options);
    }

  /* Check for tuple types (product types with unnamed fields).  */
  if (ocaml_is_tuple_type (type))
    {
      return ocaml_print_tuple_with_type (val, type, stream, recurse, options);
    }

  /* No applicable type information - fall back to heuristic printing.  */
  return false;
}

/* Print an OCaml tuple (block with tag 0, no specific ADT type).
   Tuples are printed as (field0, field1, field2, ...).  */

static void
ocaml_print_tuple (struct gdbarch *gdbarch, CORE_ADDR addr, ULONGEST size,
		   struct ui_file *stream, int recurse,
		   const struct value_print_options *options)
{
  if (size == 0)
    {
      gdb_puts ("()", stream);
      return;
    }

  gdb_puts ("(", stream);
  for (ULONGEST i = 0; i < size; i++)
    {
      if (i > 0)
	gdb_puts (", ", stream);

      LONGEST field_val;
      if (ocaml_read_block_field (gdbarch, addr, i, &field_val))
	ocaml_print_value (gdbarch, field_val, stream, recurse + 1, options);
      else
	gdb_puts ("<error>", stream);
    }
  gdb_puts (")", stream);
}

/* Print an OCaml array.
   Arrays are printed as [|elem0; elem1; elem2; ...|].

   TODO: Array vs Tuple Disambiguation
   - OCaml arrays and tuples both use tag 0 blocks
   - Without DWARF type information, we cannot reliably distinguish them
   - This function is kept for future use when type information becomes available
   - Future work: Check DWARF DIE type to determine if a tag 0 block is:
     * An array ('a array type)
     * A tuple ('a * 'b * ... type)
     * A record (may also use tag 0 depending on optimization)
   - Currently, all tag 0 blocks are printed as tuples unless they match
     the list pattern (size 2 with proper tail chain) */

static void __attribute__((unused))
ocaml_print_array (struct gdbarch *gdbarch, CORE_ADDR addr, ULONGEST size,
		   struct ui_file *stream, int recurse,
		   const struct value_print_options *options)
{
  if (size == 0)
    {
      gdb_puts ("[||]", stream);
      return;
    }

  gdb_puts ("[|", stream);

  /* Limit array elements to avoid excessive output.  */
  ULONGEST max_elems = 10;
  bool truncated = false;
  ULONGEST print_size = size;
  if (size > max_elems)
    {
      print_size = max_elems;
      truncated = true;
    }

  for (ULONGEST i = 0; i < print_size; i++)
    {
      if (i > 0)
	gdb_puts ("; ", stream);

      LONGEST field_val;
      if (ocaml_read_block_field (gdbarch, addr, i, &field_val))
	ocaml_print_value (gdbarch, field_val, stream, recurse + 1, options);
      else
	gdb_puts ("<error>", stream);
    }

  if (truncated)
    gdb_printf (stream, "; ... (%s more)", pulongest (size - print_size));

  gdb_puts ("|]", stream);
}

/* Print an OCaml list.
   Lists use tag 0 blocks with 2 fields: head and tail.
   Empty list is represented by immediate value 1 ([]).

   TODO: List Detection Heuristic Limitations
   - Current detection: tag 0, size 2, tail is [] or another block
   - This heuristic may incorrectly identify other ADTs as lists:
     * ('a * 'b) tuples where second element is [] or a block
     * Binary trees or other recursive structures with similar layout
     * Some variant constructors with 2 fields
   - Future work: Use DWARF type information to confirm list type
   - Better approach: Check if type is "list" or has DW_TAG_variant_part
     with constructors named [] and :: */

static void
ocaml_print_list (struct gdbarch *gdbarch, LONGEST list_val,
		  struct ui_file *stream, int recurse,
		  const struct value_print_options *options)
{
  /* Check for empty list.  */
  if (list_val == OCAML_VAL_EMPTY_LIST)
    {
      gdb_puts ("[]", stream);
      return;
    }

  /* Limit list length to avoid infinite loops and excessive output.  */
  const int max_list_len = 20;
  int count = 0;

  gdb_puts ("[", stream);

  LONGEST current = list_val;
  bool first = true;

  while (ocaml_is_block (current) && count < max_list_len)
    {
      CORE_ADDR addr = (CORE_ADDR) current;
      ULONGEST header;

      if (!ocaml_read_block_header (gdbarch, addr, &header))
	break;

      int tag = ocaml_header_tag (header);
      ULONGEST size = ocaml_header_size (header);

      /* List cons cells have tag 0 and size 2 (head, tail).  */
      if (tag != 0 || size != 2)
	break;

      if (!first)
	gdb_puts ("; ", stream);
      first = false;

      /* Print the head element.  */
      LONGEST head;
      if (!ocaml_read_block_field (gdbarch, addr, 0, &head))
	{
	  gdb_puts ("<error>", stream);
	  break;
	}

      ocaml_print_value (gdbarch, head, stream, recurse + 1, options);

      /* Get the tail.  */
      LONGEST tail;
      if (!ocaml_read_block_field (gdbarch, addr, 1, &tail))
	{
	  gdb_puts ("; <error>", stream);
	  break;
	}

      current = tail;
      count++;

      /* Check if we've reached the end of the list.  */
      if (current == OCAML_VAL_EMPTY_LIST)
	break;
    }

  if (count >= max_list_len && current != OCAML_VAL_EMPTY_LIST)
    gdb_puts ("; ...", stream);
  else if (current != OCAML_VAL_EMPTY_LIST && ocaml_is_block (current))
    {
      /* Non-standard tail (not a proper list).  */
      gdb_puts (" | ", stream);
      ocaml_print_value (gdbarch, current, stream, recurse + 1, options);
    }

  gdb_puts ("]", stream);
}

/* Print a single OCaml value given its raw representation.
   This is the core recursive printing function.  */

static void
ocaml_print_value (struct gdbarch *gdbarch, LONGEST val_raw,
		   struct ui_file *stream, int recurse,
		   const struct value_print_options *options)
{
  /* Prevent excessive recursion.  */
  if (recurse > 10)
    {
      gdb_puts ("...", stream);
      return;
    }

  /* Check for special immediate values.  */
  if (val_raw == OCAML_VAL_TRUE)
    {
      gdb_puts ("true", stream);
      return;
    }

  /* Check if this is an immediate integer.  */
  if (ocaml_is_immediate_int (val_raw))
    {
      LONGEST int_val = ocaml_immediate_int_val (val_raw);

      /* Special case: 0 could be false, (), [], or None.
	 Without type information, we just show the integer.  */
      gdb_printf (stream, "%s", plongest (int_val));
      return;
    }

  /* Check if this is a block pointer.  */
  if (ocaml_is_block (val_raw))
    {
      CORE_ADDR addr = (CORE_ADDR) val_raw;
      ULONGEST header;

      if (!ocaml_read_block_header (gdbarch, addr, &header))
	{
	  gdb_printf (stream, "<block at %s>", paddress (gdbarch, addr));
	  return;
	}

      int tag = ocaml_header_tag (header);
      ULONGEST size = ocaml_header_size (header);

      /* Handle special block types.  */
      if (tag == OCAML_TAG_STRING)
	{
	  ocaml_print_string (gdbarch, addr, size, stream);
	  return;
	}
      else if (tag == OCAML_TAG_DOUBLE)
	{
	  gdb_byte buf[8];
	  if (target_read_memory (addr, buf, 8) == 0)
	    {
	      double d;
	      memcpy (&d, buf, 8);
	      gdb_printf (stream, "%g", d);
	    }
	  else
	    gdb_puts ("<float>", stream);
	  return;
	}
      else if (tag == OCAML_TAG_DOUBLE_ARRAY)
	{
	  /* TODO: Print individual float array elements
	     - Currently only shows the array size
	     - Future work: Read and print each float element
	     - Elements are stored as raw 64-bit floats (not tagged)
	     - Example output: [|1.0; 2.5; 3.14|] */
	  gdb_printf (stream, "<float array[%s]>", pulongest (size));
	  return;
	}
      else if (tag == 0)
	{
	  /* Tag 0 can be tuple, list cons, option Some, or other ADTs.
	     Try to detect lists (size 2 pattern).  */
	  if (size == 2)
	    {
	      /* Could be a list cons cell. Let's check if it looks like one
		 by trying to traverse it.  */
	      LONGEST tail;
	      if (ocaml_read_block_field (gdbarch, addr, 1, &tail)
		  && (tail == OCAML_VAL_EMPTY_LIST || ocaml_is_block (tail)))
		{
		  /* Likely a list. Print as list.  */
		  ocaml_print_list (gdbarch, val_raw, stream, recurse, options);
		  return;
		}
	    }

	  /* Otherwise, print as tuple or generic block.  */
	  ocaml_print_tuple (gdbarch, addr, size, stream, recurse, options);
	  return;
	}
      else if (tag == OCAML_TAG_CLOSURE)
	{
	  /* TODO: Enhanced Closure Printing
	     - Could show the function's code pointer and captured environment
	     - Future work: Print function name if available via symbol table
	     - Could show captured variables if DWARF info available
	     - Example: <closure: foo(x=42, y=3.14)> */
	  gdb_puts ("<closure>", stream);
	  return;
	}
      else if (tag == OCAML_TAG_OBJECT)
	{
	  /* TODO: Object Field Printing
	     - Objects store their class information and fields
	     - Field 0: class structure pointer
	     - Fields 1+: instance variables
	     - Future work: Parse class structure to get field names and types
	     - Could print as {field1=value1; field2=value2; ...} */
	  gdb_puts ("<object>", stream);
	  return;
	}
      else if (tag == OCAML_TAG_CUSTOM)
	{
	  /* TODO: Custom Block Interpretation
	     - Custom blocks have operations defined in C
	     - Field 0: pointer to custom operations structure
	     - Fields 1+: custom data
	     - Custom operations include:
	       * identifier string
	       * finalize, compare, hash, serialize functions
	     - Future work: Read identifier and dispatch to type-specific printers
	     - Examples: Int64.t, Bigarray.t, Unix file descriptors
	     - Could show as <Int64: 9223372036854775807> */
	  gdb_puts ("<custom>", stream);
	  return;
	}
      else if (tag < OCAML_TAG_LAZY)
	{
	  /* TODO: Variant Constructor and Record Printing
	     - Tags 0-245 are used for variant constructors and records
	     - Tag value indicates which constructor of a variant type
	     - Without DWARF type information, we cannot determine:
	       * The type name (e.g., "option", "list", custom types)
	       * The constructor name (e.g., "Some", "None", "Cons", custom)
	       * Field names for records
	     - Future work: Parse DWARF DIE to extract:
	       * DW_TAG_variant_part for variant types
	       * DW_TAG_variant for each constructor
	       * DW_TAG_member for record fields
	     - Examples of desired output:
	       * Some 42 (instead of <block tag=0 size=1>)
	       * Red (instead of <block tag=0 size=0>)
	       * {name="Alice"; age=30} (instead of <block tag=0 size=2>)
	     - Current implementation: Show raw tag and size */
	  gdb_printf (stream, "<block tag=%d size=%s>", tag, pulongest (size));
	  return;
	}
      else
	{
	  gdb_printf (stream, "<special block tag=%d>", tag);
	  return;
	}
    }

  /* Value is 0 (NULL).  */
  gdb_puts ("<null>", stream);
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

  /* Try DWARF-based printing first if type information is available.
     This handles variants, records, and other structured types.  */
  if (type->code () == TYPE_CODE_STRUCT || TYPE_HAS_VARIANT_PARTS (type))
    {
      if (ocaml_print_with_type (val, stream, recurse, options))
	return;
    }

  /* For integer and pointer types, try heuristic-based OCaml value printing.  */
  if (type->code () == TYPE_CODE_INT || type->code () == TYPE_CODE_PTR)
    {
      /* Extract the raw value and delegate to the recursive printer.  */
      LONGEST raw_val = value_as_long (val);
      ocaml_print_value (gdbarch, raw_val, stream, recurse, options);
      return;
    }

  /* For other types (arrays, etc.), delegate to C printing.  */
  c_value_print_inner (val, stream, recurse, options);
}
