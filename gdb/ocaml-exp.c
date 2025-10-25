/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 72 "ocaml-exp.y"


#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "ocaml-lang.h"
#include "charset.h"
#include "block.h"
#include "expop.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())
#define parse_ocaml_type(ps) builtin_ocaml_type (ps->gdbarch ())

/* Remap normal yacc parser interface names.  */
#define GDB_YY_REMAP_PREFIX ocaml_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

using namespace expr;


#line 104 "ocaml-exp.c.tmp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTRPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTRPTR nullptr
#   else
#    define YY_NULLPTRPTR 0
#   endif
#  else
#   define YY_NULLPTRPTR ((void*)0)
#  endif
# endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    IDENT = 258,                   /* IDENT  */
    INT = 259,                     /* INT  */
    FLOAT = 260,                   /* FLOAT  */
    STRING = 261,                  /* STRING  */
    UNIT = 262,                    /* UNIT  */
    LET = 263,                     /* LET  */
    IN = 264,                      /* IN  */
    IF = 265,                      /* IF  */
    THEN = 266,                    /* THEN  */
    ELSE = 267,                    /* ELSE  */
    MATCH = 268,                   /* MATCH  */
    WITH = 269,                    /* WITH  */
    FUNCTION = 270,                /* FUNCTION  */
    TRUE_KEYWORD = 271,            /* TRUE_KEYWORD  */
    FALSE_KEYWORD = 272,           /* FALSE_KEYWORD  */
    FUN = 273,                     /* FUN  */
    REC = 274,                     /* REC  */
    AND = 275,                     /* AND  */
    COLONCOLON = 276,              /* COLONCOLON  */
    ARROW = 277,                   /* ARROW  */
    LEQ = 278,                     /* LEQ  */
    GEQ = 279,                     /* GEQ  */
    NEQ = 280,                     /* NEQ  */
    ANDAND = 281,                  /* ANDAND  */
    OROR = 282,                    /* OROR  */
    ERROR = 283,                   /* ERROR  */
    UMINUS = 284                   /* UMINUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define IDENT 258
#define INT 259
#define FLOAT 260
#define STRING 261
#define UNIT 262
#define LET 263
#define IN 264
#define IF 265
#define THEN 266
#define ELSE 267
#define MATCH 268
#define WITH 269
#define FUNCTION 270
#define TRUE_KEYWORD 271
#define FALSE_KEYWORD 272
#define FUN 273
#define REC 274
#define AND 275
#define COLONCOLON 276
#define ARROW 277
#define LEQ 278
#define GEQ 279
#define NEQ 280
#define ANDAND 281
#define OROR 282
#define ERROR 283
#define UMINUS 284

/* Value type.  */
#if ! defined ocaml_exp_YYSTYPE && ! defined ocaml_exp_YYSTYPE_IS_DECLARED
union ocaml_exp_YYSTYPE
{
#line 110 "ocaml-exp.y"

    struct {
      LONGEST val;
      struct type *type;
    } typed_val_int;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct symbol *sym;
    struct type *tval;
    struct stoken sval;
    int voidval;
  

#line 228 "ocaml-exp.c.tmp"

};
typedef union ocaml_exp_YYSTYPE ocaml_exp_YYSTYPE;
# define ocaml_exp_YYSTYPE_IS_TRIVIAL 1
# define ocaml_exp_YYSTYPE_IS_DECLARED 1
#endif


extern ocaml_exp_YYSTYPE yylval;


int yyparse (void);



/* Symbol kind.  */
enum ocaml_exp_yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_IDENT = 3,                      /* IDENT  */
  YYSYMBOL_INT = 4,                        /* INT  */
  YYSYMBOL_FLOAT = 5,                      /* FLOAT  */
  YYSYMBOL_STRING = 6,                     /* STRING  */
  YYSYMBOL_UNIT = 7,                       /* UNIT  */
  YYSYMBOL_LET = 8,                        /* LET  */
  YYSYMBOL_IN = 9,                         /* IN  */
  YYSYMBOL_IF = 10,                        /* IF  */
  YYSYMBOL_THEN = 11,                      /* THEN  */
  YYSYMBOL_ELSE = 12,                      /* ELSE  */
  YYSYMBOL_MATCH = 13,                     /* MATCH  */
  YYSYMBOL_WITH = 14,                      /* WITH  */
  YYSYMBOL_FUNCTION = 15,                  /* FUNCTION  */
  YYSYMBOL_TRUE_KEYWORD = 16,              /* TRUE_KEYWORD  */
  YYSYMBOL_FALSE_KEYWORD = 17,             /* FALSE_KEYWORD  */
  YYSYMBOL_FUN = 18,                       /* FUN  */
  YYSYMBOL_REC = 19,                       /* REC  */
  YYSYMBOL_AND = 20,                       /* AND  */
  YYSYMBOL_COLONCOLON = 21,                /* COLONCOLON  */
  YYSYMBOL_ARROW = 22,                     /* ARROW  */
  YYSYMBOL_LEQ = 23,                       /* LEQ  */
  YYSYMBOL_GEQ = 24,                       /* GEQ  */
  YYSYMBOL_NEQ = 25,                       /* NEQ  */
  YYSYMBOL_ANDAND = 26,                    /* ANDAND  */
  YYSYMBOL_OROR = 27,                      /* OROR  */
  YYSYMBOL_ERROR = 28,                     /* ERROR  */
  YYSYMBOL_29_ = 29,                       /* '<'  */
  YYSYMBOL_30_ = 30,                       /* '>'  */
  YYSYMBOL_31_ = 31,                       /* '='  */
  YYSYMBOL_32_ = 32,                       /* '+'  */
  YYSYMBOL_33_ = 33,                       /* '-'  */
  YYSYMBOL_34_ = 34,                       /* '*'  */
  YYSYMBOL_35_ = 35,                       /* '/'  */
  YYSYMBOL_36_ = 36,                       /* '%'  */
  YYSYMBOL_UMINUS = 37,                    /* UMINUS  */
  YYSYMBOL_38_ = 38,                       /* '.'  */
  YYSYMBOL_39_ = 39,                       /* '('  */
  YYSYMBOL_40_ = 40,                       /* ')'  */
  YYSYMBOL_41_ = 41,                       /* ','  */
  YYSYMBOL_42_ = 42,                       /* '['  */
  YYSYMBOL_43_ = 43,                       /* ']'  */
  YYSYMBOL_44_ = 44,                       /* '|'  */
  YYSYMBOL_45_ = 45,                       /* ';'  */
  YYSYMBOL_46_ = 46,                       /* '{'  */
  YYSYMBOL_47_ = 47,                       /* '}'  */
  YYSYMBOL_YYACCEPT = 48,                  /* $accept  */
  YYSYMBOL_start = 49,                     /* start  */
  YYSYMBOL_exp = 50,                       /* exp  */
  YYSYMBOL_simple_exp = 51,                /* simple_exp  */
  YYSYMBOL_tuple_exp = 52,                 /* tuple_exp  */
  YYSYMBOL_list_exp = 53,                  /* list_exp  */
  YYSYMBOL_array_exp = 54,                 /* array_exp  */
  YYSYMBOL_record_exp = 55,                /* record_exp  */
  YYSYMBOL_record_fields = 56              /* record_fields  */
};
typedef enum ocaml_exp_yysymbol_kind_t ocaml_exp_yysymbol_kind_t;


