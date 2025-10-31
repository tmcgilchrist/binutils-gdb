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
/* TODO This should be the startup.S or entry for the main module. We
   don't really have a main function like C or Rust. */
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

   TODO Have more detailed examples here.
   Examples:
   - _O4List3map -> List.map
   - _OModuleA__ModuleB__function -> ModuleA.ModuleB.function  */

gdb::unique_xmalloc_ptr<char>
ocaml_demangle (const char *symbol, int options)
{
  /* Try OCaml-specific demangling using libiberty's oxcaml_demangle.  */
  return gdb_demangle (symbol, options | DMGL_OXCAML);
}

/* Forward declaration for helper function.  */
static gdb::unique_xmalloc_ptr<char> ocaml_get_qualified_type_name (struct type *type);

/* Forward declaration for value printing.  */
static void ocaml_value_print (struct value *val, struct ui_file *stream,
			       const struct value_print_options *options);

/* Forward declaration for record printing.  */
static bool ocaml_print_record_with_type (struct value *val, struct type *type,
					  struct ui_file *stream, int recurse,
					  const struct value_print_options *options);

/* Print an OCaml type with clean module-qualified names.

   This function enhances the default C type printer by cleaning up
   OCaml-specific type name suffixes (like "@ value") while preserving
   module qualification.

   For example:
   - "String.t @ value" is displayed as "String.t"
   - "Module.Submodule.typename @ value" as "Module.Submodule.typename"  */

static void
ocaml_print_type (struct type *type, const char *varstring,
		  struct ui_file *stream, int show, int level,
		  const struct type_print_options *flags)
{
  if (type == NULL)
    {
      c_print_type (type, varstring, stream, show, level, language_ocaml,
		    flags);
      return;
    }

  /* Get the clean module-qualified type name.  */
  gdb::unique_xmalloc_ptr<char> clean_name = ocaml_get_qualified_type_name (type);

  /* Temporarily replace the type name with the clean version for printing.  */
  const char *original_name = type->name ();

  if (clean_name != NULL && original_name != NULL
      && strcmp (clean_name.get (), original_name) != 0)
    {
      /* The name was cleaned up - temporarily set it.  */
      type->set_name (clean_name.get ());
      c_print_type (type, varstring, stream, show, level, language_ocaml,
		    flags);
      /* Restore the original name.  */
      type->set_name (original_name);
    }
  else
    {
      /* No cleanup needed, use default printing.  */
      c_print_type (type, varstring, stream, show, level, language_ocaml,
		    flags);
    }
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
    ocaml_print_type (type, varstring, stream, show, level, flags);
  }

  /* See language.h.  */

  void value_print (struct value *val, struct ui_file *stream,
		    const struct value_print_options *options) const override
  {
    return ocaml_value_print (val, stream, options);
  }

  /* See language.h.  */

  void value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const override
  {
    /* Pass value's type to preserve DWARF information from the start.
       This enables type-aware printing throughout the recursion. */
    return ocaml_value_print_inner (val, stream, recurse, options, val->type ());
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
  if (result == NULL)
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

/* Print an OCaml float value with proper formatting.
   Whole numbers print with .0 suffix (e.g., 1.0 not 1).
   Negative values and unboxed floats are handled appropriately. */

static void
ocaml_print_float (double float_val, bool is_unboxed, struct ui_file *stream)
{
  const char *prefix = is_unboxed ? "#" : "";
  bool is_whole = (float_val == (double)(long long)float_val &&
                   float_val >= LLONG_MIN && float_val <= LLONG_MAX);

  if (float_val < 0)
    {
      if (is_whole)
        gdb_printf (stream, "-%s%.1f", prefix, -float_val);
      else
        gdb_printf (stream, "-%s%g", prefix, -float_val);
    }
  else
    {
      if (is_whole)
        gdb_printf (stream, "%s%.1f", prefix, float_val);
      else
        gdb_printf (stream, "%s%g", prefix, float_val);
    }
}

/* Find enum field name by its enumeration value.
   Returns NULL if not found. */

static const char *
ocaml_find_enum_name_by_value (struct type *enum_type, LONGEST value)
{
  if (enum_type == NULL || enum_type->code () != TYPE_CODE_ENUM)
    return NULL;

  for (int i = 0; i < enum_type->num_fields (); ++i)
    {
      if (enum_type->field (i).loc_enumval () == value)
        return enum_type->field (i).name ();
    }

  return NULL;
}

/* Check if a type is a pointer or reference type.
   Returns true if the type code is TYPE_CODE_REF or TYPE_CODE_PTR. */

static bool
ocaml_type_is_pointer (struct type *type)
{
  if (type == NULL)
    return false;

  type_code code = type->code ();
  return (code == TYPE_CODE_REF || code == TYPE_CODE_PTR);
}

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

  /* Assert buffer is large enough for safety.  */
  gdb_assert (ptr_size <= sizeof (buf));

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

  /* Assert buffer is large enough for safety.  */
  gdb_assert (ptr_size <= sizeof (buf));

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

  /* TODO Is there a more principled way to truncate a long string being printed? */
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
			       const struct value_print_options *options,
			       struct type *dwarf_type = NULL);

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

   Returns the variant_part array, or NULL if the type has no variants.  */

static const gdb::array_view<variant_part> *
ocaml_get_variant_parts (struct type *type)
{
  if (type == NULL)
    return NULL;

  /* Check if this type has variant parts attached as a dynamic property.  */
  dynamic_prop *variant_prop = type->dyn_prop (DYN_PROP_VARIANT_PARTS);
  if (variant_prop == NULL)
    return NULL;

  /* After variant type resolution, GDB rewrites the property kind from
     PROP_VARIANT_PARTS to PROP_TYPE and stores the original (unresolved) type.
     In this case, we need to get the original type and check for variant_parts there.

     See gdbtypes.h lines 290-294:
     "Once a variant type is resolved, we may want to be able to go from the
      resolved type to the original type. In this case we rewrite the property's
      kind and set this field [original_type]."  */
  if (variant_prop->kind () == PROP_TYPE)
    {
      /* Variant type has been resolved - get original type with variant_parts.  */
      struct type *original = variant_prop->original_type ();
      if (original != NULL)
        {
          /* Recursively check the original type for variant_parts.
             This should have PROP_VARIANT_PARTS kind.  */
          dynamic_prop *original_prop = original->dyn_prop (DYN_PROP_VARIANT_PARTS);
          if (original_prop != NULL && original_prop->kind () == PROP_VARIANT_PARTS)
            return original_prop->variant_parts ();
        }
      /* Original type doesn't have variant_parts - this shouldn't happen.  */
      return NULL;
    }

  /* Normal case - property has PROP_VARIANT_PARTS kind.  */
  if (variant_prop->kind () == PROP_VARIANT_PARTS)
    return variant_prop->variant_parts ();

  /* Unexpected property kind - neither PROP_VARIANT_PARTS nor PROP_TYPE.  */
  return NULL;
}

/* Check if this variant_part represents an unboxed variant.
   Unboxed variants have discriminant stored in bit 0 of the same value
   containing the data (bits 1-63 for float64, bits 1-31 for int32, etc.).

   Returns true if the discriminant member has bit_size=1 and bit_offset=0.

   IMPORTANT: This check alone is not sufficient. OxCaml also uses bit-level
   discriminants for some regular variants. An unboxed variant must ALSO not
   have any reference/pointer fields, since all data must be packed inline.  */

static bool
ocaml_is_unboxed_variant (struct type *type, const variant_part &part)
{
  /* Check if the discriminant_index is valid.  */
  if (part.discriminant_index < 0 || part.discriminant_index >= type->num_fields ())
    return false;

  /* Get the discriminant field.  */
  const field &discr_field = type->field (part.discriminant_index);

  /* Check if it has bit-level encoding (bit_size = 1, bit_offset = 0).  */
  if (discr_field.bitsize () != 1 || discr_field.loc_bitpos () != 0)
    return false;

  /* Bit-level discriminant found. This is strong evidence of an unboxed variant.

     For single-constructor unboxed variants like `ValueInt of int [@@unboxed]`,
     the data field will be typed as `ocaml_value` even though it's stored inline.
     This is because immediate OCaml values (int, bool, char) use the `ocaml_value`
     type in DWARF even when unboxed.

     We check two things to distinguish true unboxed variants from regular variants:
     1. Reference/pointer fields - unboxed variants cannot have these
     2. Discriminant enum names - unboxed use internal names (Pointer/Immediate),
        regular variants use constructor names (Leaf/Node/etc.)

     NOTE: ocaml_value fields are ALLOWED in unboxed variants - they just represent
     immediate values (tagged ints, bools) stored inline, not pointers to blocks.  */
  /* First check: look for reference/pointer fields.  */
  for (int i = 0; i < type->num_fields (); ++i)
    {
      const field &f = type->field (i);
      struct type *field_type = check_typedef (f.type ());

      /* Check for explicit pointer/reference types.  */
      if (ocaml_type_is_pointer (field_type))
	{
	  /* Found a reference/pointer field, so this is NOT an unboxed variant.
	     It's a regular variant with a bit-level discriminant.  */
	  return false;
	}
    }

  /* Second check: examine discriminant enum names.
     For nested regular variants, DWARF may not include the reference field in the
     typedef, so we also check if the discriminant enum has internal names
     (Pointer/Immediate) which indicate a regular variant, not truly unboxed.

     True unboxed variants (@@unboxed attribute) have discriminant enums with
     constructor names (ValueInt, ValueFloat, etc.), not Pointer/Immediate.  */
  struct type *discr_type = check_typedef (discr_field.type ());

  if (discr_type->code () == TYPE_CODE_ENUM && discr_type->num_fields () > 0)
    {
      /* Check if enum has "Pointer" or "Immediate" values - these are internal
         discriminants for regular variants, not constructor names.  */
      for (int i = 0; i < discr_type->num_fields (); ++i)
	{
	  const char *enum_name = discr_type->field (i).name ();
	  if (enum_name != NULL &&
	      (strcmp (enum_name, "Pointer") == 0 || strcmp (enum_name, "Immediate") == 0))
	    {
	      /* Found Pointer/Immediate - this is a regular variant's internal discriminant,
		 not a true unboxed variant. Return false.  */
	      return false;
	    }
	}
    }

  /* Bit-level discriminant AND no reference/pointer fields AND no Pointer/Immediate enum:
     this is a true unboxed variant.  */
  return true;
}

/* Check if a struct type is a variant (sum type) vs a record (product type).

   Both variants and records are represented as TYPE_CODE_STRUCT in DWARF, but:
   - Variants have DW_TAG_variant_part (checked via variant_parts property)
   - Records do not have variant_part information

   This distinction is important when printing nested structures:
   - Variant fields should be printed as: (Constructor data...)
   - Record fields should be printed as: {field = value; ...}

   Returns true if the type is a variant, false if it's a record or other struct.  */

static bool
ocaml_is_variant_struct (struct type *type)
{
  if (type == NULL || type->code () != TYPE_CODE_STRUCT)
    return false;

  /* Check if this type has variant parts - if yes, it's a variant.  */
  const gdb::array_view<variant_part> *parts = ocaml_get_variant_parts (type);

  return (parts != NULL);
}

/* Check if a variant should print its fields as a record.
   Returns true if the variant has multiple named fields, indicating
   an inline record constructor like: Constructor {field1=val1; field2=val2}

   Parameters:
   - type: The parent struct type containing all variant fields
   - var: The active variant (from variant_parts matching the discriminant)

   Returns: true if this variant's fields should be formatted as a record  */

static bool
ocaml_variant_has_inline_record (struct type *type, const variant &var)
{
  if (type == NULL)
    return false;

  /* Count how many fields controlled by this variant have names */
  int named_field_count = 0;

  for (int i = var.first_field; i < var.last_field; ++i)
    {
      if (i < 0 || i >= type->num_fields ())
	continue;

      const char *field_name = type->field (i).name ();
      if (field_name != NULL && field_name[0] != '\0')
	named_field_count++;
    }

  /* If we have 2 or more named fields, this is an inline record */
  return (named_field_count >= 2);
}

/* Read the discriminant value from an OCaml value.

   For OCaml variant types, the discriminant is stored in the block header's
   tag field. This function reads the tag and returns it as the discriminant.

   For immediate values (non-blocks), the discriminant is typically the
   immediate value itself (used for constant constructors).

   Returns the discriminant value, or -1 on error.  */

static LONGEST
ocaml_read_discriminant_from_value (struct gdbarch *gdbarch, struct value *val,
				    const variant_part &part)
{
  LONGEST val_raw;

  /* For struct-typed values (DWARF types), we need to read the raw contents.
     OCaml values are always pointer-sized at runtime.  */
  struct type *type = check_typedef (val->type ());
  if (type->code () == TYPE_CODE_STRUCT)
    {
      /* Read the raw pointer value from the struct's memory.  */
      const gdb_byte *contents = val->contents ().data ();
      int ptr_size = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

      if (ptr_size > 8)
	return -1;

      val_raw = extract_unsigned_integer (contents, ptr_size, byte_order);

      /* Check if this is an unboxed variant (discriminant in bit 0).  */
      if (ocaml_is_unboxed_variant (type, part))
	{
	  /* For unboxed variants, the discriminant is bit 0 of the value.
	     Bits 1-63 contain the actual data (float64, int32, etc.).  */
	  return val_raw & 0x1;
	}
    }
  else
    {
      /* For integer/pointer types, use value_as_long directly.  */
      val_raw = value_as_long (val);
    }

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

   Returns the matching variant, or NULL if no match found.  */

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

  return NULL;
}

