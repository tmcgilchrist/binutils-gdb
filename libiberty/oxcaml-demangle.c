/* Demangler for the OxCaml programming language */

/* This file exports one function: oxcaml_demangle. */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include "safe-ctype.h"

#include <sys/types.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <demangle.h>
#include "libiberty.h"

#ifndef ULONG_MAX
#define	ULONG_MAX	(~0UL)
#endif
#ifndef UINT_MAX
#define	UINT_MAX	(~0U)
#endif

/* Maximal length of a symbol */
#define SYMBOL_MAX (1024*1024)
#define ERROR (~((unsigned)0))

#define ENDONERROR() do { \
  free(outbuf);           \
  return NULL;            \
} while(0)

/* Decode the decimal integer at *pos in sym
   Require a non-empty integer to appear
   Leave *pos to the first byte after the integer */
static unsigned decode_decimal(const char *sym, size_t *pos) {
  unsigned res = 0;
  size_t p = *pos;
  while (sym[p] >= '0' && sym[p] <= '9') {
    if(res > SYMBOL_MAX)
      return ERROR;
    res = res * 10 + (sym[p] - '0');
    p++;
  }
  if(*pos == p)
    // No digit was found
    return ERROR;
  *pos = p;
  return res;
}

static unsigned decode_26(const char *sym, size_t *pos) {
  unsigned res = 0;
  size_t p = *pos;
  while (sym[p] >= 'A' && sym[p] <= 'Z') {
    if(res > SYMBOL_MAX)
      return ERROR;
    res = res * 26 + (sym[p] - 'A');
    p++;
  }
  if(*pos == p)
    // No digit was found
    return ERROR;
  *pos = p;
  return res;
}