/* Second part of user prologue.  */
#line 125 "ocaml-exp.y"

/* ocaml_exp_YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *, const char *,
			 int, int, ocaml_exp_YYSTYPE *);

#line 316 "ocaml-exp.c.tmp"


#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or xmalloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined xmalloc) \
             && (defined YYFREE || defined xfree)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC xmalloc
#   if ! defined xmalloc && ! defined EXIT_SUCCESS
void *xmalloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE xfree
#   if ! defined xfree && ! defined EXIT_SUCCESS
void xfree (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined ocaml_exp_YYSTYPE_IS_TRIVIAL && ocaml_exp_YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union ocaml_exp_yyalloc
{
  yy_state_t yyss_alloc;
  ocaml_exp_YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union ocaml_exp_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (ocaml_exp_YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  31
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   796

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  48
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  9
/* YYNRULES -- Number of rules.  */
#define YYNRULES  44
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  94

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   284


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (ocaml_exp_yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,    36,     2,     2,
      39,    40,    34,    32,    41,    33,    38,    35,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    45,
      29,    31,    30,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    42,     2,    43,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    46,    44,    47,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    37
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   172,   172,   176,   177,   179,   181,   183,   185,   187,
     189,   191,   193,   195,   197,   199,   201,   203,   205,   214,
     219,   228,   242,   247,   254,   259,   265,   271,   277,   299,
     323,   327,   328,   329,   330,   334,   339,   354,   360,   372,
     379,   385,   394,   403,   408
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (ocaml_exp_yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (ocaml_exp_yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "IDENT", "INT",
  "FLOAT", "STRING", "UNIT", "LET", "IN", "IF", "THEN", "ELSE", "MATCH",
  "WITH", "FUNCTION", "TRUE_KEYWORD", "FALSE_KEYWORD", "FUN", "REC", "AND",
  "COLONCOLON", "ARROW", "LEQ", "GEQ", "NEQ", "ANDAND", "OROR", "ERROR",
  "'<'", "'>'", "'='", "'+'", "'-'", "'*'", "'/'", "'%'", "UMINUS", "'.'",
  "'('", "')'", "','", "'['", "']'", "'|'", "';'", "'{'", "'}'", "$accept",
  "start", "exp", "simple_exp", "tuple_exp", "list_exp", "array_exp",
  "record_exp", "record_fields", YY_NULLPTRPTR
};

static const char *
yysymbol_name (ocaml_exp_yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-45)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-18)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     750,   -36,   -45,   -45,   -45,   -45,     1,   750,   -45,   -45,
     750,   750,    14,     2,     6,   464,   -45,   -45,   -45,   -45,
     -45,     5,   -22,   200,   730,    68,   -45,   661,   244,   -18,
     -44,   -45,   750,   750,   750,   750,   750,   750,   750,   750,
     750,   750,   750,   750,   750,   750,   730,   -45,   750,   750,
     -45,   750,   -29,   112,   -45,   750,    12,   -45,   508,   640,
     640,   640,   596,   552,   640,   640,   640,   685,   706,   730,
     730,   730,   288,   332,   156,   -45,   -27,   750,   464,    -8,
     750,   750,   -45,   750,   -45,   376,   750,   464,   464,   420,
     -17,   464,   -45,   -45
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    28,    22,    23,    24,    27,     0,     0,    25,    26,
       0,     0,     0,     0,     0,     2,     3,    31,    32,    33,
      34,     0,     0,     0,    17,     0,    37,     0,     0,     0,
       0,     1,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    21,    29,     0,     0,
      30,     0,     0,     0,    38,     0,     0,    42,    19,    11,
      12,    14,    15,    16,     9,    10,    13,     4,     5,     6,
       7,     8,     0,     0,     0,    39,     0,     0,    43,     0,
       0,     0,    35,     0,    40,     0,     0,    20,    18,     0,
       0,    44,    36,    41
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -45,   -45,     0,   -45,   -45,   -45,   -45,   -45,   -45
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,    14,    46,    16,    17,    18,    19,    20,    30
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      15,    56,    21,    57,    22,    29,    31,    23,    47,    48,
      24,    25,    28,    55,    75,    79,    84,     1,     2,     3,
       4,     5,     6,    86,     7,     0,    93,    53,     0,     0,
       8,     9,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,     0,    10,    72,    73,
       0,    74,     0,    11,     0,    78,    12,    26,    27,     0,
      13,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     1,     2,     3,     4,     5,     6,    85,     7,     0,
      87,    88,     0,    89,     8,     9,    91,     0,     0,    32,
       0,    33,    34,    35,    36,    37,     0,    38,    39,    40,
      41,    42,    43,    44,    45,     0,     0,    11,    50,    51,
      12,     0,     0,     0,    13,     1,     2,     3,     4,     5,
       6,     0,     7,     0,     0,     0,     0,     0,     8,     9,
       0,     0,     0,    32,     0,    33,    34,    35,    36,    37,
       0,    38,    39,    40,    41,    42,    43,    44,    45,     0,
       0,    11,     0,     0,    12,     0,    76,    77,    13,     1,
       2,     3,     4,     5,     6,     0,     7,     0,     0,     0,
       0,     0,     8,     9,     0,     0,     0,    32,     0,    33,
      34,    35,    36,    37,     0,    38,    39,    40,    41,    42,
      43,    44,    45,     0,     0,    11,    82,    83,    12,     0,
       0,     0,    13,     1,     2,     3,     4,     5,     6,     0,
       7,    49,     0,     0,     0,     0,     8,     9,     0,     0,
       0,    32,     0,    33,    34,    35,    36,    37,     0,    38,
      39,    40,    41,    42,    43,    44,    45,     0,     0,    11,
       0,     0,    12,     0,     0,     0,    13,     1,     2,     3,
       4,     5,     6,     0,     7,     0,     0,     0,     0,     0,
       8,     9,     0,     0,     0,    32,     0,    33,    34,    35,
      36,    37,     0,    38,    39,    40,    41,    42,    43,    44,
      45,     0,     0,    11,     0,     0,    12,    54,     0,     0,
      13,     1,     2,     3,     4,     5,     6,    80,     7,     0,
       0,     0,     0,     0,     8,     9,     0,     0,     0,    32,
       0,    33,    34,    35,    36,    37,     0,    38,    39,    40,
      41,    42,    43,    44,    45,     0,     0,    11,     0,     0,
      12,     0,     0,     0,    13,     1,     2,     3,     4,     5,
       6,     0,     7,     0,    81,     0,     0,     0,     8,     9,
       0,     0,     0,    32,     0,    33,    34,    35,    36,    37,
       0,    38,    39,    40,    41,    42,    43,    44,    45,     0,
       0,    11,     0,     0,    12,     0,     0,     0,    13,     1,
       2,     3,     4,     5,     6,     0,     7,     0,     0,     0,
       0,     0,     8,     9,     0,     0,     0,    32,     0,    33,
      34,    35,    36,    37,     0,    38,    39,    40,    41,    42,
      43,    44,    45,     0,     0,    11,     0,     0,    12,     0,
      90,     0,    13,     1,     2,     3,     4,     5,     6,     0,
       7,     0,     0,     0,     0,     0,     8,     9,     0,     0,
       0,    32,     0,    33,    34,    35,    36,    37,     0,    38,
      39,    40,    41,    42,    43,    44,    45,     0,     0,    11,
      92,     0,    12,     0,     0,     0,    13,     1,     2,     3,
       4,     5,     6,     0,     7,     0,     0,     0,     0,     0,
       8,     9,     0,     0,     0,    32,     0,    33,    34,    35,
      36,    37,     0,    38,    39,    40,    41,    42,    43,    44,
      45,     0,     0,    11,     0,     0,    12,     0,     0,     0,
      13,     1,     2,     3,     4,     5,     6,     0,     0,     0,
       0,     0,     0,     0,     8,     9,     0,     0,     0,    32,
       0,    33,    34,    35,    36,    37,     0,    38,    39,    40,
      41,    42,    43,    44,    45,     0,     0,    11,     0,     0,
      12,     0,     0,     0,    13,     1,     2,     3,     4,     5,
       6,     0,     0,     0,     0,     0,     0,     0,     8,     9,
       0,     0,     0,     0,     0,    33,    34,    35,    36,     0,
       0,    38,    39,    40,    41,    42,    43,    44,    45,     0,
       0,    11,     0,     0,    12,     0,     0,     0,    13,     1,
       2,     3,     4,     5,     6,     0,     0,     0,     0,     0,
       0,     0,     8,     9,     0,     0,     0,     0,     0,    33,
      34,    35,     0,     0,     0,    38,    39,    40,    41,    42,
      43,    44,    45,     0,     0,    11,     0,     0,    12,     0,
       0,     0,    13,     1,     2,     3,     4,     5,     6,     0,
       0,     0,     0,     0,     0,     0,     8,     9,     0,     0,
       0,     0,     0,     0,     1,     2,     3,     4,     5,     6,
       0,     7,    41,    42,    43,    44,    45,     8,     9,    11,
       0,     0,    12,     0,     0,     0,    13,     0,     1,     2,
       3,     4,     5,     6,    10,     0,     0,     0,     0,     0,
      11,     8,     9,    12,     0,    52,     0,    13,     0,     1,
       2,     3,     4,     5,     6,     0,     0,     0,     0,    43,
      44,    45,     8,     9,    11,     0,     0,    12,     0,     0,
       0,    13,     0,     1,     2,     3,     4,     5,     6,     0,
     -17,   -17,   -17,     0,     0,    11,     8,     9,    12,     0,
       0,     0,    13,     1,     2,     3,     4,     5,     6,     0,
       7,     0,     0,     0,     0,     0,     8,     9,     0,    11,
       0,     0,    12,     0,     0,     0,    13,     0,     0,     0,
       0,     0,     0,    10,     0,     0,     0,     0,     0,    11,
       0,     0,    12,     0,     0,     0,    13
};