/* Get the constructor name for a variant.

   Extracts the name from the DWARF field information associated with
   the variant.

   Returns the constructor name, or NULL if not available.  */

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

  /* No valid field found - return NULL to indicate no constructor name available.  */
  return NULL;
}

/* Check if a type is a record type (structure with named fields).

   In DWARF, OCaml records are represented as DW_TAG_structure_type with
   named members (DW_TAG_member). This function checks if the type has
   the characteristics of a record.

   Returns true if this appears to be a record type.  */

static bool
ocaml_is_record_type (struct type *type)
{
  if (type == NULL)
    return false;

  /* Records are struct types.  */
  if (type->code () != TYPE_CODE_STRUCT)
    return false;

  /* Check if at least one field has a non-empty name.
     Records have named fields, while tuples typically don't.  */
  for (int i = 0; i < type->num_fields (); i++)
    {
      const char *name = type->field (i).name ();
      if (name != NULL && name[0] != '\0')
	return true;
    }

  return false;
}

/* Check if a type is a tuple type (structure with unnamed fields).

   Tuples are like records but have unnamed or empty field names.

   Returns true if this appears to be a tuple type.  */

static bool
ocaml_is_tuple_type (struct type *type)
{
  if (type == NULL)
    return false;

  /* Tuples are struct types.  */
  if (type->code () != TYPE_CODE_STRUCT)
    return false;

  /* If it has variant parts, it's a variant, not a tuple.  */
  if (ocaml_get_variant_parts (type) != NULL)
    return false;

  /* Check if all fields are unnamed.
     If any field has a name, it's a record, not a tuple.  */
  for (int i = 0; i < type->num_fields (); i++)
    {
      const char *name = type->field (i).name ();
      if (name != NULL && name[0] != '\0')
	return false;
    }

  return true;
}

/* Check if a type is an OCaml reference type.

   OCaml references are mutable containers declared as:
     type 'a ref = { mutable contents : 'a }

   At runtime, references are blocks with tag 0 and size 1.
   The single field contains the referenced value.

   In DWARF, reference types have:
   - Type name containing "ref" (e.g., "int ref @ value")
   - Single field named "contents"
   - Structure type with one field

   Returns true if this is a reference type.  */

/* Check if a type represents an OCaml array.
   DWARF-based detection (primary): OCaml arrays are represented as typedef'd
   types that resolve to TYPE_CODE_ARRAY. This structural check is more reliable
   than name matching.
   Heuristic fallback: If DWARF structure check doesn't confirm it's an array,
   falls back to checking type name for " array" pattern. This may produce false
   positives for non-array types with "array" in their names.  */

static bool
ocaml_is_array_type (struct type *type)
{
  if (type == NULL)
    return false;

  /* DWARF-based detection: Check if this type resolves to TYPE_CODE_ARRAY.
     OCaml arrays are typedef'd enums that become TYPE_CODE_ARRAY after
     typedef resolution.  */
  struct type *resolved_type = check_typedef (type);
  if (resolved_type != NULL && resolved_type->code () == TYPE_CODE_ARRAY)
    return true;

  /* HEURISTIC FALLBACK: Check type name for " array" pattern.
     WARNING: This may misidentify non-array types with "array" in their names.
     For accurate detection, ensure DWARF debug info is available.  */
  const char *type_name = type->name ();
  if (type_name == NULL)
    return false;

  /* Check if type name contains " array" (may be followed by " @ value").
     We use strstr to find the pattern anywhere in the type name.  */
  const char *array_pos = strstr (type_name, " array");
  if (array_pos == NULL)
    return false;

  /* Make sure it's followed by either end-of-string or " @".  */
  const char *after = array_pos + strlen (" array");
  return (*after == '\0' || strncmp (after, " @", 2) == 0);
}

/* Helper: Check if a variant type has constructors with specific names.

   This function examines the DWARF variant_part to check if the type has
   constructors matching the provided names. Used for DWARF-based type detection.

   Returns true if ALL specified constructor names are found in the variant's enum.  */

static bool
ocaml_variant_has_constructors (struct type *type, const char **constructor_names, int count)
{
  if (type == NULL || constructor_names == NULL || count <= 0)
    return false;

  /* Get variant parts from DWARF.  */
  const gdb::array_view<variant_part> *parts = ocaml_get_variant_parts (type);
  if (parts == NULL || parts->empty ())
    return false;

  const variant_part &part = (*parts)[0];

  /* Get the discriminant enum type.  */
  if (part.discriminant_index < 0 || part.discriminant_index >= type->num_fields ())
    return false;

  struct type *enum_type = check_typedef (type->field (part.discriminant_index).type ());
  if (enum_type == NULL || enum_type->code () != TYPE_CODE_ENUM)
    return false;

  /* Check if all required constructor names exist in the enum.  */
  for (int i = 0; i < count; i++)
    {
      bool found = false;
      for (int j = 0; j < enum_type->num_fields (); j++)
	{
	  const char *field_name = enum_type->field (j).name ();
	  if (field_name != NULL && strcmp (field_name, constructor_names[i]) == 0)
	    {
	      found = true;
	      break;
	    }
	}
      if (!found)
	return false;
    }

  return true;
}

/* Check if a type represents an OCaml list.

   DWARF-based detection (primary): Lists have variant_part with constructors
   named "[]" (empty list) and "::" (cons). This is more reliable than name
   matching as it directly checks the type structure.

   Heuristic fallback: If DWARF information is unavailable or incomplete, falls
   back to checking if the type name contains " list". This may produce false
   positives for non-list types with "list" in their names.

   Examples: "char list", "int list @ value", "string list", etc.  */

static bool
ocaml_is_list_type (struct type *type)
{
  if (type == NULL)
    return false;

  /* DWARF-based detection: Check for variant constructors "[]" and "::".  */
  const char *list_constructors[] = {"[]", "::"};
  if (ocaml_variant_has_constructors (type, list_constructors, 2))
    return true;

  /* HEURISTIC FALLBACK: Check type name for " list" pattern.
     This is less reliable but works when DWARF is incomplete.  */
  const char *type_name = type->name ();
  if (type_name == NULL)
    return false;

  /* Check if type name contains " list" (may be followed by " @ value").
     We use strstr to find the pattern anywhere in the type name.  */
  const char *list_pos = strstr (type_name, " list");
  if (list_pos == NULL)
    return false;

  /* Make sure it's followed by either end-of-string or " @".  */
  const char *after = list_pos + strlen (" list");
  return (*after == '\0' || strncmp (after, " @", 2) == 0);
}

static bool
ocaml_is_reference_type (struct type *type)
{
  if (type == NULL)
    return false;

  /* References must be struct types with exactly one field.  */
  if (type->code () != TYPE_CODE_STRUCT)
    return false;

  if (type->num_fields () != 1)
    return false;

  /* The field must be named "contents".
     This is sufficient to identify references, as this structure is unique to refs.  */
  const char *field_name = type->field (0).name ();
  if (field_name == NULL)
    return false;

  return (strcmp (field_name, "contents") == 0);
}

/* Print an OCaml reference value using DWARF type information.

   References are OCaml's mutable containers: type 'a ref = { mutable contents : 'a }
   With DWARF info, we print them in LLDB format: [value] instead of {contents = value}

   Example:
   Before: {contents = 42}
   After:  [42]

   Returns true if successfully printed, false if no reference info available.  */

static bool
ocaml_print_reference_with_type (struct value *val, struct type *type,
				  struct ui_file *stream, int recurse,
				  const struct value_print_options *options)
{
  struct gdbarch *gdbarch = type->arch ();
  LONGEST val_raw;

  /* Read the "contents" field value.
     For struct types, we need to read the field using GDB's value API,
     not extract raw bytes.  */
  if (type->code () == TYPE_CODE_STRUCT && type->num_fields () >= 1)
    {
      /* Read the first field ("contents") which contains the OCaml value.  */
      struct value *field_val = value_field (val, 0);
      val_raw = value_as_long (field_val);
    }
  else
    {
      /* For non-struct types, read the value directly.  */
      val_raw = value_as_long (val);
    }

  /* Print in LLDB format: [value]
     The contents can be any OCaml value - immediate (int, bool, etc.) or block (string, record, etc.)  */
  gdb_puts ("[", stream);

  /* Print the OCaml value stored in the contents field.  */
  ocaml_print_value (gdbarch, val_raw, stream, recurse + 1, options);

  gdb_puts ("]", stream);
  return true;
}

/* Get the module-qualified type name for an OCaml type.

   OCaml types in DWARF often include module qualification and metadata suffixes:
   - "String.t @ value" -> should display as "String.t"
   - "'a list @ value" -> should display as "'a list"
   - "Module.Submodule.typename @ value" -> "Module.Submodule.typename"

   This function extracts the clean, module-qualified typename by removing
   the "@ value" suffix and other OCaml-specific metadata.

   Returns a newly allocated string containing the clean name, or NULL
   if the type has no name. The caller is responsible for freeing the result.  */

static gdb::unique_xmalloc_ptr<char>
ocaml_get_qualified_type_name (struct type *type)
{
  if (type == NULL)
    return NULL;

  const char *raw_name = type->name ();
  if (raw_name == NULL)
    return NULL;

  /* Find the "@ value" or "@ " suffix that OCaml compilers add.  */
  const char *at_sign = strstr (raw_name, " @ ");

  if (at_sign != NULL)
    {
      /* Extract the part before " @ ".  */
      size_t len = at_sign - raw_name;
      char *clean_name = (char *) xmalloc (len + 1);
      strncpy (clean_name, raw_name, len);
      clean_name[len] = '\0';
      return gdb::unique_xmalloc_ptr<char> (clean_name);
    }

  /* No suffix found, return the name as-is.  */
  return make_unique_xstrdup (raw_name);
}

/* Get the representation part of an OCaml type annotation.

   OCaml DWARF types include a representation suffix like "@ value", "@ float64", "@ bits32".
   This function extracts just the representation part after "@".

   Examples:
   - "int @ value" -> "value"
   - "float @ float64" -> "float64"
   - "int32 @ bits32" -> "bits32"

   Returns a newly allocated string containing the representation, or "value" as default.
   The caller is responsible for freeing the result.  */

static gdb::unique_xmalloc_ptr<char>
ocaml_get_type_representation (struct type *type)
{
  if (type == NULL)
    return make_unique_xstrdup ("value");

  const char *raw_name = type->name ();
  if (raw_name == NULL)
    return make_unique_xstrdup ("value");

  /* Find the "@ " suffix that indicates representation.  */
  const char *at_sign = strstr (raw_name, " @ ");

  if (at_sign != NULL)
    {
      /* Extract the part after "@ ".  */
      const char *representation = at_sign + 3; /* Skip " @ " */
      return make_unique_xstrdup (representation);
    }

  /* No representation found, default to "value".  */
  return make_unique_xstrdup ("value");
}

/* Check if a representation string indicates an unboxed type.

   Unboxed types in OCaml use special representations that store values directly
   rather than using the standard boxed value representation:
   - float64: unboxed 64-bit float (float#)
   - float32: unboxed 32-bit float (float32#)
   - bits32: unboxed 32-bit integer (int32#)
   - bits64: unboxed 64-bit integer (int64#)
   - word: unboxed native integer (nativeint#)
   - bits8, bits16: unboxed 8/16-bit integers

   Unboxed values are displayed with a # prefix (e.g., #4.1, #42l).  */