static int is_hex(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

static int hex(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  else
    return c - 'a' + 10;
}

/* Decode unicode-escaped identifier (format: u<len><coded>_<raw>)
   Returns 0 on success, ERROR on error */
static unsigned decode_unicode_escaped(const char *sym, size_t *pos,
                                       char *outbuf, size_t *outpos) {
  size_t sympos = *pos;
  size_t out = *outpos;
  size_t codedpos, endpos;
  unsigned l, raw;
  char *tmp;

  /* Decode the length */
  l = decode_decimal(sym, &sympos);
  if (l == ERROR || l == 0)
    return ERROR;

  /* Unicode encoded: format is u<len><encoded>_<raw> */
  codedpos = sympos;
  endpos = sympos + l;

  /* Find the underscore separator */
  tmp = strchr(sym + sympos, '_');
  if (!tmp || (size_t)(tmp - sym) > endpos)
    return ERROR;

  sympos = (size_t)(tmp - sym + 1);

  /* Decode the encoded portion */
  while (sym[codedpos] != '_') {
    /* Read base-26 position */
    raw = decode_26(sym, &codedpos);
    if (raw == ERROR || sympos + raw > endpos)
      return ERROR;

    /* Copy raw characters */
    tmp = stpncpy(outbuf + out, sym + sympos, raw);
    sympos += raw;
    out += raw;
    if ((size_t)(tmp - outbuf) != out)
      return ERROR;

    /* Decode hex-encoded characters */
    while (is_hex(sym[codedpos])) {
      if (!is_hex(sym[codedpos + 1]))
        return ERROR;
      outbuf[out++] = hex(sym[codedpos]) << 4 | hex(sym[codedpos + 1]);
      codedpos += 2;
    }
  }

  /* Copy remaining raw portion */
  if (sympos < endpos) {
    tmp = stpncpy(outbuf + out, sym + sympos, endpos - sympos);
    out += endpos - sympos;
    sympos = endpos;
    if ((size_t)(tmp - outbuf) != out)
      return ERROR;
  }

  *pos = sympos;
  *outpos = out;
  return 0;
}

/* Decode identifier (either plain or unicode-escaped)
   Handles: <len><text> or u<len><coded>_<raw>
   Returns 0 on success, ERROR on error */
static unsigned decode_identifier(const char *sym, size_t *pos,
                                  char *outbuf, size_t *outpos) {
  size_t sympos = *pos;
  size_t out = *outpos;
  unsigned l;
  char *tmp;

  /* Check for unicode prefix */
  if (sym[sympos] == 'u') {
    sympos++;
    *pos = sympos;
    return decode_unicode_escaped(sym, pos, outbuf, outpos);
  }

  /* Plain identifier with length prefix */
  l = decode_decimal(sym, &sympos);
  if (l == ERROR || l == 0)
    return ERROR;

  tmp = stpncpy(outbuf + out, sym + sympos, l);
  sympos += l;
  out += l;
  if ((size_t)(tmp - outbuf) != out)
    return ERROR;

  *pos = sympos;
  *outpos = out;
  return 0;
}

/* Decode anonymous location (format: filename_line_col)
   Anonymous functions/modules are encoded as: fn(filename:line:col)
   Returns 0 on success, ERROR on error */
static unsigned decode_anonymous_location(const char *sym, size_t *pos,
                                          char *outbuf, size_t *outpos) {
  char tempbuf[SYMBOL_MAX];
  size_t temppos = 0;
  size_t out = *outpos;
  size_t first_underscore = 0, second_underscore = 0;
  int underscore_count = 0;
  size_t i;

  /* Decode identifier into temp buffer */
  if (decode_identifier(sym, pos, tempbuf, &temppos) == ERROR)
    return ERROR;

  /* Parse filename_line_col format by finding the last two underscores */
  for (i = temppos; i > 0; i--) {
    if (tempbuf[i - 1] == '_') {
      underscore_count++;
      if (underscore_count == 1)
        second_underscore = i - 1;
      else if (underscore_count == 2) {
        first_underscore = i - 1;
        break;
      }
    }
  }

  /* Output in format fn(filename:line:col) */
  if (underscore_count >= 2) {
    outbuf[out++] = 'f';
    outbuf[out++] = 'n';
    outbuf[out++] = '(';

    /* Copy filename */
    for (i = 0; i < first_underscore; i++)
      outbuf[out++] = tempbuf[i];

    outbuf[out++] = ':';

    /* Copy line number */
    for (i = first_underscore + 1; i < second_underscore; i++)
      outbuf[out++] = tempbuf[i];

    outbuf[out++] = ':';

    /* Copy column number */
    for (i = second_underscore + 1; i < temppos; i++)
      outbuf[out++] = tempbuf[i];

    outbuf[out++] = ')';
  } else {
    /* Fallback: just output the identifier as-is */
    for (i = 0; i < temppos; i++)
      outbuf[out++] = tempbuf[i];
  }

  *outpos = out;
  return 0;
}

char *oxcaml_demangle(const char *mangled_name, int option ATTRIBUTE_UNUSED) {
  const char *sym = mangled_name;
  char *outbuf;
  size_t sympos, outpos, len;

  if (sym[0] == '_' && sym[1] == 'O')
    sympos = 2;
  else
    return NULL;

  len = strlen(sym);
  outpos = 0;

  /* Allocate output buffer.
     In the worst case, the output can be longer than the input:
     - Partial applications add "(partially_applied)" (19 chars)
     - Anonymous locations add "fn(" + ")": adds ~4 chars
     - Dots between path elements: could add chars
     Allocate 2x + 32 bytes to be safe. */
  outbuf = malloc(len * 2 + 32);
  if (!outbuf)
    return NULL;

  /* Parse path items */
  while (sympos < len) {
    /* Check for terminating underscore */
    if (sym[sympos] == '_') {
      /* End of symbol path, rest is unique id */
      break;
    }

    /* Handle each path_item type */
    switch (sym[sympos]) {
      case 'M':  /* Module */
        if (outpos > 0)
          outbuf[outpos++] = '.';
        sympos++;
        if (decode_identifier(sym, &sympos, outbuf, &outpos) == ERROR)
          ENDONERROR();
        break;

      case 'F':  /* NamedFunction */
        if (outpos > 0)
          outbuf[outpos++] = '.';
        sympos++;
        if (decode_identifier(sym, &sympos, outbuf, &outpos) == ERROR)
          ENDONERROR();
        break;

      case 'L':  /* AnonymousFunction */
        if (outpos > 0)
          outbuf[outpos++] = '.';
        sympos++;
        if (decode_anonymous_location(sym, &sympos, outbuf, &outpos) == ERROR)
          ENDONERROR();
        break;

      case 'S':  /* AnonymousModule */
        if (outpos > 0)
          outbuf[outpos++] = '.';
        sympos++;
        if (decode_anonymous_location(sym, &sympos, outbuf, &outpos) == ERROR)
          ENDONERROR();
        break;

      case 'P':  /* PartialFunction (no dot separator) */
        sympos++;
        memcpy(outbuf + outpos, "(partially_applied)", 19);
        outpos += 19;
        break;

      default:
        /* Unknown path item type */
        ENDONERROR();
    }
  }

  outbuf[outpos] = '\0';
  return outbuf;
}