static const yytype_int8 yycheck[] =
{
       0,    45,    38,    47,     3,     3,     0,     7,     3,    31,
      10,    11,    12,    31,    43,     3,    43,     3,     4,     5,
       6,     7,     8,    31,    10,    -1,    43,    27,    -1,    -1,
      16,    17,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    -1,    33,    48,    49,
      -1,    51,    -1,    39,    -1,    55,    42,    43,    44,    -1,
      46,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,     5,     6,     7,     8,    77,    10,    -1,
      80,    81,    -1,    83,    16,    17,    86,    -1,    -1,    21,
      -1,    23,    24,    25,    26,    27,    -1,    29,    30,    31,
      32,    33,    34,    35,    36,    -1,    -1,    39,    40,    41,
      42,    -1,    -1,    -1,    46,     3,     4,     5,     6,     7,
       8,    -1,    10,    -1,    -1,    -1,    -1,    -1,    16,    17,
      -1,    -1,    -1,    21,    -1,    23,    24,    25,    26,    27,
      -1,    29,    30,    31,    32,    33,    34,    35,    36,    -1,
      -1,    39,    -1,    -1,    42,    -1,    44,    45,    46,     3,
       4,     5,     6,     7,     8,    -1,    10,    -1,    -1,    -1,
      -1,    -1,    16,    17,    -1,    -1,    -1,    21,    -1,    23,
      24,    25,    26,    27,    -1,    29,    30,    31,    32,    33,
      34,    35,    36,    -1,    -1,    39,    40,    41,    42,    -1,
      -1,    -1,    46,     3,     4,     5,     6,     7,     8,    -1,
      10,    11,    -1,    -1,    -1,    -1,    16,    17,    -1,    -1,
      -1,    21,    -1,    23,    24,    25,    26,    27,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,    -1,    -1,    39,
      -1,    -1,    42,    -1,    -1,    -1,    46,     3,     4,     5,
       6,     7,     8,    -1,    10,    -1,    -1,    -1,    -1,    -1,
      16,    17,    -1,    -1,    -1,    21,    -1,    23,    24,    25,
      26,    27,    -1,    29,    30,    31,    32,    33,    34,    35,
      36,    -1,    -1,    39,    -1,    -1,    42,    43,    -1,    -1,
      46,     3,     4,     5,     6,     7,     8,     9,    10,    -1,
      -1,    -1,    -1,    -1,    16,    17,    -1,    -1,    -1,    21,
      -1,    23,    24,    25,    26,    27,    -1,    29,    30,    31,
      32,    33,    34,    35,    36,    -1,    -1,    39,    -1,    -1,
      42,    -1,    -1,    -1,    46,     3,     4,     5,     6,     7,
       8,    -1,    10,    -1,    12,    -1,    -1,    -1,    16,    17,
      -1,    -1,    -1,    21,    -1,    23,    24,    25,    26,    27,
      -1,    29,    30,    31,    32,    33,    34,    35,    36,    -1,
      -1,    39,    -1,    -1,    42,    -1,    -1,    -1,    46,     3,
       4,     5,     6,     7,     8,    -1,    10,    -1,    -1,    -1,
      -1,    -1,    16,    17,    -1,    -1,    -1,    21,    -1,    23,
      24,    25,    26,    27,    -1,    29,    30,    31,    32,    33,
      34,    35,    36,    -1,    -1,    39,    -1,    -1,    42,    -1,
      44,    -1,    46,     3,     4,     5,     6,     7,     8,    -1,
      10,    -1,    -1,    -1,    -1,    -1,    16,    17,    -1,    -1,
      -1,    21,    -1,    23,    24,    25,    26,    27,    -1,    29,
      30,    31,    32,    33,    34,    35,    36,    -1,    -1,    39,
      40,    -1,    42,    -1,    -1,    -1,    46,     3,     4,     5,
       6,     7,     8,    -1,    10,    -1,    -1,    -1,    -1,    -1,
      16,    17,    -1,    -1,    -1,    21,    -1,    23,    24,    25,
      26,    27,    -1,    29,    30,    31,    32,    33,    34,    35,
      36,    -1,    -1,    39,    -1,    -1,    42,    -1,    -1,    -1,
      46,     3,     4,     5,     6,     7,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    16,    17,    -1,    -1,    -1,    21,
      -1,    23,    24,    25,    26,    27,    -1,    29,    30,    31,
      32,    33,    34,    35,    36,    -1,    -1,    39,    -1,    -1,
      42,    -1,    -1,    -1,    46,     3,     4,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    16,    17,
      -1,    -1,    -1,    -1,    -1,    23,    24,    25,    26,    -1,
      -1,    29,    30,    31,    32,    33,    34,    35,    36,    -1,
      -1,    39,    -1,    -1,    42,    -1,    -1,    -1,    46,     3,
       4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    16,    17,    -1,    -1,    -1,    -1,    -1,    23,
      24,    25,    -1,    -1,    -1,    29,    30,    31,    32,    33,
      34,    35,    36,    -1,    -1,    39,    -1,    -1,    42,    -1,
      -1,    -1,    46,     3,     4,     5,     6,     7,     8,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    16,    17,    -1,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,     8,
      -1,    10,    32,    33,    34,    35,    36,    16,    17,    39,
      -1,    -1,    42,    -1,    -1,    -1,    46,    -1,     3,     4,
       5,     6,     7,     8,    33,    -1,    -1,    -1,    -1,    -1,
      39,    16,    17,    42,    -1,    44,    -1,    46,    -1,     3,
       4,     5,     6,     7,     8,    -1,    -1,    -1,    -1,    34,
      35,    36,    16,    17,    39,    -1,    -1,    42,    -1,    -1,
      -1,    46,    -1,     3,     4,     5,     6,     7,     8,    -1,
      34,    35,    36,    -1,    -1,    39,    16,    17,    42,    -1,
      -1,    -1,    46,     3,     4,     5,     6,     7,     8,    -1,
      10,    -1,    -1,    -1,    -1,    -1,    16,    17,    -1,    39,
      -1,    -1,    42,    -1,    -1,    -1,    46,    -1,    -1,    -1,
      -1,    -1,    -1,    33,    -1,    -1,    -1,    -1,    -1,    39,
      -1,    -1,    42,    -1,    -1,    -1,    46
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,    10,    16,    17,
      33,    39,    42,    46,    49,    50,    51,    52,    53,    54,
      55,    38,     3,    50,    50,    50,    43,    44,    50,     3,
      56,     0,    21,    23,    24,    25,    26,    27,    29,    30,
      31,    32,    33,    34,    35,    36,    50,     3,    31,    11,
      40,    41,    44,    50,    43,    31,    45,    47,    50,    50,
      50,    50,    50,    50,    50,    50,    50,    50,    50,    50,
      50,    50,    50,    50,    50,    43,    44,    45,    50,     3,
       9,    12,    40,    41,    43,    50,    31,    50,    50,    50,
      44,    50,    40,    43
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    48,    49,    50,    50,    50,    50,    50,    50,    50,
      50,    50,    50,    50,    50,    50,    50,    50,    50,    50,
      50,    50,    51,    51,    51,    51,    51,    51,    51,    51,
      51,    51,    51,    51,    51,    52,    52,    53,    53,    54,
      54,    54,    55,    56,    56
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     6,     3,
       6,     2,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     1,     1,     1,     1,     5,     7,     2,     3,     4,
       5,     7,     3,     3,     5
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       ocaml_exp_yysymbol_kind_t yykind, ocaml_exp_YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 ocaml_exp_yysymbol_kind_t yykind, ocaml_exp_YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, ocaml_exp_YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            ocaml_exp_yysymbol_kind_t yykind, ocaml_exp_YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
ocaml_exp_YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to xreallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    ocaml_exp_YYSTYPE yyvsa[YYINITDEPTH];
    ocaml_exp_YYSTYPE *yyvs = yyvsa;
    ocaml_exp_YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  ocaml_exp_yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  ocaml_exp_YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to xreallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        ocaml_exp_YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union ocaml_exp_yyalloc *yyptr =
          YY_CAST (union ocaml_exp_yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 4: /* exp: exp '+' exp  */
#line 178 "ocaml-exp.y"
                { pstate->wrap2<add_operation> (); }
#line 1463 "ocaml-exp.c.tmp"
    break;

  case 5: /* exp: exp '-' exp  */
#line 180 "ocaml-exp.y"
                { pstate->wrap2<sub_operation> (); }
#line 1469 "ocaml-exp.c.tmp"
    break;

  case 6: /* exp: exp '*' exp  */
#line 182 "ocaml-exp.y"
                { pstate->wrap2<mul_operation> (); }
#line 1475 "ocaml-exp.c.tmp"
    break;

  case 7: /* exp: exp '/' exp  */
#line 184 "ocaml-exp.y"
                { pstate->wrap2<div_operation> (); }
#line 1481 "ocaml-exp.c.tmp"
    break;

  case 8: /* exp: exp '%' exp  */
#line 186 "ocaml-exp.y"
                { pstate->wrap2<rem_operation> (); }
#line 1487 "ocaml-exp.c.tmp"
    break;

  case 9: /* exp: exp '<' exp  */
#line 188 "ocaml-exp.y"
                { pstate->wrap2<less_operation> (); }
#line 1493 "ocaml-exp.c.tmp"
    break;

  case 10: /* exp: exp '>' exp  */
#line 190 "ocaml-exp.y"
                { pstate->wrap2<gtr_operation> (); }
#line 1499 "ocaml-exp.c.tmp"
    break;

  case 11: /* exp: exp LEQ exp  */
#line 192 "ocaml-exp.y"
                { pstate->wrap2<leq_operation> (); }
#line 1505 "ocaml-exp.c.tmp"
    break;

  case 12: /* exp: exp GEQ exp  */
#line 194 "ocaml-exp.y"
                { pstate->wrap2<geq_operation> (); }
#line 1511 "ocaml-exp.c.tmp"
    break;

  case 13: /* exp: exp '=' exp  */
#line 196 "ocaml-exp.y"
                { pstate->wrap2<equal_operation> (); }
#line 1517 "ocaml-exp.c.tmp"
    break;

  case 14: /* exp: exp NEQ exp  */
#line 198 "ocaml-exp.y"
                { pstate->wrap2<notequal_operation> (); }
#line 1523 "ocaml-exp.c.tmp"
    break;

  case 15: /* exp: exp ANDAND exp  */
#line 200 "ocaml-exp.y"
                { pstate->wrap2<logical_and_operation> (); }
#line 1529 "ocaml-exp.c.tmp"
    break;

  case 16: /* exp: exp OROR exp  */
#line 202 "ocaml-exp.y"
                { pstate->wrap2<logical_or_operation> (); }
#line 1535 "ocaml-exp.c.tmp"
    break;

  case 17: /* exp: '-' exp  */
#line 204 "ocaml-exp.y"
                { pstate->wrap<unary_neg_operation> (); }
#line 1541 "ocaml-exp.c.tmp"
    break;

  case 18: /* exp: IF exp THEN exp ELSE exp  */
#line 206 "ocaml-exp.y"
                {
		  operation_up last = pstate->pop ();
		  operation_up mid = pstate->pop ();
		  operation_up first = pstate->pop ();
		  pstate->push_new<ternop_cond_operation>
		    (std::move (first), std::move (mid),
		     std::move (last));
		}
#line 1554 "ocaml-exp.c.tmp"
    break;

  case 19: /* exp: exp COLONCOLON exp  */
#line 215 "ocaml-exp.y"
                {
		  /* List cons operator - creates a cons cell (tag 0, size 2) */
		  pstate->wrap2<comma_operation> ();
		}
#line 1563 "ocaml-exp.c.tmp"
    break;

  case 20: /* exp: LET IDENT '=' exp IN exp  */
#line 220 "ocaml-exp.y"
                {
		  /* TODO: Implement proper let binding
		     For now, just evaluate the body expression */
		  operation_up body = pstate->pop ();
		  operation_up init = pstate->pop ();
		  /* Discard init, return body for now */
		  pstate->push (std::move (body));
		}
#line 1576 "ocaml-exp.c.tmp"
    break;

  case 21: /* exp: exp exp  */
#line 229 "ocaml-exp.y"
                {
		  /* Function application
		     In OCaml, 'f x' applies function f to argument x */
		  operation_up arg = pstate->pop ();
		  operation_up func = pstate->pop ();
		  std::vector<operation_up> args;
		  args.push_back (std::move (arg));
		  pstate->push_new<funcall_operation>
		    (std::move (func), std::move (args));
		}
#line 1591 "ocaml-exp.c.tmp"
    break;

  case 22: /* simple_exp: INT  */
#line 243 "ocaml-exp.y"
                {
		  pstate->push_new<long_const_operation>
		    ((yyvsp[0].typed_val_int).type, (yyvsp[0].typed_val_int).val);
		}
#line 1600 "ocaml-exp.c.tmp"
    break;

  case 23: /* simple_exp: FLOAT  */
#line 248 "ocaml-exp.y"
                {
		  float_data data;
		  std::copy (std::begin ((yyvsp[0].typed_val_float).val), std::end ((yyvsp[0].typed_val_float).val),
			     std::begin (data));
		  pstate->push_new<float_const_operation> ((yyvsp[0].typed_val_float).type, data);
		}
#line 1611 "ocaml-exp.c.tmp"
    break;

  case 24: /* simple_exp: STRING  */
#line 255 "ocaml-exp.y"
                {
		  pstate->push_new<string_operation>
		    (copy_name ((yyvsp[0].sval)));
		}
#line 1620 "ocaml-exp.c.tmp"
    break;

  case 25: /* simple_exp: TRUE_KEYWORD  */
#line 260 "ocaml-exp.y"
                {
		  /* OCaml true = tagged int 3 */
		  pstate->push_new<long_const_operation>
		    (parse_ocaml_type (pstate)->builtin_bool, 3);
		}
#line 1630 "ocaml-exp.c.tmp"
    break;

  case 26: /* simple_exp: FALSE_KEYWORD  */
#line 266 "ocaml-exp.y"
                {
		  /* OCaml false = tagged int 1 */
		  pstate->push_new<long_const_operation>
		    (parse_ocaml_type (pstate)->builtin_bool, 1);
		}
#line 1640 "ocaml-exp.c.tmp"
    break;

  case 27: /* simple_exp: UNIT  */
#line 272 "ocaml-exp.y"
                {
		  /* OCaml () = tagged int 1 */
		  pstate->push_new<long_const_operation>
		    (parse_ocaml_type (pstate)->builtin_unit, 1);
		}
#line 1650 "ocaml-exp.c.tmp"
    break;

  case 28: /* simple_exp: IDENT  */
#line 278 "ocaml-exp.y"
                {
		  /* Variable or function name - look up symbol */
		  std::string name = copy_name ((yyvsp[0].sval));
		  struct block_symbol sym;
		  sym = lookup_symbol (name.c_str (),
		                       pstate->expression_context_block,
		                       SEARCH_VFT, nullptr);

		  if (sym.symbol)
		    pstate->push_new<var_value_operation> (sym);
		  else
		    {
		      /* Try minimal symbol lookup */
		      bound_minimal_symbol msymbol =
		        lookup_minimal_symbol (current_program_space, name.c_str ());
		      if (msymbol.minsym != NULL)
		        pstate->push_new<var_msym_value_operation> (msymbol);
		      else
		        error (_("No symbol \"%s\" in current context."), name.c_str ());
		    }
		}
#line 1676 "ocaml-exp.c.tmp"
    break;

  case 29: /* simple_exp: IDENT '.' IDENT  */
#line 300 "ocaml-exp.y"
                {
		  /* Module-qualified name: Module.name */
		  std::string fullname = std::string ((yyvsp[-2].sval).ptr, (yyvsp[-2].sval).length) + "."
		    + std::string ((yyvsp[0].sval).ptr, (yyvsp[0].sval).length);

		  struct block_symbol sym;
		  sym = lookup_symbol (fullname.c_str (),
		                       pstate->expression_context_block,
		                       SEARCH_VFT, nullptr);

		  if (sym.symbol)
		    pstate->push_new<var_value_operation> (sym);
		  else
		    {
		      /* Try minimal symbol lookup */
		      bound_minimal_symbol msymbol =
		        lookup_minimal_symbol (current_program_space, fullname.c_str ());
		      if (msymbol.minsym != NULL)
		        pstate->push_new<var_msym_value_operation> (msymbol);
		      else
		        error (_("No symbol \"%s\" in current context."), fullname.c_str ());
		    }
		}
#line 1704 "ocaml-exp.c.tmp"
    break;

  case 30: /* simple_exp: '(' exp ')'  */
#line 324 "ocaml-exp.y"
                {
		  /* Parenthesized expression */
		}
#line 1712 "ocaml-exp.c.tmp"
    break;

  case 35: /* tuple_exp: '(' exp ',' exp ')'  */
#line 335 "ocaml-exp.y"
                {
		  /* 2-tuple */
		  pstate->wrap2<comma_operation> ();
		}
#line 1721 "ocaml-exp.c.tmp"
    break;

  case 36: /* tuple_exp: '(' exp ',' exp ',' exp ')'  */
#line 340 "ocaml-exp.y"
                {
		  /* 3-tuple */
		  operation_up third = pstate->pop ();
		  operation_up second = pstate->pop ();
		  operation_up first = pstate->pop ();
		  pstate->push_new<comma_operation>
		    (std::move (first), std::move (second));
		  operation_up tuple2 = pstate->pop ();
		  pstate->push_new<comma_operation>
		    (std::move (tuple2), std::move (third));
		}
#line 1737 "ocaml-exp.c.tmp"
    break;

  case 37: /* list_exp: '[' ']'  */
#line 355 "ocaml-exp.y"
                {
		  /* Empty list = tagged int 1 */
		  pstate->push_new<long_const_operation>
		    (parse_ocaml_type (pstate)->builtin_int, 1);
		}
#line 1747 "ocaml-exp.c.tmp"
    break;

  case 38: /* list_exp: '[' exp ']'  */
#line 361 "ocaml-exp.y"
                {
		  /* Single element list [x] = x :: [] */
		  operation_up elem = pstate->pop ();
		  pstate->push_new<long_const_operation>
		    (parse_ocaml_type (pstate)->builtin_int, 1);  /* [] */
		  pstate->push (std::move (elem));
		  pstate->wrap2<comma_operation> ();  /* Cons cell */
		}
#line 1760 "ocaml-exp.c.tmp"
    break;

  case 39: /* array_exp: '[' '|' '|' ']'  */
#line 373 "ocaml-exp.y"
                {
		  /* Empty array [||]
		     TODO: Proper array representation - for now, create a marker */
		  pstate->push_new<long_const_operation>
		    (parse_ocaml_type (pstate)->builtin_int, 0);
		}
#line 1771 "ocaml-exp.c.tmp"
    break;

  case 40: /* array_exp: '[' '|' exp '|' ']'  */
#line 380 "ocaml-exp.y"
                {
		  /* Single element array [|x|]
		     OCaml arrays are blocks with tag 0, similar to tuples
		     For now, just return the single element */
		}
#line 1781 "ocaml-exp.c.tmp"
    break;

  case 41: /* array_exp: '[' '|' exp ';' exp '|' ']'  */
#line 386 "ocaml-exp.y"
                {
		  /* Two-element array [|x; y|]
		     Represented as a 2-tuple for debugging purposes */
		  pstate->wrap2<comma_operation> ();
		}
#line 1791 "ocaml-exp.c.tmp"
    break;

  case 42: /* record_exp: '{' record_fields '}'  */
#line 395 "ocaml-exp.y"
                {
		  /* Record expression
		     TODO: Proper record representation with field names
		     For now, the fields are already on the stack */
		}
#line 1801 "ocaml-exp.c.tmp"
    break;

  case 43: /* record_fields: IDENT '=' exp  */
#line 404 "ocaml-exp.y"
                {
		  /* Single field - value is already on stack, discard field name for now
		     TODO: Store field names for better debugging */
		}
#line 1810 "ocaml-exp.c.tmp"
    break;

  case 44: /* record_fields: record_fields ';' IDENT '=' exp  */
#line 409 "ocaml-exp.y"
                {
		  /* Multiple fields - combine with comma operation
		     TODO: Store field names */
		  pstate->wrap2<comma_operation> ();
		}
#line 1820 "ocaml-exp.c.tmp"
    break;


#line 1824 "ocaml-exp.c.tmp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (ocaml_exp_yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 416 "ocaml-exp.y"


/* Called to report a parse error.  */

static void
yyerror (const char *msg)
{
  pstate->parse_error (msg);
}

/* Lexer implementation */

struct token
{
  const char *oper;
  int token;
};

static const struct token tokentab[] =
{
  {"<>", NEQ},
  {"<=", LEQ},
  {">=", GEQ},
  {"->", ARROW},
  {"::", COLONCOLON},
  {"&&", ANDAND},
  {"||", OROR},
  {NULL, 0}
};

/* Lexical analyzer.  */

static int
yylex (void)
{
  int c;
  int namelen;
  const char *tokstart;
  const char *p;

 retry:

  pstate->prev_lexptr = pstate->lexptr;
  tokstart = pstate->lexptr;

  /* Skip whitespace.  */
  c = *tokstart;
  while (c == ' ' || c == '\t' || c == '\n')
    c = *++tokstart;

  pstate->lexptr = tokstart;

  if (c == '\0')
    return 0;

  /* Handle comments (* ... *) */
  if (c == '(' && tokstart[1] == '*')
    {
      int depth = 1;
      p = tokstart + 2;
      while (*p && depth > 0)
	{
	  if (p[0] == '(' && p[1] == '*')
	    {
	      depth++;
	      p += 2;
	    }
	  else if (p[0] == '*' && p[1] == ')')
	    {
	      depth--;
	      p += 2;
	    }
	  else
	    p++;
	}
      pstate->lexptr = p;
      goto retry;
    }

  /* Check for two-character operators.  */
  for (int i = 0; tokentab[i].oper != NULL; i++)
    {
      int len = strlen (tokentab[i].oper);
      if (strncmp (tokstart, tokentab[i].oper, len) == 0)
	{
	  pstate->lexptr = tokstart + len;
	  return tokentab[i].token;
	}
    }

  /* Handle numbers.  */
  if (isdigit (c) || (c == '-' && isdigit (tokstart[1])))
    {
      p = tokstart;
      if (c == '-')
	p++;
      while (isdigit (*p))
	p++;

      /* Float? */
      if (*p == '.' && isdigit (p[1]))
	{
	  p++;
	  while (isdigit (*p))
	    p++;
	  if (*p == 'e' || *p == 'E')
	    {
	      p++;
	      if (*p == '+' || *p == '-')
		p++;
	      while (isdigit (*p))
		p++;
	    }

	  namelen = p - tokstart;
	  std::string number (tokstart, namelen);
	  yylval.typed_val_float.type = parse_ocaml_type (pstate)->builtin_float;

	  if (!parse_float (number.c_str (), namelen,
			    yylval.typed_val_float.type,
			    yylval.typed_val_float.val))
	    return ERROR;

	  pstate->lexptr = p;
	  return FLOAT;
	}

      /* Integer.  */
      namelen = p - tokstart;
      pstate->lexptr = p;
      return parse_number (pstate, tokstart, namelen, 0, &yylval);
    }

  /* Handle strings.  */
  if (c == '"')
    {
      int len = 0;
      p = tokstart + 1;
      while (*p && *p != '"')
	{
	  if (*p == '\\' && p[1])
	    p += 2;
	  else
	    p++;
	  len++;
	}
      if (*p != '"')
	error (_("Unterminated string"));

      yylval.sval.ptr = tokstart + 1;
      yylval.sval.length = len;
      pstate->lexptr = p + 1;
      return STRING;
    }

  /* Handle identifiers and keywords.  */
  if (isalpha (c) || c == '_')
    {
      p = tokstart;
      while (isalnum (*p) || *p == '_' || *p == '\'')
	p++;

      namelen = p - tokstart;
      pstate->lexptr = p;

      /* Check for keywords.  */
      if (namelen == 3 && strncmp (tokstart, "let", 3) == 0)
	return LET;
      if (namelen == 2 && strncmp (tokstart, "in", 2) == 0)
	return IN;
      if (namelen == 2 && strncmp (tokstart, "if", 2) == 0)
	return IF;
      if (namelen == 4 && strncmp (tokstart, "then", 4) == 0)
	return THEN;
      if (namelen == 4 && strncmp (tokstart, "else", 4) == 0)
	return ELSE;
      if (namelen == 4 && strncmp (tokstart, "true", 4) == 0)
	return TRUE_KEYWORD;
      if (namelen == 5 && strncmp (tokstart, "false", 5) == 0)
	return FALSE_KEYWORD;
      if (namelen == 5 && strncmp (tokstart, "match", 5) == 0)
	return MATCH;
      if (namelen == 4 && strncmp (tokstart, "with", 4) == 0)
	return WITH;
      if (namelen == 8 && strncmp (tokstart, "function", 8) == 0)
	return FUNCTION;
      if (namelen == 3 && strncmp (tokstart, "fun", 3) == 0)
	return FUN;
      if (namelen == 3 && strncmp (tokstart, "rec", 3) == 0)
	return REC;
      if (namelen == 3 && strncmp (tokstart, "and", 3) == 0)
	return AND;

      /* Unit () is lexed as a keyword.  */
      if (namelen == 0 && c == '(' && tokstart[1] == ')')
	{
	  pstate->lexptr = tokstart + 2;
	  return UNIT;
	}

      yylval.sval.ptr = tokstart;
      yylval.sval.length = namelen;
      return IDENT;
    }

  /* Single character tokens.  */
  pstate->lexptr = tokstart + 1;
  return c;
}

/* Parse a number from STR.  */

static int
parse_number (struct parser_state *ps, const char *str, int len,
	      int base, ocaml_exp_YYSTYPE *result)
{
  LONGEST val;
  const char *p = str;

  if (*p == '-')
    {
      p++;
      len--;
    }

  val = strtoll (p, NULL, base ? base : 10);

  if (*str == '-')
    val = -val;

  result->typed_val_int.val = val;
  result->typed_val_int.type = parse_ocaml_type (ps)->builtin_int;

  return INT;
}

/* Entry point for parsing OCaml expressions.  */

int
ocaml_parse (struct parser_state *par_state)
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate, par_state);
  gdb_assert (par_state != NULL);

  int result = yyparse ();
  if (!result)
    pstate->set_operation (pstate->pop ());
  return result;
}