static bool
ocaml_is_unboxed_representation (const char *representation)
{
  if (representation == NULL)
    return false;

  return (strcmp (representation, "float64") == 0 ||
	  strcmp (representation, "float32") == 0 ||
	  strcmp (representation, "bits32") == 0 ||
	  strcmp (representation, "bits64") == 0 ||
	  strcmp (representation, "word") == 0 ||
	  strcmp (representation, "bits8") == 0 ||
	  strcmp (representation, "bits16") == 0);
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
  if (parts == NULL || parts->empty ())
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

  if (parts == NULL || parts->empty ())
    return false;

  const variant_part &part = (*parts)[0];
  if (part.variants.size () != 1)
    return false;

  const variant &var = part.variants[0];

  /* Get the constructor name.  */
  const char *constructor_name = ocaml_get_constructor_name (type, var);
  if (constructor_name == NULL)
    constructor_name = "<unboxed>";

  /* Print the constructor name.  */
  gdb_puts (constructor_name, stream);

  /* If the variant has a field, print the value directly.
     For unboxed variants, the value is NOT in a block - it's stored directly.  */
  if (var.first_field < var.last_field && var.first_field < type->num_fields ())
    {
      gdb_puts (" ", stream);

      /* The value is stored directly - read the raw value.  */
      LONGEST raw_val;
      if (type->code () == TYPE_CODE_STRUCT)
	{
	  const gdb_byte *contents = val->contents ().data ();
	  int ptr_size = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
	  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
	  raw_val = extract_unsigned_integer (contents, ptr_size, byte_order);
	}
      else
	{
	  raw_val = value_as_long (val);
	}

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
  /* Get the variant parts from the type.  */
  const gdb::array_view<variant_part> *parts = ocaml_get_variant_parts (type);
  if (parts == NULL || parts->empty ())
    {
      return false;
    }

  const variant_part &part = (*parts)[0];
  struct gdbarch *gdbarch = type->arch ();

  /* Read the OCaml runtime discriminant (block tag or immediate value).  */
  LONGEST discr = ocaml_read_discriminant_from_value (gdbarch, val, part);
  if (discr < 0)
    {
      gdb_printf (stream, "<invalid variant>");
      return true;
    }

  /* Check if this is an unboxed variant (discriminant in bit 0).
     Handle it separately since data is packed in the same value.  */
  bool is_unboxed = ocaml_is_unboxed_variant (type, part);
  if (is_unboxed)
    {
      /* Read the full 64-bit value.  */
      const gdb_byte *contents = val->contents ().data ();
      int ptr_size = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      ULONGEST packed_value = extract_unsigned_integer (contents, ptr_size, byte_order);

      /* Find the matching variant based on discriminant.  */
      const variant *var = ocaml_find_matching_variant (part, discr);
      if (var == NULL || var->first_field < 0 || var->first_field >= type->num_fields ())
	{
	  gdb_printf (stream, "<invalid variant>");
	  return true;
	}

      /* Get the data type from the variant's first field.  */
      struct type *data_type = check_typedef (type->field (var->first_field).type ());

      /* For unboxed variants, OCaml stores the data value AS-IS in the 64-bit word.
	 The discriminant (bit 0) is part of the value's natural representation.
	 For floats, bit 0 is the LSB of the mantissa, so the entire value is the float.
	 NO SHIFTING NEEDED!  */
      ULONGEST data_bits = packed_value;

      /* Get constructor name from enum.  */
      const char *constructor_name = "<unknown>";
      if (part.discriminant_index >= 0 && part.discriminant_index < type->num_fields ())
	{
	  struct type *discr_type = check_typedef (type->field (part.discriminant_index).type ());
	  const char *name = ocaml_find_enum_name_by_value (discr_type, discr);
	  if (name != NULL)
	    constructor_name = name;
	}

      /* Print the constructor name and unboxed data.  */
      gdb_puts ("(", stream);
      gdb_puts (constructor_name, stream);
      gdb_puts (" ", stream);

      /* Reinterpret data_bits as the proper type and print with # prefix.  */
      if (data_type->code () == TYPE_CODE_FLT)
	{
	  /* Handle float types.  */
	  if (data_type->length () == 8)
	    {
	      /* float64: reinterpret bits 1-63 as double.  */
	      double float_val;
	      /* Reconstruct full 64 bits for IEEE 754 double.  */
	      memcpy (&float_val, &data_bits, sizeof (double));
	      ocaml_print_float (float_val, true, stream);
	    }
	  else if (data_type->length () == 4)
	    {
	      /* float32: extract 32 bits and reinterpret.  */
	      uint32_t float32_bits = (uint32_t)(data_bits & 0xFFFFFFFF);
	      float float_val;
	      memcpy (&float_val, &float32_bits, sizeof (float));
	      ocaml_print_float ((double)float_val, true, stream);
	    }
	}
      else if (data_type->code () == TYPE_CODE_INT)
	{
	  /* Handle integer types.  */
	  const char *type_name = data_type->name ();

	  /* Check if this is ocaml_value type - could be immediate int or block pointer.  */
	  if (type_name != NULL && strcmp (type_name, "ocaml_value") == 0)
	    {
	      /* ocaml_value can be:
		 1. Immediate int (LSB=1): decode with right shift
		 2. Block pointer (LSB=0): dereference and print contents  */

	      if (ocaml_is_immediate_int (data_bits))
		{
		  /* OCaml immediate value - decode by right shifting.
		     This handles `ValueInt of int [@@unboxed]` where the int
		     is stored as an OCaml immediate (value << 1 | 1).  */
		  LONGEST int_val = ocaml_immediate_int_val (data_bits);
		  gdb_printf (stream, "%s", plongest (int_val));
		}
	      else if (ocaml_is_block (data_bits))
		{
		  /* Block pointer - use runtime type inspection to print the value.
		     This handles `ValueString of string [@@unboxed]`,
		     `ValueBool of bool [@@unboxed]`, etc.

		     We use ocaml_print_value() instead of ocaml_value_print_inner()
		     because the latter expects DWARF type information, but here we
		     only have an ocaml_value typed pointer which doesn't tell us
		     what kind of block it points to. ocaml_print_value() does
		     runtime type inspection by reading the block header.  */
		  ocaml_print_value (gdbarch, data_bits, stream, recurse + 1,
				     options, NULL);
		}
	      else
		{
		  /* Neither immediate nor block - print raw value.  */
		  gdb_printf (stream, "%s", plongest (data_bits));
		}
	    }
	  else if (data_type->length () == 4)
	    {
	      /* int32/bits32: extract 32 bits.  */
	      int32_t int_val = (int32_t)(data_bits & 0xFFFFFFFF);
	      gdb_printf (stream, "#%ldl", (long)int_val);
	    }
	  else if (data_type->length () == 8)
	    {
	      /* int64/bits64: use all 63 bits.  */
	      int64_t int_val = (int64_t)data_bits;
	      gdb_printf (stream, "#%ldL", (long)int_val);
	    }
	  else if (data_type->length () == 8 && data_type->is_unsigned ())
	    {
	      /* nativeint: platform-specific int (64-bit on x86_64).  */
	      gdb_printf (stream, "#%ldn", (long)data_bits);
	    }
	}
      else if (data_type->code () == TYPE_CODE_ENUM)
	{
	  /* Handle enum types (e.g., bool encoded as enum {false = 1, true = 3}).
	     Look up the enum value in the type's fields to find the name.  */
	  const char *enum_name = ocaml_find_enum_name_by_value (data_type, data_bits);

	  if (enum_name != NULL)
	    {
	      /* Found the enum value name - print it directly (no # prefix for bools).  */
	      gdb_puts (enum_name, stream);
	    }
	  else
	    {
	      /* Enum value not found - print raw value.  */
	      gdb_printf (stream, "#%ld", (long)data_bits);
	    }
	}
      else
	{
	  /* Unknown unboxed type - print raw bits.  */
	  gdb_printf (stream, "#0x%lx", (unsigned long)data_bits);
	}

      gdb_puts (")", stream);
      return true;
    }

  /* OCaml variants are encoded in two possible ways:

     1. No-arg constructors (immediate values or constant blocks):
        - Encoded as immediate integers or blocks with tag
        - Constructor names in parent enum field [1]

     2. Constructors with data (blocks with fields):
        - OCaml block with tag indicating constructor
        - Constructor names in nested enum (field [2] -> field [0])
        - Data fields in nested struct (field [2] -> fields [1], [2], ...)

     Strategy:
     1. Read OCaml runtime discriminant (tag)
     2. Find constructor name by matching tag to enum value
     3. If data exists, dereference and print it  */

  const char *constructor_name = NULL;
  struct type *enum_type = NULL;

  /* Check if this value is a block (pointer) or immediate.
     This determines which enum to use for constructor name lookup.  */
  LONGEST val_raw;
  const gdb_byte *contents = val->contents ().data ();
  int ptr_size = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  val_raw = extract_unsigned_integer (contents, ptr_size, byte_order);
  bool is_block_value = ocaml_is_block (val_raw);

  /* Find the reference field (field [2] in the parent struct).  */
  int ref_field = -1;
  for (int i = 0; i < type->num_fields (); ++i)
    {
      const field &f = type->field (i);
      struct type *field_type = check_typedef (f.type ());
      if (ocaml_type_is_pointer (field_type))
	{
	  ref_field = i;
	  break;
	}
    }

  /* Try to get enum from referenced struct.
     Both immediate and block values of the same variant type share the same
     constructor enum, which is located in the referenced struct's discriminant.
     For nested variants, this is critical because the typedef's top-level enums
     might be incomplete (empty), but the referenced struct has the full enum.  */
  if (is_block_value && ref_field >= 0)
    {
      /* Get the struct that the reference points to.  */
      struct type *ref_type = check_typedef (type->field (ref_field).type ());
      struct type *target_type = check_typedef (ref_type->target_type ());

      if (target_type->code () == TYPE_CODE_STRUCT && target_type->num_fields () > 0)
	{
	  /* For nested variants, the enum with constructor names is at the discriminant
	     of the referenced struct's variant_part. Use ocaml_get_variant_parts() to
	     handle both resolved (PROP_TYPE) and unresolved (PROP_VARIANT_PARTS) types.  */
	  const gdb::array_view<variant_part> *nested_parts = ocaml_get_variant_parts (target_type);
	  if (nested_parts != NULL && !nested_parts->empty ())
	    {
	      const variant_part &nested_part = (*nested_parts)[0];
	      if (nested_part.discriminant_index >= 0 &&
		  nested_part.discriminant_index < target_type->num_fields ())
		{
		  struct type *discr_type = check_typedef (target_type->field (nested_part.discriminant_index).type ());
		  if (discr_type->code () == TYPE_CODE_ENUM)
		    enum_type = discr_type;
		}
	    }

	  /* Fallback to field[0] if no variant_part found. */
	  if (enum_type == NULL)
	    {
	      struct type *field0_type = check_typedef (target_type->field (0).type ());
	      if (field0_type->code () == TYPE_CODE_ENUM)
		enum_type = field0_type;
	    }
	}
    }

  /* For immediate values or if no nested enum found, use parent enum.  */
  if (enum_type == NULL)
    {
      int max_bitsize = 0;
      int constructor_enum_field = -1;
      int polymorphic_variant_enum_field = -1;

      for (int i = 0; i < type->num_fields (); ++i)
	{
	  const field &f = type->field (i);
	  struct type *field_type = check_typedef (f.type ());
	  if (field_type->code () == TYPE_CODE_ENUM)
	    {
	      int bitsize = f.bitsize ();

	      /* Check if this is a polymorphic variant enum by looking for:
	         1. Enum field name starting with backtick
	         2. Large enum values (hash values > 1000) */
	      bool is_poly_variant = false;
	      if (field_type->num_fields () > 0)
		{
		  const char *first_name = field_type->field (0).name ();
		  LONGEST first_value = field_type->field (0).loc_enumval ();

		  if (first_name && first_name[0] == '`')
		    is_poly_variant = true;
		  else if (first_value > 1000)
		    is_poly_variant = true;
		}

	      /* Skip the discriminant enum (Pointer/Immediate) - we want the constructor enum.
	         The discriminant enum has values "Pointer" and "Immediate" which are internal.  */
	      bool is_discriminant_enum = false;
	      if (field_type->num_fields () > 0)
		{
		  const char *first_name = field_type->field (0).name ();
		  if (first_name && (strcmp (first_name, "Pointer") == 0 || strcmp (first_name, "Immediate") == 0))
		    is_discriminant_enum = true;
		}

	      if (is_poly_variant)
		{
		  polymorphic_variant_enum_field = i;
		}
	      else if (!is_discriminant_enum && bitsize > max_bitsize)
		{
		  max_bitsize = bitsize;
		  constructor_enum_field = i;
		}
	    }
	}

      /* Prefer polymorphic variant enum if found, otherwise use regular variant enum */
      if (polymorphic_variant_enum_field >= 0)
	{
	  enum_type = check_typedef (type->field (polymorphic_variant_enum_field).type ());
	}
      else if (constructor_enum_field >= 0)
	{
	  enum_type = check_typedef (type->field (constructor_enum_field).type ());
	}
    }

  /* Track if we used the Pointer branch fallback for immediate variants.
     This is needed to distinguish between:
     - Simple variants (A | B | C) where all are immediates: use standard matching
     - Nested variants (Leaf of int | Node) where only some can be immediate: use fallback  */
  bool used_pointer_branch_fallback = false;

  /* FALLBACK for nested immediate variants with empty constructor enum:
     OxCaml generates DWARF where the Immediate branch has an empty enum,
     but the Pointer branch has the full constructor enum. For immediate nested
     variants, we need to access the Pointer branch enum to get constructor names.

     For immediate values, ref_field=-1 because the resolved type only shows the
     Immediate branch. We need to look in the original unresolved type (or search
     all fields) to find the Pointer branch's reference field. */
  if (!is_block_value && (enum_type == NULL || enum_type->num_fields () == 0))
    {
      /* For resolved types (immediate branch), we need to access the original unresolved
         type which has all variant branches including the Pointer branch with ref field.  */
      struct type *search_type = type;
      dynamic_prop *variant_prop = type->dyn_prop (DYN_PROP_VARIANT_PARTS);
      if (variant_prop != NULL && variant_prop->kind () == PROP_TYPE)
	{
	  struct type *orig = variant_prop->original_type ();
	  if (orig != NULL)
	    {
	      search_type = orig;
	    }
	}

      /* Search for reference field in the (possibly original) type */
      int search_ref_field = ref_field;
      if (search_ref_field < 0)
	{
	  for (int i = 0; i < search_type->num_fields (); ++i)
	    {
	      struct type *field_type = check_typedef (search_type->field (i).type ());
	      if (ocaml_type_is_pointer (field_type))
		{
		  search_ref_field = i;
		  break;
		}
	    }
	}

      if (search_ref_field >= 0 && search_ref_field < search_type->num_fields ())
	{
	  /* Navigate to Pointer branch: reference field → target struct → variant_part → discriminant */
	  struct type *ref_type = check_typedef (search_type->field (search_ref_field).type ());
	  struct type *target_type = check_typedef (ref_type->target_type ());

	  if (target_type->code () == TYPE_CODE_STRUCT && target_type->num_fields () > 0)
	    {
	      const gdb::array_view<variant_part> *target_parts = ocaml_get_variant_parts (target_type);
	      if (target_parts != NULL && !target_parts->empty ())
		{
		  const variant_part &target_part = (*target_parts)[0];
		  if (target_part.discriminant_index >= 0 &&
		      target_part.discriminant_index < target_type->num_fields ())
		    {
		      struct type *discr_type = check_typedef (target_type->field (target_part.discriminant_index).type ());
		      if (discr_type->code () == TYPE_CODE_ENUM && discr_type->num_fields () > 0)
			{
			  enum_type = discr_type;
			  /* Also update ref_field for later use in immediate matching */
			  ref_field = search_ref_field;
			  /* Mark that we used the Pointer branch fallback */
			  used_pointer_branch_fallback = true;
			}
		    }
		}
	    }
	}
    }

  /* Check if this is a polymorphic variant enum */
  bool is_poly_variant_enum = false;
  if (enum_type != NULL && enum_type->num_fields () > 0)
    {
      const char *first_name = enum_type->field (0).name ();
      if (first_name && first_name[0] == '`')
	is_poly_variant_enum = true;
    }

  /* For polymorphic variant blocks, the discriminant (block tag) is not meaningful.
     Instead, field[0] of the block contains the hash. Read it if this is a block. */
  char poly_hash_name[64] = {0};
  if (is_poly_variant_enum && is_block_value && constructor_name == NULL)
    {
      CORE_ADDR block_addr = (CORE_ADDR) val_raw;
      LONGEST field0_val;
      if (ocaml_read_block_field (gdbarch, block_addr, 0, &field0_val))
	{
	  /* field0_val is the hash as an immediate value (raw with LSB=1).
	     Try to match it against enum values. */
	  const char *hash_name = ocaml_find_enum_name_by_value (enum_type, field0_val);
	  if (hash_name != NULL)
	    constructor_name = hash_name;

	  /* If no match found in DWARF, format hash for display.
	     NOTE: OxCaml's DWARF may not include all poly variant constructors in the enum,
	     so we fall back to showing the hash value. */
	  if (constructor_name == NULL)
	    {
	      snprintf (poly_hash_name, sizeof(poly_hash_name), "`#0x%lx", (unsigned long)field0_val);
	      constructor_name = poly_hash_name;
	    }
	}
    }

  /* Look up constructor name from enum using discriminant (for non-poly-variant or immediates).  */
  if (constructor_name == NULL && enum_type != NULL)
    {
      /* For immediate nested variants with Pointer branch enum (from fallback above),
         the discriminant is the immediate value, but enum values are block tags.
         In this case, only single-argument constructors can be immediate, so we match
         to the first (lowest-tag) constructor in the enum. */
      if (!is_block_value && used_pointer_branch_fallback && !is_poly_variant_enum &&
	  enum_type->num_fields () > 0)
	{
	  /* Find constructor with smallest enum value (likely the simple constructor) */
	  LONGEST min_tag = enum_type->field (0).loc_enumval ();
	  const char *min_name = enum_type->field (0).name ();
	  for (int i = 1; i < enum_type->num_fields (); ++i)
	    {
	      LONGEST tag = enum_type->field (i).loc_enumval ();
	      if (tag < min_tag)
		{
		  min_tag = tag;
		  min_name = enum_type->field (i).name ();
		}
	    }
	  constructor_name = min_name;
	}
      else
	{
	  /* Standard constructor lookup by discriminant matching */
	  for (int i = 0; i < enum_type->num_fields (); ++i)
	    {
	      LONGEST enum_value = enum_type->field (i).loc_enumval ();

	      /* For polymorphic variants, the enum value stores the raw immediate value
		 (hash with LSB=1 tag), but the discriminant has been unshifted (>>1).
		 We need to reconstruct the raw immediate from the discriminant to match. */
	      bool match = false;
	      if (is_poly_variant_enum)
		{
		  /* Poly variant: reconstruct raw immediate from unshifted discriminant */
		  LONGEST raw_discr = (discr << 1) | 1;
		  match = (enum_value == raw_discr);
		}
	      else
		{
		  /* Regular variant: direct comparison */
		  match = (enum_value == discr);
		}

	      if (match)
		{
		  constructor_name = enum_type->field (i).name ();
		  break;
		}
	    }
	}
    }

  if (constructor_name == NULL || constructor_name[0] == '\0')
    constructor_name = "<unknown>";

  /* Check if this constructor has data (block pointer).
     Note: val_raw and is_block_value already computed above.  */
  if (is_block_value && ref_field >= 0)
    {
      /* This is a constructor with data.
         Read block fields directly from OCaml memory, but try to extract
         proper DWARF field types for correct OCaml value printing.  */
      CORE_ADDR block_addr = (CORE_ADDR) val_raw;
      ULONGEST header;

      /* Try to extract field types from DWARF for proper value printing.
         The target_type contains the variant data fields (starting at field 1,
         since field 0 is the discriminant enum).  */
      struct type *target_type = NULL;
      if (ref_field >= 0)
	{
	  struct type *ref_type = check_typedef (type->field (ref_field).type ());
	  target_type = check_typedef (ref_type->target_type ());
	}

      if (ocaml_read_block_header (gdbarch, block_addr, &header))
	{
	  ULONGEST num_fields = ocaml_header_size (header);

	  /* Check if DWARF shows a single complex field (like a record) but
	     OCaml block has many fields. This happens when a variant constructor
	     has a record as its only argument - DWARF shows 1 field (the record),
	     but the record is flattened in the OCaml block.

	     Example: type t = Mixed of {a:int; b:float; c:bool}
	     DWARF: field[0]=discriminant, field[1]=record with 3 fields
	     OCaml: block with 3 fields (a, b, c flattened)

	     In this case, print the block as a single record instead of
	     multiple simple values.  */
	  if (target_type != NULL && target_type->code () == TYPE_CODE_STRUCT &&
	      num_fields > 1)
	    {
	      /* Count non-discriminant data fields in DWARF.
	         Field[0] is discriminant enum, field[1+] are data fields.  */
	      int dwarf_data_fields = 0;
	      for (int i = 1; i < target_type->num_fields (); ++i)
		{
		  if (target_type->field (i).name () != NULL)
		    dwarf_data_fields++;
		}

	      /* If DWARF shows exactly 1 data field but OCaml block has many,
	         the single field is a complex type (record) that was flattened.  */
	      if (dwarf_data_fields == 1)
		{
		  /* Get the record type from DWARF field[1].  */
		  struct type *record_type = check_typedef (target_type->field (1).type ());

		  /* Verify it's actually a record (struct without variant parts).  */
		  if (record_type->code () == TYPE_CODE_STRUCT &&
		      !ocaml_is_variant_struct (record_type) &&
		      record_type->num_fields () > 0)
		    {
		      /* Print as: (Constructor {field=value; ...})  */
		      gdb_puts ("(", stream);
		      gdb_puts (constructor_name, stream);
		      gdb_puts (" ", stream);

		      /* Create a value for the record at the block address.
		         The OCaml block IS the flattened record.  */
		      struct value *record_val = value_at (record_type, block_addr);
		      ocaml_print_record_with_type (record_val, record_type, stream,
						    recurse + 1, options);

		      gdb_puts (")", stream);
		      return true;  /* Successfully printed complex variant.  */
		    }
		}
	    }

	  gdb_puts ("(", stream);
	  gdb_puts (constructor_name, stream);

	  /* For polymorphic variant blocks, field[0] contains the hash, not data.
	     Skip it when printing. */
	  ULONGEST start_field = is_poly_variant_enum ? 1 : 0;

	  /* Print each field from the OCaml block.
	     Respect print_max to avoid buffer overflows with large structures.  */
	  ULONGEST print_limit = (options->print_max < num_fields) ? options->print_max : num_fields;


	  /* Check if the constructor fields should be printed as a record.
	     Use DWARF variant_parts to find which fields are active for this discriminant,
	     then check if those fields have names (indicating an inline record).  */
	  bool print_as_record = false;
	  const variant *active_var = NULL;

	  if (target_type != NULL && target_type->code () == TYPE_CODE_STRUCT && num_fields > 1)
	    {
	      /* Get the variant_parts from target_type to find active fields.
	         Note: target_type is the struct containing the variant's data fields.  */
	      const gdb::array_view<variant_part> *type_parts = ocaml_get_variant_parts (target_type);
	      if (type_parts != NULL && !type_parts->empty ())
		{
		  const variant_part &type_part = (*type_parts)[0];
		  active_var = ocaml_find_matching_variant (type_part, discr);

		  if (active_var != NULL)
		    {
		      /* Check if the active variant has multiple named fields (inline record) */
		      print_as_record = ocaml_variant_has_inline_record (target_type, *active_var);

		      /* Limit field printing to the active variant's field range.
		         The active variant owns fields [first_field, last_field), so
		         we should only print (last_field - first_field) fields from the block.  */
		      if (print_as_record)
			{
			  ULONGEST variant_field_count = active_var->last_field - active_var->first_field;
			  print_limit = (options->print_max < variant_field_count)
			    ? options->print_max : variant_field_count;
			}
		    }
		}
	    }

	  if (print_as_record)
	    gdb_puts (" {", stream);

	  for (ULONGEST i = start_field; i < print_limit; ++i)
	    {
	      LONGEST field_val;
	      if (ocaml_read_block_field (gdbarch, block_addr, i, &field_val))
		{

		  /* Create a value from the raw OCaml field and print it.
		     Try to use the proper DWARF field type if available (field index i+1
		     since field 0 is the discriminant enum).  */
		  struct type *field_type = NULL;
		  bool use_typedef_type = false;

		  if (target_type != NULL && target_type->code () == TYPE_CODE_STRUCT)
		    {
		      /* Map OCaml field index to DWARF field index.
		         If we have active variant info, use its field range.
		         Otherwise fall back to sequential mapping (i+1).  */
		      int dwarf_field_index;
		      if (active_var != NULL)
			{
			  /* Active variant: OCaml field i maps to first_field + i */
			  dwarf_field_index = active_var->first_field + (i - start_field);
			}
		      else
			{
			  /* Fallback: sequential mapping */
			  dwarf_field_index = i + 1;
			}

		      /* Print field separator and name for record-style variants */
		      if (print_as_record && dwarf_field_index < target_type->num_fields ())
			{
			  const char *field_name = target_type->field (dwarf_field_index).name ();
			  if (field_name != NULL && field_name[0] != '\0')
			    {
			      if (i > start_field)
				gdb_puts ("; ", stream);
			      gdb_puts (field_name, stream);
			      gdb_puts (" = ", stream);
			    }
			}
		      else if (!print_as_record)
			{
			  gdb_puts (" ", stream);
			}

		      if (dwarf_field_index < target_type->num_fields ())
			{
			  field_type = target_type->field (dwarf_field_index).type ();

			  /* Check if this is a complex type (typedef to struct/variant).
			     For typedef types like "char list @ value", preserve the typedef
			     so recursive printing can identify the type correctly.  */
			  struct type *resolved_type = check_typedef (field_type);
			  if (resolved_type->code () == TYPE_CODE_STRUCT &&
			      field_type->code () == TYPE_CODE_TYPEDEF)
			    {
			      /* Use value_at() to create a typed value at the address.
			         This preserves typedef wrappers for recursive types.  */
			      use_typedef_type = true;
			    }
			}
		    }

		      /* Special handling for unboxed float fields in variant inline records.
		         Unboxed floats are stored as raw float bits in the OCaml block,
		         not as OCaml values. DWARF identifies these as TYPE_CODE_FLT.  */
		      if (field_type != NULL && field_type->code () == TYPE_CODE_FLT)
			{
			  double float_val;

			  if (field_type->length () == 8)
			    {
			      /* 64-bit float (double). field_val contains raw float bits.  */
			      memcpy (&float_val, &field_val, sizeof (double));
			    }
			  else if (field_type->length () == 4)
			    {
			      /* 32-bit float.  */
			      float float32_val;
			      uint32_t bits32 = (uint32_t) field_val;
			      memcpy (&float32_val, &bits32, sizeof (float));
			      float_val = float32_val;
			    }
			  else
			    {
			      /* Unexpected float size - skip special handling.  */
			      goto regular_field_printing;
			    }

			  /* Print unboxed float with # prefix and proper .0 suffix.  */
			  ocaml_print_float (float_val, true, stream);
			  continue;  /* Skip regular field value printing.  */
			}

		    regular_field_printing:

		  /* Check for empty list BEFORE falling back to pointer type.
		     This must be done while we still have the original DWARF field type.  */
		  if (!ocaml_is_block (field_val) && field_type != NULL)
		    {
		      const char *type_name = field_type->name ();

		      /* If this is a typedef without a name, try resolving it or checking target.  */
		      if (type_name == NULL && field_type->code () == TYPE_CODE_TYPEDEF)
			{
			  struct type *resolved = check_typedef (field_type);
			  if (resolved != NULL)
			    type_name = resolved->name ();

			  /* Also check target_type.  */
			  if (type_name == NULL)
			    {
			      struct type *target = field_type->target_type ();
			      if (target != NULL)
				type_name = target->name ();
			    }
			}

		      if (type_name != NULL && ocaml_is_list_type (field_type))
			{
			  gdb_puts ("[]", stream);
			  continue;
			}

		      /* HEURISTIC: If we have a typedef (without name) that resolves to a struct,
		         and the value is immediate, it's likely an empty constructor like [] or None.
		         Since we can't determine the type from DWARF (OxCaml doesn't include field
		         type names), we print [] as a best guess for recursive list types.  */
		      if (type_name == NULL && field_type->code () == TYPE_CODE_TYPEDEF)
			{
			  struct type *resolved = check_typedef (field_type);
			  if (resolved != NULL && resolved->code () == TYPE_CODE_STRUCT)
			    {
			      /* This is likely an empty list or option. Print [] as a guess.  */
			      gdb_puts ("[]", stream);
			      continue;
			    }
			}
		    }

		  /* Fall back to pointer type if no field type found.  */
		  if (field_type == NULL)
		    field_type = builtin_type (gdbarch)->builtin_data_ptr;

		  struct value *field_value;

		  if (use_typedef_type && ocaml_is_block (field_val))
		    {
		      /* For typedef-wrapped complex types (that are blocks/pointers),
		         create value at address. This preserves type information for
		         recursive printing.  */
		      field_value = value_at (field_type, (CORE_ADDR) field_val);
		    }
		  else if (use_typedef_type)
		    {
		      /* For other typedef-wrapped complex types that are immediates,
		         fall back to pointer type.  */
		      struct type *ptr_type = builtin_type (gdbarch)->builtin_data_ptr;
		      field_value = value_from_longest (ptr_type, field_val);
		    }
		  else
		    {
		      /* For simple types, use value_from_longest with the field type.  */
		      field_value = value_from_longest (field_type, field_val);
		    }

		  /* Print the field value recursively.
		     Empty lists will be detected and handled at the top level of
		     ocaml_value_print_inner() when printing list types.

		     Check if this field is a record (struct without variant parts).
		     If so, print it with proper {field = value} format instead of
		     space-separated values.  */
		  /* Print the field value recursively.
		     Empty lists will be detected and handled at the top level of
		     ocaml_value_print_inner() when printing list types.

		     Check if this field is a record (struct without variant parts).
		     For typedef-wrapped types, check the target type directly before
		     resolving, as check_typedef() may lose variant_parts information
		     for recursive types.  */
		  struct type *field_type_orig = field_value->type ();
		  struct type *ftype_resolved = check_typedef (field_type_orig);

		  /* For typedefs, check if the target has variant_parts before resolution */
		  bool is_variant = false;
		  if (field_type_orig->code () == TYPE_CODE_TYPEDEF)
		    {
	      struct type *target = field_type_orig->target_type ();
	      /* Follow the typedef chain, checking each level for variant_parts.
		 variant_parts might be on any typedef in the chain, not just the final type. */
	      while (target != NULL && !is_variant)
		{
		  /* Check if this level has variant_parts */
		  is_variant = ocaml_is_variant_struct (target);

		  if (target->code () == TYPE_CODE_TYPEDEF)
		    target = target->target_type ();
		  else
		    break;  /* Reached non-typedef, stop */
		}
		    }

		  if (!is_variant)
		    is_variant = ocaml_is_variant_struct (ftype_resolved);

		  if (ftype_resolved->code () == TYPE_CODE_STRUCT &&
		      !is_variant &&
		      ftype_resolved->num_fields () > 0)
		    {
		      /* Check if this looks like a record by verifying all fields are named.
			 If any fields are unnamed or if the struct has an enum field
			 (discriminant), it might be a variant that we failed to detect. */
		      bool looks_like_record = true;
		      for (int j = 0; j < ftype_resolved->num_fields (); j++)
			{
			  const char *fname = ftype_resolved->field (j).name ();
			  struct type *ftype = ftype_resolved->field (j).type ();
			  if (fname == NULL || fname[0] == '\0')
			    {
			      looks_like_record = false;
			      break;
			    }
			  /* If there's an enum field, it might be a discriminant */
			  if (ftype != NULL && check_typedef (ftype)->code () == TYPE_CODE_ENUM)
			    {
			      looks_like_record = false;
			      break;
			    }
			}

		      if (looks_like_record)
			{
			  /* This is a record - print with field names.  */
			  ocaml_print_record_with_type (field_value, ftype_resolved, stream,
						      recurse + 1, options);
			}
		      else
			{
			  /* Pass DWARF field_type to enable type-aware variant detection.
			     Even though we create a simple pointer value, the DWARF type
			     parameter carries typedef→struct with variant_parts information. */
			  struct type *ptr_type = builtin_type (gdbarch)->builtin_data_ptr;
			  struct value *simple_value = value_from_longest (ptr_type, field_val);
			  ocaml_value_print_inner (simple_value, stream, recurse + 1, options, field_type);
			}
		    }
		  else
		    {
		      /* Not a record (or is a variant) - pass field's DWARF type.  */
		      ocaml_value_print_inner (field_value, stream, recurse + 1, options, field_value->type ());
		    }
		}
	    }

	  /* Close record-style variant fields */
	  if (print_as_record)
	    gdb_puts ("}", stream);

	  /* If we didn't print all fields, show that there's more.  */
	  if (print_limit < num_fields)
	    gdb_puts (" ...", stream);

	  gdb_puts (")", stream);
	}
      else
	{
	  /* Failed to read block header - just print constructor name.  */
	  gdb_puts (constructor_name, stream);
	}
    }
  else
    {
      /* Immediate variant (not a block pointer).
         For regular variants with data, the immediate encodes the data value.
         For no-arg constructors, it's just a constant.  */

      if (strcmp (constructor_name, "<unknown>") == 0 && ocaml_is_immediate_int (val_raw))
	{
	  /* Constructor name not found (empty enum), but we have an immediate value.
	     This happens for nested variants where DWARF doesn't provide full type info.
	     Decode the immediate value and print it as data.

	     FALLBACK: Without constructor names, we show the decoded immediate value.
	     For `Leaf 1`, this will print as `1` instead of `(Leaf 1)`.  */
	  LONGEST data_val = ocaml_immediate_int_val (val_raw);
	  gdb_printf (stream, "(%s)", plongest (data_val));
	}
      else
	{
	  /* Check if this is a constructor with data (not a constant constructor).
	     For immediate variants like `Leaf of int`, we need to print both the
	     constructor name and the integer value: (Leaf 1)

	     Only print data if we used the Pointer branch fallback, which indicates
	     this is a mixed-constructor variant where immediate values have data.
	     For simple variants (A | B | C), all are immediates with no data. */
	  if (used_pointer_branch_fallback && strcmp (constructor_name, "<unknown>") != 0)
	    {
	      /* This is a constructor with data - extract and print the integer value */
	      LONGEST data_val = ocaml_immediate_int_val (val_raw);
	      gdb_printf (stream, "(%s %s)", constructor_name, plongest (data_val));
	    }
	  else
	    {
	      /* No-arg constructor - just print the name.  */
	      gdb_puts (constructor_name, stream);
	    }
	}
    }

  return true;
}

/* Forward declaration - defined later.  */
static bool
ocaml_print_with_type (struct value *val, struct ui_file *stream, int recurse,
		       const struct value_print_options *options,
		       struct type *dwarf_type = NULL);

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
  /* Unboxed records must have exactly one field.  */
  if (type->num_fields () != 1)
    return false;

  const field &f = type->field (0);
  const char *field_name = f.name ();

  /* Print as a record with the single field.  */
  gdb_puts ("{", stream);

  if (field_name != NULL && field_name[0] != '\0')
    gdb_printf (stream, "%s = ", field_name);

  /* Get the field value with DWARF type information preserved.
     This is critical for nested records/structs to print correctly.  */
  struct value *field_val = value_field (val, 0);
  struct type *field_type = check_typedef (field_val->type ());

  /* If the field is a reference, dereference it.  */
  if (field_type->code () == TYPE_CODE_REF)
    {
      field_val = coerce_ref (field_val);
      field_type = check_typedef (field_val->type ());
    }

  /* If it's a struct with DWARF type info, try DWARF-based printing first.
     This preserves proper OCaml semantics (semicolons, nested records, etc.).  */
  if (field_type->code () == TYPE_CODE_STRUCT)
    {
      if (!ocaml_print_with_type (field_val, stream, recurse + 1, options))
	{
	  /* DWARF printing failed - fall back to common_val_print.  */
	  common_val_print (field_val, stream, recurse + 1, options, current_language);
	}
    }
  else
    {
      /* For non-struct fields, use common_val_print.  */
      common_val_print (field_val, stream, recurse + 1, options, current_language);
    }

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
  /* The value is already a struct with fields - print them directly.  */
  gdb_puts ("{", stream);

  int num_fields = type->num_fields ();
  bool first = true;
  int fields_printed = 0;

  /* Limit number of fields printed to avoid buffer overflows.  */
  int print_limit = (options->print_max < (unsigned int)num_fields) ? options->print_max : num_fields;

  for (int i = 0; i < num_fields; i++)
    {
      const field &f = type->field (i);
      const char *field_name = f.name ();

      /* Skip fields without names (shouldn't happen for records).  */
      if (field_name == NULL || field_name[0] == '\0')
	continue;

      /* Stop printing if we've reached the limit.  */
      if (fields_printed >= print_limit)
	{
	  gdb_puts ("; ...", stream);
	  break;
	}

      if (!first)
	{
	  gdb_puts ("; ", stream);
	}
      first = false;
      fields_printed++;

      gdb_printf (stream, "%s = ", field_name);

      /* Get field value and type from DWARF record definition.
	 Use DWARF field type, not value's type, to preserve variant_parts in typedefs.
	 The value might have a different type (e.g., pointer) which loses type information.  */
      struct value *field_val = value_field (val, i);
      struct type *field_type = type->field (i).type ();  /* From DWARF, not value */

      /* If the field is a reference, dereference it but keep the DWARF type.  */
      if (field_type->code () == TYPE_CODE_REF)
	{
	  field_val = coerce_ref (field_val);
	  /* Keep using DWARF field_type, don't switch to value's type */
	}

      /* Special handling for unboxed tuple fields (e.g., f.#0, f.#1).
         These are flattened unboxed tuple elements that lost their type annotations in DWARF.
         Heuristic: if field name matches *.#\d+ and type is pointer without typedef,
         use the field byte size to determine the suffix (like LLDB does).  */
      bool is_unboxed_tuple_field = false;

      /* Check if field name matches pattern: *.#\d+ (e.g., f.#0, f.#1).  */
      const char *dot_hash = strstr (field_name, ".#");
      if (dot_hash != NULL && field_type->code () == TYPE_CODE_INT)
	{
	  /* Verify the part after .# is a digit.  */
	  const char *p = dot_hash + 2;
	  if (*p >= '0' && *p <= '9')
	    {
	      /* Check if this is ocaml_value type (regular int) or truly unboxed int64#.
	         HEURISTIC FALLBACK: DWARF doesn't distinguish int from int64# reliably.
	         - If type name is "ocaml_value": it's a regular int (no # prefix)
	         - Otherwise: assume it's unboxed int64# (needs # prefix and suffix) */
	      const char *type_name = field_type->name ();

	      if (type_name != NULL && strcmp (type_name, "ocaml_value") == 0)
		{
		  /* Regular OCaml value field - pass DWARF type for nested type resolution.  */
		  ocaml_value_print_inner (field_val, stream, recurse + 1, options, field_type);
		  is_unboxed_tuple_field = true;  /* Mark as handled */
		}
	      else
		{
		  /* Unboxed int32# or int64# field.  */
		  is_unboxed_tuple_field = true;
		  /* Print as unboxed integer with # prefix (like unboxed floats).
		     Use byte size to determine suffix:
		     4 bytes = int32# (suffix "l"), 8 bytes = int64# (suffix "L").  */
		  LONGEST raw_val = value_as_long (field_val);
		  const char *suffix = "";
		  if (field_type->length () == 4)
		    suffix = "l";
		  else if (field_type->length () == 8)
		    suffix = "L";

		  if (raw_val < 0)
		    gdb_printf (stream, "-#%s%s", pulongest (-raw_val), suffix);
		  else
		    gdb_printf (stream, "#%s%s", pulongest (raw_val), suffix);
		}
	    }
	}

      if (!is_unboxed_tuple_field)
	{
	  /* Special handling for unboxed float fields in records.
	     Float fields stored inline (not as pointers) are unboxed and need # prefix.
	     DWARF shows these as base_type with float encoding, no typedef or name.  */
	  if (field_type->code () == TYPE_CODE_FLT)
	    {
	      /* Extract float value from field contents.  */
	      const gdb_byte *valaddr = field_val->contents ().data ();
	      double float_val;

	      if (field_type->length () == 8)
		{
		  /* 64-bit float (double).  */
		  memcpy (&float_val, valaddr, sizeof (double));
		}
	      else if (field_type->length () == 4)
		{
		  /* 32-bit float.  */
		  float float32_val;
		  memcpy (&float32_val, valaddr, sizeof (float));
		  float_val = float32_val;
		}
	      else
		{
		  /* Unexpected float size - fall back to regular printing with DWARF type.  */
		  ocaml_value_print_inner (field_val, stream, recurse + 1, options, field_type);
		  continue;
		}

	      /* Print unboxed float with # prefix and proper .0 suffix for whole numbers.  */
	      ocaml_print_float (float_val, true, stream);
	    }
	  /* If it's a struct with DWARF type info, try DWARF-based printing first.
	     This preserves proper OCaml semantics (semicolons, nested records, etc.).

	     For variant-typed fields, we need to ensure they print as (Constructor data)
	     instead of raw field values. Check if this field is a variant and call
	     the variant printer directly if so.  */
	  else if (field_type->code () == TYPE_CODE_STRUCT)
	    {
	      /* Check if this struct field is actually a variant type.  */
	      if (ocaml_is_variant_struct (field_type))
		{
		  /* This is a variant - print with constructor format.  */
		  if (!ocaml_print_variant_with_type (field_val, field_type, stream,
						      recurse + 1, options))
		    {
		      /* Variant printing failed - fall back.  */
		      common_val_print (field_val, stream, recurse + 1, options,
				       current_language);
		    }
		}
	      else if (!ocaml_print_with_type (field_val, stream, recurse + 1, options))
		{
		  /* DWARF printing failed - fall back to common_val_print.  */
		  common_val_print (field_val, stream, recurse + 1, options, current_language);
		}
	    }
	  else
	    {
	      /* Regular field: pass DWARF type for nested type resolution.  */
	      ocaml_value_print_inner (field_val, stream, recurse + 1, options, field_type);
	    }
	}
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
  LONGEST val_raw;

  /* Arrays are passed as pointers in OCaml. The value itself contains
     the pointer to the OCaml block. We need to read this pointer value
     directly from the value contents.

     Check for reference types BEFORE calling check_typedef(), since
     check_typedef() will strip the typedef and lose the reference type.  */
  struct type *typedef_resolved = check_typedef (type);

  /* For typedef'd reference types (e.g., "char array @ value" which is
     typedef to enum (&)[0]), read the address being referenced.
     The value might be in a register, so use value_as_address().  */
  if (typedef_resolved->code () == TYPE_CODE_REF)
    {
      val_raw = value_as_address (val);
    }
  else if (typedef_resolved->code () == TYPE_CODE_STRUCT)
    {
      /* For struct types, extract the pointer from the first field.  */
      const gdb_byte *contents = val->contents ().data ();
      int ptr_size = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      val_raw = extract_unsigned_integer (contents, ptr_size, byte_order);
    }
  else
    {
      /* For integer/pointer types, use value_as_long.  */
      val_raw = value_as_long (val);
    }

  /* Arrays must be blocks.  */
  if (!ocaml_is_block (val_raw))
    return false;

  CORE_ADDR addr = (CORE_ADDR) val_raw;
  ULONGEST header;

  if (!ocaml_read_block_header (gdbarch, addr, &header))
    return false;

  ULONGEST size = ocaml_header_size (header);

  /* Extract the array element type from DWARF type information.
     Type hierarchy: typedef → reference → array → element_type
     This allows array elements to print as proper OCaml values (e.g., 'a' instead of 97).  */
  struct type *elem_type = NULL;

  if (typedef_resolved->code () == TYPE_CODE_REF)
    {
      struct type *array_type = typedef_resolved->target_type ();
      if (array_type != NULL)
	{
	  array_type = check_typedef (array_type);
	  if (array_type->code () == TYPE_CODE_ARRAY)
	    elem_type = array_type->target_type ();
	}
    }

  /* Print array elements.  */
  if (size == 0)
    {
      gdb_puts ("[||]", stream);
      return true;
    }

  gdb_puts ("[|", stream);

  /* Limit array elements using options->print_max to respect user settings.  */
  ULONGEST print_limit = (options->print_max < size) ? options->print_max : size;

  for (ULONGEST i = 0; i < print_limit; i++)
    {
      if (i > 0)
	gdb_puts ("; ", stream);

      LONGEST field_val;
      if (ocaml_read_block_field (gdbarch, addr, i, &field_val))
	{
	  /* Create a value with the proper element type for correct OCaml printing.
	     If we successfully extracted the element type from DWARF, use it.
	     Otherwise fall back to generic pointer type.  */
	  struct type *value_type = (elem_type != NULL)
	    ? elem_type
	    : builtin_type (gdbarch)->builtin_data_ptr;

	  struct value *elem_value = value_from_longest (value_type, field_val);
	  /* Pass elem_type for DWARF-aware printing of array elements (e.g., nested variants, lists).  */
	  ocaml_value_print_inner (elem_value, stream, recurse + 1, options, elem_type);
	}
      else
	gdb_puts ("<error>", stream);
    }

  if (print_limit < size)
    gdb_puts (" ...", stream);

  gdb_puts ("|]", stream);
  return true;
}

/* Print an OCaml tuple value using DWARF type information.

   Tuples are printed as [elem0, elem1, elem2, ...] to match LLDB format.
   With DWARF info, we can distinguish tuples from arrays and records.

   Returns true if successfully printed, false if not a tuple.  */

static bool
ocaml_print_tuple_with_type (struct value *val, struct type *type,
			      struct ui_file *stream, int recurse,
			      const struct value_print_options *options)
{
  struct gdbarch *gdbarch = type->arch ();
  LONGEST val_raw;

  /* Read the raw value.  */
  if (type->code () == TYPE_CODE_STRUCT)
    {
      const gdb_byte *contents = val->contents ().data ();
      int ptr_size = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      val_raw = extract_unsigned_integer (contents, ptr_size, byte_order);
    }
  else
    {
      val_raw = value_as_long (val);
    }

  /* Tuples must be blocks.  */
  if (!ocaml_is_block (val_raw))
    return false;

  CORE_ADDR addr = (CORE_ADDR) val_raw;
  int num_fields = type->num_fields ();

  if (num_fields == 0)
    {
      gdb_puts ("[]", stream);
      return true;
    }

  gdb_puts ("[", stream);

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

  gdb_puts ("]", stream);
  return true;
}

/* Dispatch value printing based on DWARF type information.

   This function checks if DWARF type information is available and dispatches
   to the appropriate type-aware printing function.

   Returns true if the value was printed using type information, false otherwise.  */

static bool
ocaml_print_with_type (struct value *val, struct ui_file *stream, int recurse,
		       const struct value_print_options *options,
		       struct type *dwarf_type)
{
  /* Prioritize DWARF type information over value's type.
     This is essential for nested variants where the value has pointer type
     but DWARF type contains typedef→struct with variant_parts.  */
  struct type *type = (dwarf_type != NULL) ? dwarf_type : val->type ();

  if (type == NULL)
    {
      return false;
    }

  /* Check for array types BEFORE resolving typedefs, since the array type name
     (e.g., "char array") is in the typedef and would be lost after check_typedef().
     OCaml arrays are represented as typedef'd enums in DWARF.  */
  if (ocaml_is_array_type (type))
    {
      return ocaml_print_array_with_type (val, type, stream, recurse, options);
    }

  /* Resolve typedefs to get the actual type.  */
  type = check_typedef (type);

  /* Also check TYPE_CODE_ARRAY after typedef resolution, in case of native arrays.  */
  if (type->code () == TYPE_CODE_ARRAY)
    {
      return ocaml_print_array_with_type (val, type, stream, recurse, options);
    }

  /* Check for unboxed variant types FIRST.
     Unboxed variants need special handling since the value is stored
     directly without OCaml's block wrapper.  */
  if (ocaml_is_unboxed_variant (type))
    {
      return ocaml_print_unboxed_variant (val, type, stream, recurse, options);
    }

  /* Check for regular variant types (sum types with variant_part).
     This also handles OCaml exceptions, which are represented identically
     to variants in DWARF (both use DW_TAG_variant_part).
     Exception types (exn) will have variant_part information and will be
     printed with their constructor names automatically.  */
  const gdb::array_view<variant_part> *vparts = ocaml_get_variant_parts (type);
  if (vparts != NULL)
    {
      return ocaml_print_variant_with_type (val, type, stream, recurse, options);
    }

  /* Check for reference types BEFORE unboxed records.
     References are technically records with a single "contents" field,
     but we want to print them in LLDB format: [value] instead of {contents = value}.
     Must check before unboxed records since references also have 1 field.  */
  if (ocaml_is_reference_type (type))
    {
      return ocaml_print_reference_with_type (val, type, stream, recurse, options);
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
   Tuples are printed as [field0, field1, field2, ...] to match LLDB format.  */

static void
ocaml_print_tuple (struct gdbarch *gdbarch, CORE_ADDR addr, ULONGEST size,
		   struct ui_file *stream, int recurse,
		   const struct value_print_options *options)
{
  if (size == 0)
    {
      gdb_puts ("[]", stream);
      return;
    }

  gdb_puts ("[", stream);

  /* Respect print_max limit for tuple elements.  */
  ULONGEST print_size = size;
  bool truncated = false;
  if (size > options->print_max)
    {
      print_size = options->print_max;
      truncated = true;
    }

  for (ULONGEST i = 0; i < print_size; i++)
    {
      if (i > 0)
	gdb_puts (", ", stream);

      LONGEST field_val;
      if (ocaml_read_block_field (gdbarch, addr, i, &field_val))
	ocaml_print_value (gdbarch, field_val, stream, recurse + 1, options);
      else
	gdb_puts ("<error>", stream);
    }

  if (truncated)
    gdb_puts (", ...", stream);

  gdb_puts ("]", stream);
}

/* Print an OCaml array.
   Arrays are printed as [|elem0; elem1; elem2; ...|].

   Array vs Tuple Disambiguation:
   - OCaml arrays and tuples both use tag 0 blocks
   - We now use DWARF type information to distinguish them:
     * Arrays: TYPE_CODE_ARRAY after typedef resolution (implemented)
     * Lists: variant_part with constructors [] and :: (implemented)
     * Tuples: TYPE_CODE_STRUCT without variant_parts
     * Records: TYPE_CODE_STRUCT with named fields
   - Tag 0 blocks are printed as tuples unless identified as arrays or lists */

/* Print an OCaml list in LLDB format: (:: (head, tail)).
   Lists use tag 0 blocks with 2 fields: head and tail.
   Empty list is represented as [].

   List Detection (DWARF-first):
   - Primary method: Check DWARF variant_part for constructors [] and ::
   - This reliably identifies list types at compile time
   - Fallback: Runtime pattern matching (tag 0, size 2, tail is [] or block)
   - The fallback may incorrectly identify other ADTs as lists:
     * ('a * 'b) tuples where second element is [] or a block
     * Binary trees or other recursive structures with similar layout
     * Some variant constructors with 2 fields
   - For accurate detection, ensure DWARF debug info is available */

static void
ocaml_print_list (struct gdbarch *gdbarch, LONGEST list_val,
		  struct ui_file *stream, int recurse,
		  const struct value_print_options *options,
		  unsigned int *elements_printed)
{
  /* Check for empty list.  */
  if (list_val == OCAML_VAL_EMPTY_LIST)
    {
      gdb_puts ("[]", stream);
      return;
    }

  /* Respect print_max limit for list elements.  */
  if (elements_printed != NULL
      && *elements_printed >= options->print_max)
    {
      gdb_puts ("...", stream);
      return;
    }

  /* Verify this is a cons cell (tag 0, size 2).  */
  if (!ocaml_is_block (list_val))
    {
      /* Not a block, shouldn't happen for non-empty lists.  */
      gdb_printf (stream, "<not-a-list: %s>", plongest (list_val));
      return;
    }

  CORE_ADDR addr = (CORE_ADDR) list_val;
  ULONGEST header;

  if (!ocaml_read_block_header (gdbarch, addr, &header))
    {
      gdb_puts ("<list-error>", stream);
      return;
    }

  int tag = ocaml_header_tag (header);
  ULONGEST size = ocaml_header_size (header);

  /* List cons cells have tag 0 and size 2 (head, tail).  */
  if (tag != 0 || size != 2)
    {
      gdb_printf (stream, "<not-cons: tag=%d size=%s>", tag, pulongest (size));
      return;
    }

  /* Print in LLDB format: (:: (head, tail)) */
  gdb_puts ("(:: (", stream);

  /* Print the head element.  */
  LONGEST head;
  if (!ocaml_read_block_field (gdbarch, addr, 0, &head))
    {
      gdb_puts ("<error>", stream);
      return;
    }

  ocaml_print_value (gdbarch, head, stream, recurse + 1, options);

  /* Increment element count after printing head.  */
  if (elements_printed != NULL)
    (*elements_printed)++;

  gdb_puts (", ", stream);

  /* Print the tail element (recursively if it's another cons).  */
  LONGEST tail;
  if (!ocaml_read_block_field (gdbarch, addr, 1, &tail))
    {
      gdb_puts ("<error>", stream);
      return;
    }

  ocaml_print_list (gdbarch, tail, stream, recurse + 1, options,
		    elements_printed);

  gdb_puts ("))", stream);
}

/* Print a single OCaml value given its raw representation.
   This is the core recursive printing function.

   Note: Type information is not available at this level, so we use heuristics.
   For accurate type-aware printing, use ocaml_value_print_inner() which has
   access to DWARF type information.

   IMPORTANT: If dwarf_type is provided (non-null), it MUST be used to interpret
   immediate values correctly. OCaml's runtime representation causes ambiguity:
   - false = 1 (OCaml encoding)
   - 0 (int) = 1 (OCaml encoding)
   Without DWARF type info, we cannot distinguish them. When dwarf_type is available,
   we check its TYPE_CODE to determine if the value should be printed as bool or int.  */

static void
ocaml_print_value (struct gdbarch *gdbarch, LONGEST val_raw,
		   struct ui_file *stream, int recurse,
		   const struct value_print_options *options,
		   struct type *dwarf_type)
{
  /* Prevent excessive recursion using user's max_depth setting.  */
  if (options->max_depth > 0 && recurse >= options->max_depth)
    {
      gdb_puts ("...", stream);
      return;
    }

  /* CRITICAL: When DWARF type information is available, use it to interpret
     immediate values correctly. OCaml's runtime encoding causes ambiguity where
     multiple distinct values share the same runtime representation:
     - false = 1, but 0 (int) = 1
     - true = 3, but 1 (int) = 3
     Without type info, we cannot distinguish them.

     HEURISTIC FALLBACK: When dwarf_type is NULL, we fall back to heuristic
     interpretation (checking for bool values first). This may produce incorrect
     output for integer values that coincide with bool encodings.  */

  /* Check if we have DWARF type information to guide interpretation.  */
  if (dwarf_type != NULL)
    {
      struct type *base_type = check_typedef (dwarf_type);

      /* If DWARF says this is a bool, check for true/false values.  */
      if (base_type->code () == TYPE_CODE_BOOL)
	{
	  if (val_raw == OCAML_VAL_TRUE)
	    {
	      gdb_puts ("true", stream);
	      return;
	    }
	  if (val_raw == OCAML_VAL_FALSE)
	    {
	      gdb_puts ("false", stream);
	      return;
	    }
	}

      /* If DWARF says this is an int, print as integer even if value matches bool encoding.  */
      if (base_type->code () == TYPE_CODE_INT && ocaml_is_immediate_int (val_raw))
	{
	  LONGEST int_val = ocaml_immediate_int_val (val_raw);
	  gdb_printf (stream, "%s", plongest (int_val));
	  return;
	}

      /* For other types with DWARF info, continue to normal processing below.  */
    }
  else
    {
      /* HEURISTIC FALLBACK: No DWARF type information available.
	 Check for special immediate values using heuristics.
	 WARNING: This may misinterpret int 0 as false, int 1 as true, etc.  */

      if (val_raw == OCAML_VAL_TRUE)
	{
	  gdb_puts ("true", stream);
	  return;
	}

      if (val_raw == OCAML_VAL_FALSE)
	{
	  gdb_puts ("false", stream);
	  return;
	}
    }

  /* Check for special non-ambiguous immediate values.  */
  if (val_raw == OCAML_VAL_UNIT)
    {
      gdb_puts ("()", stream);
      return;
    }

  if (val_raw == OCAML_VAL_EMPTY_LIST)
    {
      gdb_puts ("[]", stream);
      return;
    }

  /* Check if this is an immediate integer.  */
  if (ocaml_is_immediate_int (val_raw))
    {
      LONGEST int_val = ocaml_immediate_int_val (val_raw);

      /* For characters (values 0-255 in typical usage), we could print as 'c',
	 but without type information we can't distinguish char from int.
	 Print as integer for now.  */
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
	      ocaml_print_float (d, false, stream);
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
	     DWARF-based detection (primary): Check if DWARF type information
	     indicates this is a list type (has constructors "[]" and "::").
	     This is more reliable than runtime pattern matching.  */
	  if (dwarf_type != NULL && ocaml_is_list_type (dwarf_type))
	    {
	      /* DWARF confirms this is a list type. Print as list.  */
	      unsigned int elements_printed = 0;
	      ocaml_print_list (gdbarch, val_raw, stream, recurse, options,
				&elements_printed);
	      return;
	    }

	  /* HEURISTIC FALLBACK: Try to detect lists using runtime pattern.
	     WARNING: This may misidentify tuples or other ADTs as lists.
	     For accurate detection, ensure DWARF debug info is available.  */
	  if (size == 2)
	    {
	      /* Could be a list cons cell. Let's check if it looks like one
		 by trying to traverse it.  */
	      LONGEST tail;
	      if (ocaml_read_block_field (gdbarch, addr, 1, &tail)
		  && (tail == OCAML_VAL_EMPTY_LIST || ocaml_is_block (tail)))
		{
		  /* Likely a list. Print as list with element counting.  */
		  unsigned int elements_printed = 0;
		  ocaml_print_list (gdbarch, val_raw, stream, recurse, options,
				    &elements_printed);
		  return;
		}
	    }

	  /* Otherwise, print as tuple or generic block.  */
	  ocaml_print_tuple (gdbarch, addr, size, stream, recurse, options);
	  return;
	}
      else if (tag == OCAML_TAG_CLOSURE)
	{
	  /* Closures: Functions with captured environment.
	     Display as <closure>@ADDRESS to match LLDB format.
	     Future enhancement: Could show function name and captured variables if DWARF info available. */
	  gdb_printf (stream, "<closure>@0x%s", phex_nz (addr, sizeof (addr)));
	  return;
	}
      else if (tag == OCAML_TAG_OBJECT)
	{
	  /* Objects: OOP instances with methods and fields.
	     Display as <object>@ADDRESS to match LLDB format.
	     Future enhancement: Could parse class structure and print fields. */
	  gdb_printf (stream, "<object>@0x%s", phex_nz (addr, sizeof (addr)));
	  return;
	}
      else if (tag == OCAML_TAG_CUSTOM)
	{
	  /* Custom Block Interpretation
	     Custom blocks contain boxed primitive types (int32, int64, nativeint, float).
	     Layout:
	     - Field 0: pointer to custom operations structure
	     - Field 1: data (int32, int64, or float)

	     DWARF-based detection (primary): Check the DWARF type name to determine
	     if this is int32, int64, nativeint, or float. This is reliable.

	     HEURISTIC FALLBACK: Without DWARF type information, we cannot definitively
	     determine the custom block type. We use the block size to guess:
	     - Size 2 (1 ops + 1 data field): int32 (4 bytes) or float (8 bytes)
	     - Size 3 (1 ops + 2 data fields): int64 (8 bytes) packed as 2 words on 32-bit

	     For accurate type interpretation, ensure DWARF debug info is available.  */

	  /* Read field 1 (the data field) - needed for both DWARF and heuristic paths.  */
	  LONGEST data_field = 0;
	  ULONGEST block_size = ocaml_header_size (header);
	  bool has_data = false;

	  if (block_size >= 2 && ocaml_read_block_field (gdbarch, addr, 1, &data_field))
	    has_data = true;

	  /* DWARF-based detection: Check type name for int32, int64, nativeint.  */
	  if (dwarf_type != NULL && has_data)
	    {
	      const char *type_name = dwarf_type->name ();
	      if (type_name != NULL)
		{
		  if (strstr (type_name, "int32") != NULL)
		    {
		      gdb_printf (stream, "%ldl", (long)data_field);
		      return;
		    }
		  else if (strstr (type_name, "int64") != NULL)
		    {
		      gdb_printf (stream, "%ldL", (long)data_field);
		      return;
		    }
		  else if (strstr (type_name, "nativeint") != NULL)
		    {
		      gdb_printf (stream, "%ldn", (long)data_field);
		      return;
		    }
		}
	    }

	  /* HEURISTIC FALLBACK: Guess type based on value range.
	     WARNING: Cannot distinguish int32 from int64 when value fits in 32 bits.
	     For accurate detection, ensure DWARF debug info is available.  */
	  if (has_data)
	    {
	      /* On 64-bit architectures, int32 and int64 both fit in one word.
		 Check if this looks like int32 (value fits in 32 bits).  */
	      if (data_field >= INT32_MIN && data_field <= INT32_MAX)
		{
		  /* Likely int32 - print with 'l' suffix.  */
		  gdb_printf (stream, "%ldl", (long)data_field);
		  return;
		}
	      else
		{
		  /* Likely int64 or nativeint - print with 'L' suffix.  */
		  gdb_printf (stream, "%ldL", (long)data_field);
		  return;
		}
	    }

	  /* Fallback: unknown custom block type.  */
	  gdb_puts ("<custom>", stream);
	  return;
	}
      else if (tag == OCAML_TAG_LAZY)
	{
	  /* Lazy values (tag 246): Unevaluated lazy expression.
	     Field 0 contains a closure that will compute the value when forced.
	     We show <lazy> to indicate the value hasn't been evaluated yet. */
	  gdb_puts ("<lazy>", stream);
	  return;
	}
      else if (tag == OCAML_TAG_FORWARD)
	{
	  /* Forward tags (tag 250): Evaluated lazy value or GC forwarding pointer.
	     When a lazy value is forced, its tag changes from 246 to 250,
	     and field 0 points to the computed value.
	     We follow the pointer and print the actual value. */
	  LONGEST forwarded_val;
	  if (ocaml_read_block_field (gdbarch, addr, 0, &forwarded_val))
	    ocaml_print_value (gdbarch, forwarded_val, stream, recurse + 1, options);
	  else
	    gdb_puts ("<forward: error reading value>", stream);
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

/* Top-level value printing for OCaml.

   This overrides the default C-based `value_print` to avoid printing the
   type cast prefix `(typename)` for records and other struct types.

   OCaml's type annotations come at the end (`: TYPE @ REPRESENTATION`),
   not as a prefix cast, so we skip the C++ objectprint logic entirely.  */

static void
ocaml_value_print (struct value *val, struct ui_file *stream,
		   const struct value_print_options *options)
{
  /* Simply delegate to the inner printer at recursion level 0.
     The inner printer handles all OCaml-specific printing and type annotations.
     By not calling c_value_print, we avoid the (typename) prefix that C++ uses.  */
  struct value_print_options opts = *options;
  opts.deref_ref = true;

  ocaml_value_print_inner (val, stream, 0, &opts);
}

/* Implement la_value_print_inner for OCaml.

   OCaml uses a tagged value representation:
   - Immediate integers: LSB = 1, value = raw_value >> 1
   - Block pointers: LSB = 0, points to heap-allocated data
   - Special immediate values: (), true, false, [], None

   This function detects the value type and prints it appropriately,
   and appends type annotations in LLDB format: VALUE : TYPE @ REPRESENTATION.  */

void
ocaml_value_print_inner (struct value *val, struct ui_file *stream, int recurse,
			 const struct value_print_options *options,
			 struct type *dwarf_type)
{
  /* Check recursion depth limit to prevent buffer overflows and infinite loops
     when printing large or recursive structures (lists, trees, etc.).  */
  if (options->max_depth > 0 && recurse >= options->max_depth)
    {
      gdb_puts ("...", stream);
      return;
    }

  /* Save the original value type BEFORE any transformations for type annotations.
     This preserves typedef names which would be lost by check_typedef.  */
  struct type *original_val_type = val->type ();

  struct type *type = check_typedef (val->type ());
  struct gdbarch *gdbarch = type->arch ();

  /* If DWARF type information was provided, use it to check for variants.
     This is more reliable than runtime heuristics when typedef chains hide variant_parts. */
  struct type *dwarf_variant_type = NULL;
  if (dwarf_type != NULL)
    {
      /* Follow typedef chain to find variant_parts */
      struct type *current = dwarf_type;
      while (current != NULL)
        {
          if (current->code () == TYPE_CODE_TYPEDEF)
            {
              /* Check if this level or its target has variant_parts */
              if (ocaml_is_variant_struct (current))
                {
                  dwarf_variant_type = current;
                  break;
                }
              current = current->target_type ();
            }
          else if (current->code () == TYPE_CODE_STRUCT)
            {
              if (ocaml_is_variant_struct (current))
                dwarf_variant_type = current;
              break;
            }
          else
            break;
        }
    }


  /* Handle C++ reference types (&) by dereferencing them.
     OCaml function parameters are often passed as C++ references in DWARF.
     We need to preserve the type name from the reference type, as the underlying
     struct is often anonymous.  */
  struct type *original_type = type;
  if (type->code () == TYPE_CODE_REF)
    {
      val = coerce_ref (val);
      type = check_typedef (val->type ());
      /* If the dereferenced type has no name, use the original reference type's name.  */
      if (type->name () == NULL && original_type->name () != NULL)
	{
	  /* The underlying type is anonymous, but the reference type has the name.
	     We'll use original_type for name lookups but type for structure analysis.  */
	}
    }

  /* Handle exceptions with custom formatting before DWARF-based printing.

     KNOWN LIMITATION: Exception formatter is partially implemented.

     Target format: { exn = { name = "Failure"; id = -3 }; raw = [...] } : exn @ value
     Current output: { exn = <object>@0x30; raw = [...] } : exn @ value

     Issue: OxCaml encodes exception information differently than expected.
     The exn field (after dereferencing) contains small integer values like
     0x30 (48 decimal), which appear to be exception IDs in a global exception
     table rather than heap-allocated structures with name/id fields.

     The implementation successfully:
     - Detects exception types
     - Dereferences DWARF references (including unnamed fields)
     - Follows the reference to get the ocaml_value integer

     But fails to read fields because:
     - ocaml_read_block_field() expects a valid heap address
     - Exception IDs are small integers (not heap pointers)
     - Need to lookup exception info from runtime's global exception table

     TODO: Investigate OxCaml's exception encoding and implement proper
     exception ID → (name, id) lookup mechanism.  */
  struct type *typedef_type = val->type ();
  gdb::unique_xmalloc_ptr<char> type_name_for_exn = ocaml_get_qualified_type_name (typedef_type);

  if (type->code () == TYPE_CODE_STRUCT && type_name_for_exn != NULL &&
      (strcmp (type_name_for_exn.get (), "exn") == 0 ||
       strncmp (type_name_for_exn.get (), "exn ", 4) == 0))
    {
      /* Exception format: { exn = { name = "..."; id = N }; raw = [...] } : exn @ value
	 The exn field is a reference to a struct with name and id fields.
	 The raw field contains the exception argument(s).  */

      gdb_printf (stream, "{ exn = ");

      /* Try to access and print the exn field.  */
      if (type->num_fields () >= 1)
	{
	  struct value *exn_field_val = value_field (val, 0);
	  struct type *exn_field_type = check_typedef (exn_field_val->type ());

	  /* Dereference if it's a reference type to get the ocaml_value.  */
	  if (exn_field_type->code () == TYPE_CODE_REF)
	    {
	      exn_field_val = coerce_ref (exn_field_val);
	      exn_field_type = check_typedef (exn_field_val->type ());
	    }

	  /* The dereferenced value can be either a struct (with DWARF info) or
	     an ocaml_value integer that needs manual following.  */
	  if (exn_field_type->code () == TYPE_CODE_STRUCT)
	    {
	      /* Check if this is an OCaml reference (1 field, possibly unnamed or named "contents").  */
	      if (exn_field_type->num_fields () == 1)
		{
		  const char *field_name = exn_field_type->field (0).name ();
		  bool is_reference = (field_name == NULL ||
				       field_name[0] == '\0' ||
				       strcmp (field_name, "contents") == 0);
		  if (is_reference)
		    {
		      /* This is an OCaml reference - follow the field to get the
			 actual value (may be a struct or ocaml_value integer).  */
		      exn_field_val = value_field (exn_field_val, 0);
		      exn_field_type = check_typedef (exn_field_val->type ());
		    }
		}

	      /* Now check if we have the exception struct with name and id fields.  */
	      if (exn_field_type->num_fields () >= 2)
		{
		  /* We have the exception struct - print name and id directly.  */
		  gdb_printf (stream, "{ name = ");
		  struct value *name_val = value_field (exn_field_val, 0);
		  ocaml_value_print_inner (name_val, stream, recurse + 1, options);

		  gdb_printf (stream, "; id = ");
		  struct value *id_val = value_field (exn_field_val, 1);
		  ocaml_value_print_inner (id_val, stream, recurse + 1, options);

		  gdb_printf (stream, " }");
		  return;  /* Successfully printed, exit exception handler.  */
		}
	      /* If not enough fields, fall through to check if it's an INT type.  */
	    }

	  /* Check if we have an ocaml_value integer to follow.  */
	  if (exn_field_type->code () == TYPE_CODE_INT ||
	      exn_field_type->code () == TYPE_CODE_PTR)
	    {
	      /* We have an ocaml_value integer - follow it as an OCaml block pointer.  */
	      LONGEST exn_ptr = value_as_long (exn_field_val);

	      /* Check if it's a block pointer (LSB = 0).  */
	      if ((exn_ptr & 1) == 0)
		{
		  CORE_ADDR block_addr = (CORE_ADDR) exn_ptr;

		  /* Read the name and id fields from the OCaml block.
		     NOTE: This currently fails because OxCaml encodes exception
		     information differently (possibly as exception IDs in a global
		     table rather than heap-allocated structures). Further
		     investigation needed.  */
		  LONGEST name_val, id_val;
		  if (ocaml_read_block_field (gdbarch, block_addr, 0, &name_val) &&
		      ocaml_read_block_field (gdbarch, block_addr, 1, &id_val))
		    {
		      gdb_printf (stream, "{ name = ");
		      ocaml_print_value (gdbarch, name_val, stream, recurse + 1, options);

		      gdb_printf (stream, "; id = ");
		      ocaml_print_value (gdbarch, id_val, stream, recurse + 1, options);

		      gdb_printf (stream, " }");
		    }
		  else
		    {
		      /* Failed to read fields - print as object address.  */
		      gdb_printf (stream, "<object>@0x%s", phex_nz (block_addr, sizeof (CORE_ADDR)));
		    }
		}
	      else
		{
		  /* Not a block pointer - shouldn't happen for exceptions.  */
		  gdb_printf (stream, "<immediate value %s>", plongest (exn_ptr));
		}
	    }
	  else
	    {
	      /* Unexpected type - fallback to regular printing.  */
	      ocaml_value_print_inner (exn_field_val, stream, recurse + 1, options);
	    }
	}

      /* Print the raw field.  */
      if (type->num_fields () >= 2)
	{
	  gdb_printf (stream, "; raw = ");
	  struct value *raw_field_val = value_field (val, 1);
	  ocaml_value_print_inner (raw_field_val, stream, recurse + 1, options);
	}

      gdb_printf (stream, " }");

      /* Add type annotation.  */
      gdb::unique_xmalloc_ptr<char> representation = ocaml_get_type_representation (typedef_type);
      if (representation != NULL)
	gdb_printf (stream, " : %s @ %s", type_name_for_exn.get (), representation.get ());

      return;
    }

  /* Check for lists with empty value - print [] for immediate values in list types.
     This must be done before variant printing to catch empty list [] constructor.
     Use original_val_type to preserve typedef names.  */
  if (ocaml_is_list_type (original_val_type))
    {
      /* Read the raw value to check if it's immediate (empty list).  */
      const gdb_byte *contents = val->contents ().data ();
      int ptr_size = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      LONGEST val_raw = extract_unsigned_integer (contents, ptr_size, byte_order);

      /* If it's an immediate value (not a block), it's the empty list [].  */
      if (!ocaml_is_block (val_raw))
	{
	  gdb_puts ("[]", stream);

	  /* Add type annotation.  */
	  gdb::unique_xmalloc_ptr<char> type_name = ocaml_get_qualified_type_name (original_val_type);
	  gdb::unique_xmalloc_ptr<char> representation = ocaml_get_type_representation (original_val_type);
	  if (type_name != NULL && representation != NULL)
	    gdb_printf (stream, " : %s @ %s", type_name.get (), representation.get ());

	  return;
	}
      /* Otherwise it's a cons cell (::), fall through to variant printing.  */
    }

  /* Check for arrays BEFORE checking for structs, since arrays have typedef names
     like "char array @ value" that must be detected before typedef resolution.
     Use original_val_type to preserve typedef names lost by dereferencing.  */
  if (ocaml_is_array_type (original_val_type))
    {
      bool array_success = ocaml_print_array_with_type (val, original_val_type, stream, recurse, options);
      if (array_success)
	{
	  /* Append type annotation: : TYPE @ REPRESENTATION.  */
	  gdb::unique_xmalloc_ptr<char> type_name = ocaml_get_qualified_type_name (original_val_type);
	  gdb::unique_xmalloc_ptr<char> representation = ocaml_get_type_representation (original_val_type);

	  if (type_name != NULL && representation != NULL)
	    gdb_printf (stream, " : %s @ %s", type_name.get (), representation.get ());

	  return;
	}
    }

  /* Try DWARF-based printing first if type information is available.
     This handles variants, records, and other structured types.
     Prioritize dwarf_type parameter over value type for nested variants. */
  struct type *print_type = (dwarf_variant_type != NULL) ? dwarf_variant_type : type;
  if (print_type->code () == TYPE_CODE_STRUCT || TYPE_HAS_VARIANT_PARTS (print_type))
    {
      /* Pass print_type to ocaml_print_with_type() to enable DWARF-aware variant detection.
	 For nested variants, this carries typedef→struct with variant_parts information.  */
      bool dwarf_success = ocaml_print_with_type (val, stream, recurse, options, print_type);
      if (dwarf_success)
	{
	  /* Append type annotation: : TYPE @ REPRESENTATION
	     Use original_val_type to preserve typedef names.  */
	  gdb::unique_xmalloc_ptr<char> type_name = ocaml_get_qualified_type_name (original_val_type);
	  gdb::unique_xmalloc_ptr<char> representation = ocaml_get_type_representation (original_val_type);

	  if (type_name != NULL && representation != NULL)
	    gdb_printf (stream, " : %s @ %s", type_name.get (), representation.get ());

	  return;
	}
    }

  /* For integer and pointer types, check for unboxed representation first.  */
  if (type->code () == TYPE_CODE_INT || type->code () == TYPE_CODE_PTR)
    {
      /* Use typedef_type (already set to val->type() at function start) to get annotation.  */
      gdb::unique_xmalloc_ptr<char> representation = ocaml_get_type_representation (typedef_type);

      /* Check if this is an unboxed integer type.
         Both TYPE_CODE_INT and TYPE_CODE_PTR can be unboxed integers:
         - TYPE_CODE_INT: regular unboxed integers (int32#, nativeint#)
         - TYPE_CODE_PTR: unboxed int64# in record fields  */
      if (ocaml_is_unboxed_representation (representation.get ()))
	{
	  /* Print unboxed integer with # prefix.
	     Format: #42l for int32, #42L for int64, #42n for nativeint.  */
	  LONGEST int_val = value_as_long (val);

	  /* Determine suffix based on representation type.  */
	  const char *suffix = "";
	  if (strcmp (representation.get (), "bits32") == 0)
	    suffix = "l";
	  else if (strcmp (representation.get (), "bits64") == 0)
	    suffix = "L";
	  else if (strcmp (representation.get (), "word") == 0)
	    suffix = "n";

	  /* Print with # prefix and suffix.  */
	  if (int_val < 0)
	    gdb_printf (stream, "-#%s%s", pulongest (-int_val), suffix);
	  else
	    gdb_printf (stream, "#%s%s", pulongest (int_val), suffix);

	  /* Append type annotation (only for top-level values).  */
	  if (recurse == 0)
	    {
	      gdb::unique_xmalloc_ptr<char> type_name = ocaml_get_qualified_type_name (typedef_type);
	      if (type_name != NULL)
		gdb_printf (stream, " : %s @ %s", type_name.get (), representation.get ());
	    }

	  return;
	}

      /* Boxed OCaml value: extract and delegate to heuristic printer.
	 IMPORTANT: Pass the DWARF type information to enable correct interpretation
	 of immediate values (distinguishing int 0 from bool false, etc.).  */
      LONGEST raw_val = value_as_long (val);
      ocaml_print_value (gdbarch, raw_val, stream, recurse, options, type);

      /* Append type annotation (only for top-level values).  */
      if (recurse == 0)
	{
	  struct type *name_type = (type->name () != NULL) ? type : original_type;
	  gdb::unique_xmalloc_ptr<char> type_name = ocaml_get_qualified_type_name (name_type);

	  if (type_name != NULL)
	    gdb_printf (stream, " : %s @ %s", type_name.get (), representation.get ());
	}

      return;
    }

  /* Handle unboxed floats with # prefix.  */
  if (type->code () == TYPE_CODE_FLT)
    {
      /* Use typedef_type (already set to val->type() at function start) to get annotation.  */
      gdb::unique_xmalloc_ptr<char> representation = ocaml_get_type_representation (typedef_type);

      if (ocaml_is_unboxed_representation (representation.get ()))
	{
	  /* Print unboxed float with # prefix.
	     Format: #4.1 for positive, -#3.14 for negative.  */

	  /* Get the float value using C printing, then prepend #.  */
	  string_file tmp_stream;
	  c_value_print_inner (val, &tmp_stream, recurse, options);
	  std::string float_str = tmp_stream.string ();

	  /* Handle negative sign placement: move '-' before '#'.  */
	  if (!float_str.empty () && float_str[0] == '-')
	    {
	      /* Negative: print as -#value.  */
	      gdb_printf (stream, "-%s#%s",  "", float_str.c_str () + 1);
	    }
	  else
	    {
	      /* Positive: print as #value.  */
	      gdb_printf (stream, "#%s", float_str.c_str ());
	    }

	  /* Append type annotation.  */
	  gdb::unique_xmalloc_ptr<char> type_name = ocaml_get_qualified_type_name (typedef_type);
	  if (type_name != NULL)
	    gdb_printf (stream, " : %s @ %s", type_name.get (), representation.get ());

	  return;
	}
      else
	{
	  /* Boxed float or float without representation info - print normally with type annotation.  */
	  c_value_print_inner (val, stream, recurse, options);

	  gdb::unique_xmalloc_ptr<char> type_name = ocaml_get_qualified_type_name (typedef_type);
	  if (type_name != NULL)
	    gdb_printf (stream, " : %s @ %s", type_name.get (), representation.get ());

	  return;
	}
    }

  /* For other types (arrays, enums, structs, etc.), delegate to C printing
     and add type annotation.  */
  c_value_print_inner (val, stream, recurse, options);

  /* Add type annotation if available.
     Use val->type() to get typedef before check_typedef resolution.  */
  struct type *typedef_type_for_annotation = val->type ();
  gdb::unique_xmalloc_ptr<char> type_name_for_annotation = ocaml_get_qualified_type_name (typedef_type_for_annotation);
  gdb::unique_xmalloc_ptr<char> representation = ocaml_get_type_representation (typedef_type_for_annotation);

  if (type_name_for_annotation != NULL && representation != NULL)
    gdb_printf (stream, " : %s @ %s", type_name_for_annotation.get (), representation.get ());
}
