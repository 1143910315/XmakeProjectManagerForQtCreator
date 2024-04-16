#include "cmStandardLexer.h"

#define FLEXINT_H 1
#define  YY_INT_ALIGNED short int

/* A lexical scanner created by flex */

#define FLEX_SCANNER
#define YY_FLEX_MAJOR_VERSION 2
#define YY_FLEX_MINOR_VERSION 6
#define YY_FLEX_SUBMINOR_VERSION 4
#if YY_FLEX_SUBMINOR_VERSION > 0
#define FLEX_BETA
#endif

#ifdef yy_create_buffer
#define cmListFileLexer_yy_create_buffer_ALREADY_DEFINED
#else
#define yy_create_buffer cmListFileLexer_yy_create_buffer
#endif

#ifdef yy_delete_buffer
#define cmListFileLexer_yy_delete_buffer_ALREADY_DEFINED
#else
#define yy_delete_buffer cmListFileLexer_yy_delete_buffer
#endif

#ifdef yy_scan_buffer
#define cmListFileLexer_yy_scan_buffer_ALREADY_DEFINED
#else
#define yy_scan_buffer cmListFileLexer_yy_scan_buffer
#endif

#ifdef yy_scan_string
#define cmListFileLexer_yy_scan_string_ALREADY_DEFINED
#else
#define yy_scan_string cmListFileLexer_yy_scan_string
#endif

#ifdef yy_scan_bytes
#define cmListFileLexer_yy_scan_bytes_ALREADY_DEFINED
#else
#define yy_scan_bytes cmListFileLexer_yy_scan_bytes
#endif

#ifdef yy_init_buffer
#define cmListFileLexer_yy_init_buffer_ALREADY_DEFINED
#else
#define yy_init_buffer cmListFileLexer_yy_init_buffer
#endif

#ifdef yy_flush_buffer
#define cmListFileLexer_yy_flush_buffer_ALREADY_DEFINED
#else
#define yy_flush_buffer cmListFileLexer_yy_flush_buffer
#endif

#ifdef yy_load_buffer_state
#define cmListFileLexer_yy_load_buffer_state_ALREADY_DEFINED
#else
#define yy_load_buffer_state cmListFileLexer_yy_load_buffer_state
#endif

#ifdef yy_switch_to_buffer
#define cmListFileLexer_yy_switch_to_buffer_ALREADY_DEFINED
#else
#define yy_switch_to_buffer cmListFileLexer_yy_switch_to_buffer
#endif

#ifdef yypush_buffer_state
#define cmListFileLexer_yypush_buffer_state_ALREADY_DEFINED
#else
#define yypush_buffer_state cmListFileLexer_yypush_buffer_state
#endif

#ifdef yypop_buffer_state
#define cmListFileLexer_yypop_buffer_state_ALREADY_DEFINED
#else
#define yypop_buffer_state cmListFileLexer_yypop_buffer_state
#endif

#ifdef yyensure_buffer_stack
#define cmListFileLexer_yyensure_buffer_stack_ALREADY_DEFINED
#else
#define yyensure_buffer_stack cmListFileLexer_yyensure_buffer_stack
#endif

#ifdef yylex
#define cmListFileLexer_yylex_ALREADY_DEFINED
#else
#define yylex cmListFileLexer_yylex
#endif

#ifdef yyrestart
#define cmListFileLexer_yyrestart_ALREADY_DEFINED
#else
#define yyrestart cmListFileLexer_yyrestart
#endif

#ifdef yylex_init
#define cmListFileLexer_yylex_init_ALREADY_DEFINED
#else
#define yylex_init cmListFileLexer_yylex_init
#endif

#ifdef yylex_init_extra
#define cmListFileLexer_yylex_init_extra_ALREADY_DEFINED
#else
#define yylex_init_extra cmListFileLexer_yylex_init_extra
#endif

#ifdef yylex_destroy
#define cmListFileLexer_yylex_destroy_ALREADY_DEFINED
#else
#define yylex_destroy cmListFileLexer_yylex_destroy
#endif

#ifdef yyget_debug
#define cmListFileLexer_yyget_debug_ALREADY_DEFINED
#else
#define yyget_debug cmListFileLexer_yyget_debug
#endif

#ifdef yyset_debug
#define cmListFileLexer_yyset_debug_ALREADY_DEFINED
#else
#define yyset_debug cmListFileLexer_yyset_debug
#endif

#ifdef yyget_extra
#define cmListFileLexer_yyget_extra_ALREADY_DEFINED
#else
#define yyget_extra cmListFileLexer_yyget_extra
#endif

#ifdef yyset_extra
#define cmListFileLexer_yyset_extra_ALREADY_DEFINED
#else
#define yyset_extra cmListFileLexer_yyset_extra
#endif

#ifdef yyget_in
#define cmListFileLexer_yyget_in_ALREADY_DEFINED
#else
#define yyget_in cmListFileLexer_yyget_in
#endif

#ifdef yyset_in
#define cmListFileLexer_yyset_in_ALREADY_DEFINED
#else
#define yyset_in cmListFileLexer_yyset_in
#endif

#ifdef yyget_out
#define cmListFileLexer_yyget_out_ALREADY_DEFINED
#else
#define yyget_out cmListFileLexer_yyget_out
#endif

#ifdef yyset_out
#define cmListFileLexer_yyset_out_ALREADY_DEFINED
#else
#define yyset_out cmListFileLexer_yyset_out
#endif

#ifdef yyget_leng
#define cmListFileLexer_yyget_leng_ALREADY_DEFINED
#else
#define yyget_leng cmListFileLexer_yyget_leng
#endif

#ifdef yyget_text
#define cmListFileLexer_yyget_text_ALREADY_DEFINED
#else
#define yyget_text cmListFileLexer_yyget_text
#endif

#ifdef yyget_lineno
#define cmListFileLexer_yyget_lineno_ALREADY_DEFINED
#else
#define yyget_lineno cmListFileLexer_yyget_lineno
#endif

#ifdef yyset_lineno
#define cmListFileLexer_yyset_lineno_ALREADY_DEFINED
#else
#define yyset_lineno cmListFileLexer_yyset_lineno
#endif

#ifdef yyget_column
#define cmListFileLexer_yyget_column_ALREADY_DEFINED
#else
#define yyget_column cmListFileLexer_yyget_column
#endif

#ifdef yyset_column
#define cmListFileLexer_yyset_column_ALREADY_DEFINED
#else
#define yyset_column cmListFileLexer_yyset_column
#endif

#ifdef yywrap
#define cmListFileLexer_yywrap_ALREADY_DEFINED
#else
#define yywrap cmListFileLexer_yywrap
#endif

#ifdef yyalloc
#define cmListFileLexer_yyalloc_ALREADY_DEFINED
#else
#define yyalloc cmListFileLexer_yyalloc
#endif

#ifdef yyrealloc
#define cmListFileLexer_yyrealloc_ALREADY_DEFINED
#else
#define yyrealloc cmListFileLexer_yyrealloc
#endif

#ifdef yyfree
#define cmListFileLexer_yyfree_ALREADY_DEFINED
#else
#define yyfree cmListFileLexer_yyfree
#endif

/* First, we deal with  platform-specific or compiler-specific issues. */

/* begin standard C headers. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/* end standard C headers. */

/* flex integer type definitions */

#ifndef FLEXINT_H
#define FLEXINT_H

/* C99 systems have <inttypes.h>. Non-C99 systems may or may not. */

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

/* C99 says to define __STDC_LIMIT_MACROS before including stdint.h,
 * if you want the limit (max/min) macros for int types. 
 */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif

#include <inttypes.h>
typedef int8_t flex_int8_t;
typedef uint8_t flex_uint8_t;
typedef int16_t flex_int16_t;
typedef uint16_t flex_uint16_t;
typedef int32_t flex_int32_t;
typedef uint32_t flex_uint32_t;
#else
typedef signed char flex_int8_t;
typedef short int flex_int16_t;
typedef int flex_int32_t;
typedef unsigned char flex_uint8_t; 
typedef unsigned short int flex_uint16_t;
typedef unsigned int flex_uint32_t;

/* Limits of integral types. */
#ifndef INT8_MIN
#define INT8_MIN               (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN              (-32767-1)
#endif
#ifndef INT32_MIN
#define INT32_MIN              (-2147483647-1)
#endif
#ifndef INT8_MAX
#define INT8_MAX               (127)
#endif
#ifndef INT16_MAX
#define INT16_MAX              (32767)
#endif
#ifndef INT32_MAX
#define INT32_MAX              (2147483647)
#endif
#ifndef UINT8_MAX
#define UINT8_MAX              (255U)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX             (65535U)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

#ifndef SIZE_MAX
#define SIZE_MAX               (~(size_t)0)
#endif

#endif /* ! C99 */

#endif /* ! FLEXINT_H */

/* begin standard C++ headers. */

/* TODO: this is always defined, so inline it */
#define yyconst const

#if defined(__GNUC__) && __GNUC__ >= 3
#define yynoreturn __attribute__((__noreturn__))
#else
#define yynoreturn
#endif

/* Returned upon end-of-file. */
#define YY_NULL 0

/* Promotes a possibly negative, possibly signed char to an
 *   integer in range [0..255] for use as an array index.
 */
#define YY_SC_TO_UI(c) ((YY_CHAR) (c))

/* An opaque pointer. */
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

/* For convenience, these vars (plus the bison vars far below)
   are macros in the reentrant scanner. */
#define yyin yyg->yyin_r
#define yyout yyg->yyout_r
#define yyextra yyg->yyextra_r
#define yyleng yyg->yyleng_r
#define yytext yyg->yytext_r
#define yylineno (YY_CURRENT_BUFFER_LVALUE->yy_bs_lineno)
#define yycolumn (YY_CURRENT_BUFFER_LVALUE->yy_bs_column)
#define yy_flex_debug yyg->yy_flex_debug_r

/* Enter a start condition.  This macro really ought to take a parameter,
 * but we do it the disgusting crufty way forced on us by the ()-less
 * definition of BEGIN.
 */
#define BEGIN yyg->yy_start = 1 + 2 *
/* Translate the current start state into a value that can be later handed
 * to BEGIN to return to the state.  The YYSTATE alias is for lex
 * compatibility.
 */
#define YY_START ((yyg->yy_start - 1) / 2)
#define YYSTATE YY_START
/* Action number for EOF rule of a given start state. */
#define YY_STATE_EOF(state) (YY_END_OF_BUFFER + state + 1)
/* Special action meaning "start processing a new file". */
#define YY_NEW_FILE yyrestart( yyin , yyscanner )
#define YY_END_OF_BUFFER_CHAR 0

/* Size of default input buffer. */
#ifndef YY_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k.
 * Moreover, YY_BUF_SIZE is 2*YY_READ_BUF_SIZE in the general case.
 * Ditto for the __ia64__ case accordingly.
 */
#define YY_BUF_SIZE 32768
#else
#define YY_BUF_SIZE 16384
#endif /* __ia64__ */
#endif

/* The state buf must be large enough to hold one state per character in the main buffer.
 */
#define YY_STATE_BUF_SIZE   ((YY_BUF_SIZE + 2) * sizeof(yy_state_type))

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

#ifndef YY_TYPEDEF_YY_SIZE_T
#define YY_TYPEDEF_YY_SIZE_T
typedef size_t yy_size_t;
#endif

#define EOB_ACT_CONTINUE_SCAN 0
#define EOB_ACT_END_OF_FILE 1
#define EOB_ACT_LAST_MATCH 2
    
    /* Note: We specifically omit the test for yy_rule_can_match_eol because it requires
     *       access to the local variable yy_act. Since yyless() is a macro, it would break
     *       existing scanners that call yyless() from OUTSIDE yylex.
     *       One obvious solution it to make yy_act a global. I tried that, and saw
     *       a 5% performance hit in a non-yylineno scanner, because yy_act is
     *       normally declared as a register variable-- so it is not worth it.
     */
    #define  YY_LESS_LINENO(n) \
            do { \
                int yyl;\
                for ( yyl = n; yyl < yyleng; ++yyl )\
                    if ( yytext[yyl] == '\n' )\
                        --yylineno;\
            }while(0)
    #define YY_LINENO_REWIND_TO(dst) \
            do {\
                const char *p;\
                for ( p = yy_cp-1; p >= (dst); --p)\
                    if ( *p == '\n' )\
                        --yylineno;\
            }while(0)
    
/* Return all but the first "n" matched characters back to the input stream. */
#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up yytext. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		*yy_cp = yyg->yy_hold_char; \
		YY_RESTORE_YY_MORE_OFFSET \
		yyg->yy_c_buf_p = yy_cp = yy_bp + yyless_macro_arg - YY_MORE_ADJ; \
		YY_DO_BEFORE_ACTION; /* set up yytext again */ \
		} \
	while ( 0 )
#define unput(c) yyunput( c, yyg->yytext_ptr , yyscanner )

#ifndef YY_STRUCT_YY_BUFFER_STATE
#define YY_STRUCT_YY_BUFFER_STATE
struct yy_buffer_state
	{
	FILE *yy_input_file;

	char *yy_ch_buf;		/* input buffer */
	char *yy_buf_pos;		/* current position in input buffer */

	/* Size of input buffer in bytes, not including room for EOB
	 * characters.
	 */
	int yy_buf_size;

	/* Number of characters read into yy_ch_buf, not including EOB
	 * characters.
	 */
	int yy_n_chars;

	/* Whether we "own" the buffer - i.e., we know we created it,
	 * and can realloc() it to grow it, and should free() it to
	 * delete it.
	 */
	int yy_is_our_buffer;

	/* Whether this is an "interactive" input source; if so, and
	 * if we're using stdio for input, then we want to use getc()
	 * instead of fread(), to make sure we stop fetching input after
	 * each newline.
	 */
	int yy_is_interactive;

	/* Whether we're considered to be at the beginning of a line.
	 * If so, '^' rules will be active on the next match, otherwise
	 * not.
	 */
	int yy_at_bol;

    int yy_bs_lineno; /**< The line count. */
    int yy_bs_column; /**< The column count. */

	/* Whether to try to fill the input buffer when we reach the
	 * end of it.
	 */
	int yy_fill_buffer;

	int yy_buffer_status;

#define YY_BUFFER_NEW 0
#define YY_BUFFER_NORMAL 1
	/* When an EOF's been seen but there's still some text to process
	 * then we mark the buffer as YY_EOF_PENDING, to indicate that we
	 * shouldn't try reading from the input source any more.  We might
	 * still have a bunch of tokens to match, though, because of
	 * possible backing-up.
	 *
	 * When we actually see the EOF, we change the status to "new"
	 * (via yyrestart()), so that the user can continue scanning by
	 * just pointing yyin at a new input file.
	 */
#define YY_BUFFER_EOF_PENDING 2

	};
#endif /* !YY_STRUCT_YY_BUFFER_STATE */

/* We provide macros for accessing buffer states in case in the
 * future we want to put the buffer states in a more general
 * "scanner state".
 *
 * Returns the top of the stack, or NULL.
 */
#define YY_CURRENT_BUFFER ( yyg->yy_buffer_stack \
                          ? yyg->yy_buffer_stack[yyg->yy_buffer_stack_top] \
                          : NULL)
/* Same as previous macro, but useful when we know that the buffer stack is not
 * NULL or when we need an lvalue. For internal use only.
 */
#define YY_CURRENT_BUFFER_LVALUE yyg->yy_buffer_stack[yyg->yy_buffer_stack_top]

void yyrestart ( FILE *input_file , yyscan_t yyscanner );
void yy_switch_to_buffer ( YY_BUFFER_STATE new_buffer , yyscan_t yyscanner );
YY_BUFFER_STATE yy_create_buffer ( FILE *file, int size , yyscan_t yyscanner );
void yy_delete_buffer ( YY_BUFFER_STATE b , yyscan_t yyscanner );
void yy_flush_buffer ( YY_BUFFER_STATE b , yyscan_t yyscanner );
void yypush_buffer_state ( YY_BUFFER_STATE new_buffer , yyscan_t yyscanner );
void yypop_buffer_state ( yyscan_t yyscanner );

static void yyensure_buffer_stack ( yyscan_t yyscanner );
static void yy_load_buffer_state ( yyscan_t yyscanner );
static void yy_init_buffer ( YY_BUFFER_STATE b, FILE *file , yyscan_t yyscanner );
#define YY_FLUSH_BUFFER yy_flush_buffer( YY_CURRENT_BUFFER , yyscanner)

YY_BUFFER_STATE yy_scan_buffer ( char *base, yy_size_t size , yyscan_t yyscanner );
YY_BUFFER_STATE yy_scan_string ( const char *yy_str , yyscan_t yyscanner );
YY_BUFFER_STATE yy_scan_bytes ( const char *bytes, int len , yyscan_t yyscanner );

void *yyalloc ( yy_size_t , yyscan_t yyscanner );
void *yyrealloc ( void *, yy_size_t , yyscan_t yyscanner );
void yyfree ( void * , yyscan_t yyscanner );

#define yy_new_buffer yy_create_buffer
#define yy_set_interactive(is_interactive) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){ \
        yyensure_buffer_stack (yyscanner); \
		YY_CURRENT_BUFFER_LVALUE =    \
            yy_create_buffer( yyin, YY_BUF_SIZE , yyscanner); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_is_interactive = is_interactive; \
	}
#define yy_set_bol(at_bol) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){\
        yyensure_buffer_stack (yyscanner); \
		YY_CURRENT_BUFFER_LVALUE =    \
            yy_create_buffer( yyin, YY_BUF_SIZE , yyscanner); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_at_bol = at_bol; \
	}
#define YY_AT_BOL() (YY_CURRENT_BUFFER_LVALUE->yy_at_bol)

/* Begin user sect3 */

#define cmListFileLexer_yywrap(yyscanner) (/*CONSTCOND*/1)
#define YY_SKIP_YYWRAP
typedef flex_uint8_t YY_CHAR;

typedef int yy_state_type;

#define yytext_ptr yytext_r

static yy_state_type yy_get_previous_state ( yyscan_t yyscanner );
static yy_state_type yy_try_NUL_trans ( yy_state_type current_state  , yyscan_t yyscanner);
static int yy_get_next_buffer ( yyscan_t yyscanner );
static void yynoreturn yy_fatal_error ( const char* msg , yyscan_t yyscanner );

/* Done after the current pattern has been matched and before the
 * corresponding action - sets up yytext.
 */
#define YY_DO_BEFORE_ACTION \
	yyg->yytext_ptr = yy_bp; \
	yyleng = (int) (yy_cp - yy_bp); \
	yyg->yy_hold_char = *yy_cp; \
	*yy_cp = '\0'; \
	yyg->yy_c_buf_p = yy_cp;
#define YY_NUM_RULES 24
#define YY_END_OF_BUFFER 25
/* This struct is not used in this scanner,
   but its presence is necessary. */
struct yy_trans_info
	{
	flex_int32_t yy_verify;
	flex_int32_t yy_nxt;
	};
static const flex_int16_t yy_accept[79] =
    {   0,
        0,    0,    0,    0,    0,    0,    0,    0,    4,    4,
       25,   13,   22,    1,   16,    3,   13,    5,    6,    7,
       15,   23,   23,   17,   19,   20,   21,   24,   10,   11,
        8,   12,    9,    4,   13,    0,   13,    0,   22,    0,
        0,    7,   13,    0,   13,    0,    2,    0,   13,   17,
        0,   18,   10,    8,    4,    0,   14,    0,    0,    0,
        0,   14,    0,    0,   14,    0,    0,    0,    2,   14,
        0,    0,    0,    0,    0,    0,    0,    0
    } ;

static const YY_CHAR yy_ec[256] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    2,    3,
        1,    1,    4,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    2,    1,    5,    6,    7,    1,    1,    1,    8,
        9,    1,    1,    1,    1,    1,    1,   10,   10,   10,
       10,   10,   10,   10,   10,   10,   10,    1,    1,    1,
       11,    1,    1,    1,   12,   12,   12,   12,   12,   12,
       12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
       12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
       13,   14,   15,    1,   12,    1,   12,   12,   12,   12,

       12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
       12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
       12,   12,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,

        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1
    } ;

static const YY_CHAR yy_meta[17] =
    {   0,
        1,    1,    2,    3,    4,    3,    1,    3,    5,    6,
        1,    6,    1,    1,    7,    2
    } ;

static const flex_int16_t yy_base[97] =
    {   0,
        0,    0,   14,   28,   42,   56,   70,   84,   18,   19,
       68,  100,   16,  298,  298,   54,   58,  298,  298,   13,
      115,    0,  298,   51,  298,  298,   21,  298,    0,  298,
       53,  298,  298,    0,    0,  126,   55,    0,   25,   25,
       53,    0,    0,  136,   53,    0,   57,    0,    0,   42,
       50,  298,    0,   43,    0,  146,  160,   45,  172,   43,
       26,    0,   42,  177,    0,   42,  188,   40,  298,   40,
        0,   38,   37,   34,   32,   31,   23,  298,  197,  204,
      211,  218,  225,  232,  239,  245,  252,  259,  262,  268,
      275,  278,  280,  286,  289,  291

    } ;

static const flex_int16_t yy_def[97] =
    {   0,
       78,    1,   79,   79,   80,   80,   81,   81,   82,   82,
       78,   78,   78,   78,   78,   78,   12,   78,   78,   12,
       78,   83,   78,   84,   78,   78,   84,   78,   85,   78,
       78,   78,   78,   86,   12,   87,   12,   88,   78,   78,
       89,   20,   12,   90,   12,   21,   78,   91,   12,   84,
       84,   78,   85,   78,   86,   87,   78,   56,   87,   92,
       78,   57,   89,   90,   57,   64,   90,   93,   78,   57,
       94,   95,   92,   96,   93,   95,   96,    0,   78,   78,
       78,   78,   78,   78,   78,   78,   78,   78,   78,   78,
       78,   78,   78,   78,   78,   78

    } ;

static const flex_int16_t yy_nxt[315] =
    {   0,
       12,   13,   14,   13,   15,   16,   17,   18,   19,   12,
       12,   20,   21,   22,   12,   23,   25,   39,   26,   39,
       14,   14,   42,   52,   42,   50,   39,   27,   39,   28,
       25,   64,   26,   28,   28,   61,   61,   47,   47,   56,
       65,   27,   64,   28,   30,   57,   56,   60,   65,   74,
       62,   57,   72,   54,   50,   51,   31,   28,   30,   69,
       68,   62,   60,   54,   51,   41,   40,   78,   78,   78,
       31,   28,   30,   78,   78,   78,   78,   78,   78,   78,
       78,   78,   78,   78,   33,   28,   30,   78,   78,   78,
       78,   78,   78,   78,   78,   78,   78,   78,   33,   28,

       35,   78,   78,   78,   36,   78,   37,   78,   78,   35,
       35,   35,   35,   38,   35,   43,   78,   78,   78,   44,
       78,   45,   78,   78,   43,   46,   43,   47,   48,   43,
       57,   78,   58,   78,   78,   78,   78,   78,   78,   59,
       65,   78,   66,   78,   78,   78,   78,   78,   78,   67,
       57,   78,   58,   78,   78,   78,   78,   78,   78,   59,
       57,   78,   78,   78,   36,   78,   70,   78,   78,   57,
       57,   57,   57,   71,   57,   56,   78,   56,   78,   56,
       56,   65,   78,   66,   78,   78,   78,   78,   78,   78,
       67,   64,   78,   64,   78,   64,   64,   24,   24,   24,

       24,   24,   24,   24,   29,   29,   29,   29,   29,   29,
       29,   32,   32,   32,   32,   32,   32,   32,   34,   34,
       34,   34,   34,   34,   34,   49,   78,   49,   49,   49,
       49,   49,   50,   78,   50,   78,   50,   50,   50,   53,
       78,   53,   53,   53,   53,   55,   78,   55,   55,   55,
       55,   55,   56,   78,   78,   56,   78,   56,   56,   35,
       78,   35,   35,   35,   35,   35,   63,   63,   64,   78,
       78,   64,   78,   64,   64,   43,   78,   43,   43,   43,
       43,   43,   73,   73,   75,   75,   57,   78,   57,   57,
       57,   57,   57,   76,   76,   77,   77,   11,   78,   78,

       78,   78,   78,   78,   78,   78,   78,   78,   78,   78,
       78,   78,   78,   78
    } ;

static const flex_int16_t yy_chk[315] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    3,   13,    3,   13,
        9,   10,   20,   27,   20,   27,   39,    3,   39,    3,
        4,   77,    4,    9,   10,   40,   61,   40,   61,   76,
       75,    4,   74,    4,    5,   73,   72,   70,   68,   66,
       63,   60,   58,   54,   51,   50,    5,    5,    6,   47,
       45,   41,   37,   31,   24,   17,   16,   11,    0,    0,
        6,    6,    7,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    7,    7,    8,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    8,    8,

       12,    0,    0,    0,   12,    0,   12,    0,    0,   12,
       12,   12,   12,   12,   12,   21,    0,    0,    0,   21,
        0,   21,    0,    0,   21,   21,   21,   21,   21,   21,
       36,    0,   36,    0,    0,    0,    0,    0,    0,   36,
       44,    0,   44,    0,    0,    0,    0,    0,    0,   44,
       56,    0,   56,    0,    0,    0,    0,    0,    0,   56,
       57,    0,    0,    0,   57,    0,   57,    0,    0,   57,
       57,   57,   57,   57,   57,   59,    0,   59,    0,   59,
       59,   64,    0,   64,    0,    0,    0,    0,    0,    0,
       64,   67,    0,   67,    0,   67,   67,   79,   79,   79,

       79,   79,   79,   79,   80,   80,   80,   80,   80,   80,
       80,   81,   81,   81,   81,   81,   81,   81,   82,   82,
       82,   82,   82,   82,   82,   83,    0,   83,   83,   83,
       83,   83,   84,    0,   84,    0,   84,   84,   84,   85,
        0,   85,   85,   85,   85,   86,    0,   86,   86,   86,
       86,   86,   87,    0,    0,   87,    0,   87,   87,   88,
        0,   88,   88,   88,   88,   88,   89,   89,   90,    0,
        0,   90,    0,   90,   90,   91,    0,   91,   91,   91,
       91,   91,   92,   92,   93,   93,   94,    0,   94,   94,
       94,   94,   94,   95,   95,   96,   96,   78,   78,   78,

       78,   78,   78,   78,   78,   78,   78,   78,   78,   78,
       78,   78,   78,   78
    } ;

/* Table of booleans, true if rule could match eol. */
static const flex_int32_t yy_rule_can_match_eol[25] =
    {   0,
1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 
    0, 0, 0, 0, 0,     };

/* The intent behind this definition is that it'll catch
 * any uses of REJECT which flex missed.
 */
#define REJECT reject_used_but_not_detected
#define yymore() yymore_used_but_not_detected
#define YY_MORE_ADJ 0
#define YY_RESTORE_YY_MORE_OFFSET
/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
/*

This file must be translated to C and modified to build everywhere.

Run flex >= 2.6 like this:

  flex --nounistd -DFLEXINT_H --noline -ocmListFileLexer.cxx cmListFileLexer.in.l

Modify cmListFileLexer.c:
  - remove trailing whitespace:              sed -i 's/\s*$//' cmListFileLexer.c
  - remove blank lines at end of file:       sed -i '${/^$/d;}' cmListFileLexer.c
  - #include "cmStandardLexer.h" at the top: sed -i '1i#include "cmStandardLexer.h"' cmListFileLexer.c

*/

/* IWYU pragma: no_forward_declare yyguts_t */

/* Setup the proper cmListFileLexer_yylex declaration.  */
#define YY_EXTRA_TYPE cmListFileLexer*
#define YY_DECL int cmListFileLexer_yylex (yyscan_t yyscanner, cmListFileLexer* lexer)

#include "cmListFileLexer.h"

/*--------------------------------------------------------------------------*/
struct cmListFileLexer_s
{
  cmListFileLexer_Token token;
  int bracket;
  int comment;
  int line;
  int column;
  int size;
  FILE* file;
  size_t cr;
  char* string_buffer;
  char* string_position;
  int string_left;
  yyscan_t scanner;
};

static void cmListFileLexerSetToken(cmListFileLexer* lexer, const char* text,
                                    int length);
static void cmListFileLexerAppend(cmListFileLexer* lexer, const char* text,
                                  int length);
static int cmListFileLexerInput(cmListFileLexer* lexer, char* buffer,
                                size_t bufferSize);
static void cmListFileLexerInit(cmListFileLexer* lexer);
static void cmListFileLexerDestroy(cmListFileLexer* lexer);

/* Replace the lexer input function.  */
#undef YY_INPUT
#define YY_INPUT(buf, result, max_size) \
  do { result = cmListFileLexerInput(cmListFileLexer_yyget_extra(yyscanner), buf, max_size); } while (0)

/*--------------------------------------------------------------------------*/

#define INITIAL 0
#define STRING 1
#define BRACKET 2
#define BRACKETEND 3
#define COMMENT 4

#ifndef YY_EXTRA_TYPE
#define YY_EXTRA_TYPE void *
#endif

/* Holds the entire state of the reentrant scanner. */
struct yyguts_t
    {

    /* User-defined. Not touched by flex. */
    YY_EXTRA_TYPE yyextra_r;

    /* The rest are the same as the globals declared in the non-reentrant scanner. */
    FILE *yyin_r, *yyout_r;
    size_t yy_buffer_stack_top; /**< index of top of stack. */
    size_t yy_buffer_stack_max; /**< capacity of stack. */
    YY_BUFFER_STATE * yy_buffer_stack; /**< Stack as an array. */
    char yy_hold_char;
    int yy_n_chars;
    int yyleng_r;
    char *yy_c_buf_p;
    int yy_init;
    int yy_start;
    int yy_did_buffer_switch_on_eof;
    int yy_start_stack_ptr;
    int yy_start_stack_depth;
    int *yy_start_stack;
    yy_state_type yy_last_accepting_state;
    char* yy_last_accepting_cpos;

    int yylineno_r;
    int yy_flex_debug_r;

    char *yytext_r;
    int yy_more_flag;
    int yy_more_len;

    }; /* end struct yyguts_t */

static int yy_init_globals ( yyscan_t yyscanner );

int yylex_init (yyscan_t* scanner);

int yylex_init_extra ( YY_EXTRA_TYPE user_defined, yyscan_t* scanner);

/* Accessor methods to globals.
   These are made visible to non-reentrant scanners for convenience. */

int yylex_destroy ( yyscan_t yyscanner );

int yyget_debug ( yyscan_t yyscanner );

void yyset_debug ( int debug_flag , yyscan_t yyscanner );

YY_EXTRA_TYPE yyget_extra ( yyscan_t yyscanner );

void yyset_extra ( YY_EXTRA_TYPE user_defined , yyscan_t yyscanner );

FILE *yyget_in ( yyscan_t yyscanner );

void yyset_in  ( FILE * _in_str , yyscan_t yyscanner );

FILE *yyget_out ( yyscan_t yyscanner );

void yyset_out  ( FILE * _out_str , yyscan_t yyscanner );

			int yyget_leng ( yyscan_t yyscanner );

char *yyget_text ( yyscan_t yyscanner );

int yyget_lineno ( yyscan_t yyscanner );

void yyset_lineno ( int _line_number , yyscan_t yyscanner );

int yyget_column  ( yyscan_t yyscanner );

void yyset_column ( int _column_no , yyscan_t yyscanner );

/* Macros after this point can all be overridden by user definitions in
 * section 1.
 */

#ifndef YY_SKIP_YYWRAP
#ifdef __cplusplus
extern "C" int yywrap ( yyscan_t yyscanner );
#else
extern int yywrap ( yyscan_t yyscanner );
#endif
#endif

#ifndef YY_NO_UNPUT
    
    static void yyunput ( int c, char *buf_ptr  , yyscan_t yyscanner);
    
#endif

#ifndef yytext_ptr
static void yy_flex_strncpy ( char *, const char *, int , yyscan_t yyscanner);
#endif

#ifdef YY_NEED_STRLEN
static int yy_flex_strlen ( const char * , yyscan_t yyscanner);
#endif

#ifndef YY_NO_INPUT
#ifdef __cplusplus
static int yyinput ( yyscan_t yyscanner );
#else
static int input ( yyscan_t yyscanner );
#endif

#endif

/* Amount of stuff to slurp up with each read. */
#ifndef YY_READ_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k */
#define YY_READ_BUF_SIZE 16384
#else
#define YY_READ_BUF_SIZE 8192
#endif /* __ia64__ */
#endif

/* Copy whatever the last rule matched to the standard output. */
#ifndef ECHO
/* This used to be an fputs(), but since the string might contain NUL's,
 * we now use fwrite().
 */
#define ECHO do { if (fwrite( yytext, (size_t) yyleng, 1, yyout )) {} } while (0)
#endif

/* Gets input and stuffs it into "buf".  number of characters read, or YY_NULL,
 * is returned in "result".
 */
#ifndef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( YY_CURRENT_BUFFER_LVALUE->yy_is_interactive ) \
		{ \
		int c = '*'; \
		int n; \
		for ( n = 0; n < max_size && \
			     (c = getc( yyin )) != EOF && c != '\n'; ++n ) \
			buf[n] = (char) c; \
		if ( c == '\n' ) \
			buf[n++] = (char) c; \
		if ( c == EOF && ferror( yyin ) ) \
			YY_FATAL_ERROR( "input in flex scanner failed" ); \
		result = n; \
		} \
	else \
		{ \
		errno=0; \
		while ( (result = (int) fread(buf, 1, (yy_size_t) max_size, yyin)) == 0 && ferror(yyin)) \
			{ \
			if( errno != EINTR) \
				{ \
				YY_FATAL_ERROR( "input in flex scanner failed" ); \
				break; \
				} \
			errno=0; \
			clearerr(yyin); \
			} \
		}\
\

#endif

/* No semi-colon after return; correct usage is to write "yyterminate();" -
 * we don't want an extra ';' after the "return" because that will cause
 * some compilers to complain about unreachable statements.
 */
#ifndef yyterminate
#define yyterminate() return YY_NULL
#endif

/* Number of entries by which start-condition stack grows. */
#ifndef YY_START_STACK_INCR
#define YY_START_STACK_INCR 25
#endif

/* Report a fatal error. */
#ifndef YY_FATAL_ERROR
#define YY_FATAL_ERROR(msg) yy_fatal_error( msg , yyscanner)
#endif

/* end tables serialization structures and prototypes */

/* Default declaration of generated scanner - a define so the user can
 * easily add parameters.
 */
#ifndef YY_DECL
#define YY_DECL_IS_OURS 1

extern int yylex (yyscan_t yyscanner);

#define YY_DECL int yylex (yyscan_t yyscanner)
#endif /* !YY_DECL */

/* Code executed at the beginning of each rule, after yytext and yyleng
 * have been set up.
 */
#ifndef YY_USER_ACTION
#define YY_USER_ACTION
#endif

/* Code executed at the end of each rule. */
#ifndef YY_BREAK
#define YY_BREAK /*LINTED*/break;
#endif

#define YY_RULE_SETUP \
	YY_USER_ACTION

/** The main scanner function which does all the work.
 */
YY_DECL
{
	yy_state_type yy_current_state;
	char *yy_cp, *yy_bp;
	int yy_act;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	if ( !yyg->yy_init )
		{
		yyg->yy_init = 1;

#ifdef YY_USER_INIT
		YY_USER_INIT;
#endif

		if ( ! yyg->yy_start )
			yyg->yy_start = 1;	/* first start state */

		if ( ! yyin )
			yyin = stdin;

		if ( ! yyout )
			yyout = stdout;

		if ( ! YY_CURRENT_BUFFER ) {
			yyensure_buffer_stack (yyscanner);
			YY_CURRENT_BUFFER_LVALUE =
				yy_create_buffer( yyin, YY_BUF_SIZE , yyscanner);
		}

		yy_load_buffer_state( yyscanner );
		}

	{

	while ( /*CONSTCOND*/1 )		/* loops until end-of-file is reached */
		{
		yy_cp = yyg->yy_c_buf_p;

		/* Support of yytext. */
		*yy_cp = yyg->yy_hold_char;

		/* yy_bp points to the position in yy_ch_buf of the start of
		 * the current run.
		 */
		yy_bp = yy_cp;

		yy_current_state = yyg->yy_start;
yy_match:
		do
			{
			YY_CHAR yy_c = yy_ec[YY_SC_TO_UI(*yy_cp)] ;
			if ( yy_accept[yy_current_state] )
				{
				yyg->yy_last_accepting_state = yy_current_state;
				yyg->yy_last_accepting_cpos = yy_cp;
				}
			while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
				{
				yy_current_state = (int) yy_def[yy_current_state];
				if ( yy_current_state >= 79 )
					yy_c = yy_meta[yy_c];
				}
			yy_current_state = yy_nxt[yy_base[yy_current_state] + yy_c];
			++yy_cp;
			}
		while ( yy_base[yy_current_state] != 298 );

yy_find_action:
		yy_act = yy_accept[yy_current_state];
		if ( yy_act == 0 )
			{ /* have to back up */
			yy_cp = yyg->yy_last_accepting_cpos;
			yy_current_state = yyg->yy_last_accepting_state;
			yy_act = yy_accept[yy_current_state];
			}

		YY_DO_BEFORE_ACTION;

		if ( yy_act != YY_END_OF_BUFFER && yy_rule_can_match_eol[yy_act] )
			{
			int yyl;
			for ( yyl = 0; yyl < yyleng; ++yyl )
				if ( yytext[yyl] == '\n' )
					
    do{ yylineno++;
        yycolumn=0;
    }while(0)
;
			}

do_action:	/* This label is used only to access EOF actions. */

		switch ( yy_act )
	{ /* beginning of action switch */
			case 0: /* must back up */
			/* undo the effects of YY_DO_BEFORE_ACTION */
			*yy_cp = yyg->yy_hold_char;
			yy_cp = yyg->yy_last_accepting_cpos;
			yy_current_state = yyg->yy_last_accepting_state;
			goto yy_find_action;

case 1:
/* rule 1 can match eol */
YY_RULE_SETUP
{
  lexer->token.type = cmListFileLexer_Token_Newline;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  ++lexer->line;
  lexer->column = 1;
  BEGIN(INITIAL);
  return 1;
}
	YY_BREAK
case 2:
/* rule 2 can match eol */
YY_RULE_SETUP
{
  const char* bracket = yytext;
  lexer->comment = yytext[0] == '#';
  if (lexer->comment) {
    lexer->token.type = cmListFileLexer_Token_CommentBracket;
    bracket += 1;
  } else {
    lexer->token.type = cmListFileLexer_Token_ArgumentBracket;
  }
  cmListFileLexerSetToken(lexer, "", 0);
  lexer->bracket = strchr(bracket+1, '[') - bracket;
  if (yytext[yyleng-1] == '\n') {
    ++lexer->line;
    lexer->column = 1;
  } else {
    lexer->column += yyleng;
  }
  BEGIN(BRACKET);
}
	YY_BREAK
case 3:
YY_RULE_SETUP
{
  lexer->column += yyleng;
  BEGIN(COMMENT);
}
	YY_BREAK
case 4:
YY_RULE_SETUP
{
  lexer->column += yyleng;
}
	YY_BREAK
case 5:
YY_RULE_SETUP
{
  lexer->token.type = cmListFileLexer_Token_ParenLeft;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}
	YY_BREAK
case 6:
YY_RULE_SETUP
{
  lexer->token.type = cmListFileLexer_Token_ParenRight;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}
	YY_BREAK
case 7:
YY_RULE_SETUP
{
  lexer->token.type = cmListFileLexer_Token_Identifier;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}
	YY_BREAK
case 8:
YY_RULE_SETUP
{
  /* Handle ]]====]=======]*/
  cmListFileLexerAppend(lexer, yytext, yyleng);
  lexer->column += yyleng;
  if (yyleng == lexer->bracket) {
    BEGIN(BRACKETEND);
  }
}
	YY_BREAK
case 9:
YY_RULE_SETUP
{
  lexer->column += yyleng;
  /* Erase the partial bracket from the token.  */
  lexer->token.length -= lexer->bracket;
  lexer->token.text[lexer->token.length] = 0;
  BEGIN(INITIAL);
  return 1;
}
	YY_BREAK
case 10:
YY_RULE_SETUP
{
  cmListFileLexerAppend(lexer, yytext, yyleng);
  lexer->column += yyleng;
}
	YY_BREAK
case 11:
/* rule 11 can match eol */
YY_RULE_SETUP
{
  cmListFileLexerAppend(lexer, yytext, yyleng);
  ++lexer->line;
  lexer->column = 1;
  BEGIN(BRACKET);
}
	YY_BREAK
case 12:
YY_RULE_SETUP
{
  cmListFileLexerAppend(lexer, yytext, yyleng);
  lexer->column += yyleng;
  BEGIN(BRACKET);
}
	YY_BREAK
case YY_STATE_EOF(BRACKET):
case YY_STATE_EOF(BRACKETEND):
{
  lexer->token.type = cmListFileLexer_Token_BadBracket;
  BEGIN(INITIAL);
  return 1;
}
	YY_BREAK
case 13:
YY_RULE_SETUP
{
  lexer->token.type = cmListFileLexer_Token_ArgumentUnquoted;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}
	YY_BREAK
case 14:
YY_RULE_SETUP
{
  lexer->token.type = cmListFileLexer_Token_ArgumentUnquoted;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}
	YY_BREAK
case 15:
YY_RULE_SETUP
{
  lexer->token.type = cmListFileLexer_Token_ArgumentUnquoted;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}
	YY_BREAK
case 16:
YY_RULE_SETUP
{
  lexer->token.type = cmListFileLexer_Token_ArgumentQuoted;
  cmListFileLexerSetToken(lexer, "", 0);
  lexer->column += yyleng;
  BEGIN(STRING);
}
	YY_BREAK
case 17:
YY_RULE_SETUP
{
  cmListFileLexerAppend(lexer, yytext, yyleng);
  lexer->column += yyleng;
}
	YY_BREAK
case 18:
/* rule 18 can match eol */
YY_RULE_SETUP
{
  /* Continuation: text is not part of string */
  ++lexer->line;
  lexer->column = 1;
}
	YY_BREAK
case 19:
/* rule 19 can match eol */
YY_RULE_SETUP
{
  cmListFileLexerAppend(lexer, yytext, yyleng);
  ++lexer->line;
  lexer->column = 1;
}
	YY_BREAK
case 20:
YY_RULE_SETUP
{
  lexer->column += yyleng;
  BEGIN(INITIAL);
  return 1;
}
	YY_BREAK
case 21:
YY_RULE_SETUP
{
  cmListFileLexerAppend(lexer, yytext, yyleng);
  lexer->column += yyleng;
}
	YY_BREAK
case YY_STATE_EOF(STRING):
{
  lexer->token.type = cmListFileLexer_Token_BadString;
  BEGIN(INITIAL);
  return 1;
}
	YY_BREAK
case 22:
YY_RULE_SETUP
{
  lexer->token.type = cmListFileLexer_Token_Space;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}
	YY_BREAK
case 23:
YY_RULE_SETUP
{
  lexer->token.type = cmListFileLexer_Token_BadCharacter;
  cmListFileLexerSetToken(lexer, yytext, yyleng);
  lexer->column += yyleng;
  return 1;
}
	YY_BREAK
case YY_STATE_EOF(INITIAL):
case YY_STATE_EOF(COMMENT):
{
  lexer->token.type = cmListFileLexer_Token_None;
  cmListFileLexerSetToken(lexer, 0, 0);
  return 0;
}
	YY_BREAK
case 24:
YY_RULE_SETUP
ECHO;
	YY_BREAK

	case YY_END_OF_BUFFER:
		{
		/* Amount of text matched not including the EOB char. */
		int yy_amount_of_matched_text = (int) (yy_cp - yyg->yytext_ptr) - 1;

		/* Undo the effects of YY_DO_BEFORE_ACTION. */
		*yy_cp = yyg->yy_hold_char;
		YY_RESTORE_YY_MORE_OFFSET

		if ( YY_CURRENT_BUFFER_LVALUE->yy_buffer_status == YY_BUFFER_NEW )
			{
			/* We're scanning a new file or input source.  It's
			 * possible that this happened because the user
			 * just pointed yyin at a new source and called
			 * yylex().  If so, then we have to assure
			 * consistency between YY_CURRENT_BUFFER and our
			 * globals.  Here is the right place to do so, because
			 * this is the first action (other than possibly a
			 * back-up) that will match for the new input source.
			 */
			yyg->yy_n_chars = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
			YY_CURRENT_BUFFER_LVALUE->yy_input_file = yyin;
			YY_CURRENT_BUFFER_LVALUE->yy_buffer_status = YY_BUFFER_NORMAL;
			}

		/* Note that here we test for yy_c_buf_p "<=" to the position
		 * of the first EOB in the buffer, since yy_c_buf_p will
		 * already have been incremented past the NUL character
		 * (since all states make transitions on EOB to the
		 * end-of-buffer state).  Contrast this with the test
		 * in input().
		 */
		if ( yyg->yy_c_buf_p <= &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[yyg->yy_n_chars] )
			{ /* This was really a NUL. */
			yy_state_type yy_next_state;

			yyg->yy_c_buf_p = yyg->yytext_ptr + yy_amount_of_matched_text;

			yy_current_state = yy_get_previous_state( yyscanner );

			/* Okay, we're now positioned to make the NUL
			 * transition.  We couldn't have
			 * yy_get_previous_state() go ahead and do it
			 * for us because it doesn't know how to deal
			 * with the possibility of jamming (and we don't
			 * want to build jamming into it because then it
			 * will run more slowly).
			 */

			yy_next_state = yy_try_NUL_trans( yy_current_state , yyscanner);

			yy_bp = yyg->yytext_ptr + YY_MORE_ADJ;

			if ( yy_next_state )
				{
				/* Consume the NUL. */
				yy_cp = ++yyg->yy_c_buf_p;
				yy_current_state = yy_next_state;
				goto yy_match;
				}

			else
				{
				yy_cp = yyg->yy_c_buf_p;
				goto yy_find_action;
				}
			}

		else switch ( yy_get_next_buffer( yyscanner ) )
			{
			case EOB_ACT_END_OF_FILE:
				{
				yyg->yy_did_buffer_switch_on_eof = 0;

				if ( yywrap( yyscanner ) )
					{
					/* Note: because we've taken care in
					 * yy_get_next_buffer() to have set up
					 * yytext, we can now set up
					 * yy_c_buf_p so that if some total
					 * hoser (like flex itself) wants to
					 * call the scanner after we return the
					 * YY_NULL, it'll still work - another
					 * YY_NULL will get returned.
					 */
					yyg->yy_c_buf_p = yyg->yytext_ptr + YY_MORE_ADJ;

					yy_act = YY_STATE_EOF(YY_START);
					goto do_action;
					}

				else
					{
					if ( ! yyg->yy_did_buffer_switch_on_eof )
						YY_NEW_FILE;
					}
				break;
				}

			case EOB_ACT_CONTINUE_SCAN:
				yyg->yy_c_buf_p =
					yyg->yytext_ptr + yy_amount_of_matched_text;

				yy_current_state = yy_get_previous_state( yyscanner );

				yy_cp = yyg->yy_c_buf_p;
				yy_bp = yyg->yytext_ptr + YY_MORE_ADJ;
				goto yy_match;

			case EOB_ACT_LAST_MATCH:
				yyg->yy_c_buf_p =
				&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[yyg->yy_n_chars];

				yy_current_state = yy_get_previous_state( yyscanner );

				yy_cp = yyg->yy_c_buf_p;
				yy_bp = yyg->yytext_ptr + YY_MORE_ADJ;
				goto yy_find_action;
			}
		break;
		}

	default:
		YY_FATAL_ERROR(
			"fatal flex scanner internal error--no action found" );
	} /* end of action switch */
		} /* end of scanning one token */
	} /* end of user's declarations */
} /* end of yylex */

/* yy_get_next_buffer - try to read in a new buffer
 *
 * Returns a code representing an action:
 *	EOB_ACT_LAST_MATCH -
 *	EOB_ACT_CONTINUE_SCAN - continue scanning from current position
 *	EOB_ACT_END_OF_FILE - end of file
 */
static int yy_get_next_buffer (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	char *dest = YY_CURRENT_BUFFER_LVALUE->yy_ch_buf;
	char *source = yyg->yytext_ptr;
	int number_to_move, i;
	int ret_val;

	if ( yyg->yy_c_buf_p > &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[yyg->yy_n_chars + 1] )
		YY_FATAL_ERROR(
		"fatal flex scanner internal error--end of buffer missed" );

	if ( YY_CURRENT_BUFFER_LVALUE->yy_fill_buffer == 0 )
		{ /* Don't try to fill the buffer, so this is an EOF. */
		if ( yyg->yy_c_buf_p - yyg->yytext_ptr - YY_MORE_ADJ == 1 )
			{
			/* We matched a single character, the EOB, so
			 * treat this as a final EOF.
			 */
			return EOB_ACT_END_OF_FILE;
			}

		else
			{
			/* We matched some text prior to the EOB, first
			 * process it.
			 */
			return EOB_ACT_LAST_MATCH;
			}
		}

	/* Try to read more data. */

	/* First move last chars to start of buffer. */
	number_to_move = (int) (yyg->yy_c_buf_p - yyg->yytext_ptr - 1);

	for ( i = 0; i < number_to_move; ++i )
		*(dest++) = *(source++);

	if ( YY_CURRENT_BUFFER_LVALUE->yy_buffer_status == YY_BUFFER_EOF_PENDING )
		/* don't do the read, it's not guaranteed to return an EOF,
		 * just force an EOF
		 */
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yyg->yy_n_chars = 0;

	else
		{
			int num_to_read =
			YY_CURRENT_BUFFER_LVALUE->yy_buf_size - number_to_move - 1;

		while ( num_to_read <= 0 )
			{ /* Not enough room in the buffer - grow it. */

			/* just a shorter name for the current buffer */
			YY_BUFFER_STATE b = YY_CURRENT_BUFFER_LVALUE;

			int yy_c_buf_p_offset =
				(int) (yyg->yy_c_buf_p - b->yy_ch_buf);

			if ( b->yy_is_our_buffer )
				{
				int new_size = b->yy_buf_size * 2;

				if ( new_size <= 0 )
					b->yy_buf_size += b->yy_buf_size / 8;
				else
					b->yy_buf_size *= 2;

				b->yy_ch_buf = (char *)
					/* Include room in for 2 EOB chars. */
					yyrealloc( (void *) b->yy_ch_buf,
							 (yy_size_t) (b->yy_buf_size + 2) , yyscanner );
				}
			else
				/* Can't grow it, we don't own it. */
				b->yy_ch_buf = NULL;

			if ( ! b->yy_ch_buf )
				YY_FATAL_ERROR(
				"fatal error - scanner input buffer overflow" );

			yyg->yy_c_buf_p = &b->yy_ch_buf[yy_c_buf_p_offset];

			num_to_read = YY_CURRENT_BUFFER_LVALUE->yy_buf_size -
						number_to_move - 1;

			}

		if ( num_to_read > YY_READ_BUF_SIZE )
			num_to_read = YY_READ_BUF_SIZE;

		/* Read in more data. */
		YY_INPUT( (&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[number_to_move]),
			yyg->yy_n_chars, num_to_read );

		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yyg->yy_n_chars;
		}

	if ( yyg->yy_n_chars == 0 )
		{
		if ( number_to_move == YY_MORE_ADJ )
			{
			ret_val = EOB_ACT_END_OF_FILE;
			yyrestart( yyin  , yyscanner);
			}

		else
			{
			ret_val = EOB_ACT_LAST_MATCH;
			YY_CURRENT_BUFFER_LVALUE->yy_buffer_status =
				YY_BUFFER_EOF_PENDING;
			}
		}

	else
		ret_val = EOB_ACT_CONTINUE_SCAN;

	if ((yyg->yy_n_chars + number_to_move) > YY_CURRENT_BUFFER_LVALUE->yy_buf_size) {
		/* Extend the array by 50%, plus the number we really need. */
		int new_size = yyg->yy_n_chars + number_to_move + (yyg->yy_n_chars >> 1);
		YY_CURRENT_BUFFER_LVALUE->yy_ch_buf = (char *) yyrealloc(
			(void *) YY_CURRENT_BUFFER_LVALUE->yy_ch_buf, (yy_size_t) new_size , yyscanner );
		if ( ! YY_CURRENT_BUFFER_LVALUE->yy_ch_buf )
			YY_FATAL_ERROR( "out of dynamic memory in yy_get_next_buffer()" );
		/* "- 2" to take care of EOB's */
		YY_CURRENT_BUFFER_LVALUE->yy_buf_size = (int) (new_size - 2);
	}

	yyg->yy_n_chars += number_to_move;
	YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[yyg->yy_n_chars] = YY_END_OF_BUFFER_CHAR;
	YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[yyg->yy_n_chars + 1] = YY_END_OF_BUFFER_CHAR;

	yyg->yytext_ptr = &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[0];

	return ret_val;
}

/* yy_get_previous_state - get the state just before the EOB char was reached */

    static yy_state_type yy_get_previous_state (yyscan_t yyscanner)
{
	yy_state_type yy_current_state;
	char *yy_cp;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	yy_current_state = yyg->yy_start;

	for ( yy_cp = yyg->yytext_ptr + YY_MORE_ADJ; yy_cp < yyg->yy_c_buf_p; ++yy_cp )
		{
		YY_CHAR yy_c = (*yy_cp ? yy_ec[YY_SC_TO_UI(*yy_cp)] : 16);
		if ( yy_accept[yy_current_state] )
			{
			yyg->yy_last_accepting_state = yy_current_state;
			yyg->yy_last_accepting_cpos = yy_cp;
			}
		while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
			{
			yy_current_state = (int) yy_def[yy_current_state];
			if ( yy_current_state >= 79 )
				yy_c = yy_meta[yy_c];
			}
		yy_current_state = yy_nxt[yy_base[yy_current_state] + yy_c];
		}

	return yy_current_state;
}

/* yy_try_NUL_trans - try to make a transition on the NUL character
 *
 * synopsis
 *	next_state = yy_try_NUL_trans( current_state );
 */
    static yy_state_type yy_try_NUL_trans  (yy_state_type yy_current_state , yyscan_t yyscanner)
{
	int yy_is_jam;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner; /* This var may be unused depending upon options. */
	char *yy_cp = yyg->yy_c_buf_p;

	YY_CHAR yy_c = 16;
	if ( yy_accept[yy_current_state] )
		{
		yyg->yy_last_accepting_state = yy_current_state;
		yyg->yy_last_accepting_cpos = yy_cp;
		}
	while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
		{
		yy_current_state = (int) yy_def[yy_current_state];
		if ( yy_current_state >= 79 )
			yy_c = yy_meta[yy_c];
		}
	yy_current_state = yy_nxt[yy_base[yy_current_state] + yy_c];
	yy_is_jam = (yy_current_state == 78);

	(void)yyg;
	return yy_is_jam ? 0 : yy_current_state;
}

#ifndef YY_NO_UNPUT

    static void yyunput (int c, char * yy_bp , yyscan_t yyscanner)
{
	char *yy_cp;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

    yy_cp = yyg->yy_c_buf_p;

	/* undo effects of setting up yytext */
	*yy_cp = yyg->yy_hold_char;

	if ( yy_cp < YY_CURRENT_BUFFER_LVALUE->yy_ch_buf + 2 )
		{ /* need to shift things up to make room */
		/* +2 for EOB chars. */
		int number_to_move = yyg->yy_n_chars + 2;
		char *dest = &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[
					YY_CURRENT_BUFFER_LVALUE->yy_buf_size + 2];
		char *source =
				&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[number_to_move];

		while ( source > YY_CURRENT_BUFFER_LVALUE->yy_ch_buf )
			*--dest = *--source;

		yy_cp += (int) (dest - source);
		yy_bp += (int) (dest - source);
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars =
			yyg->yy_n_chars = (int) YY_CURRENT_BUFFER_LVALUE->yy_buf_size;

		if ( yy_cp < YY_CURRENT_BUFFER_LVALUE->yy_ch_buf + 2 )
			YY_FATAL_ERROR( "flex scanner push-back overflow" );
		}

	*--yy_cp = (char) c;

    if ( c == '\n' ){
        --yylineno;
    }

	yyg->yytext_ptr = yy_bp;
	yyg->yy_hold_char = *yy_cp;
	yyg->yy_c_buf_p = yy_cp;
}

#endif

#ifndef YY_NO_INPUT
#ifdef __cplusplus
    static int yyinput (yyscan_t yyscanner)
#else
    static int input  (yyscan_t yyscanner)
#endif

{
	int c;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	*yyg->yy_c_buf_p = yyg->yy_hold_char;

	if ( *yyg->yy_c_buf_p == YY_END_OF_BUFFER_CHAR )
		{
		/* yy_c_buf_p now points to the character we want to return.
		 * If this occurs *before* the EOB characters, then it's a
		 * valid NUL; if not, then we've hit the end of the buffer.
		 */
		if ( yyg->yy_c_buf_p < &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[yyg->yy_n_chars] )
			/* This was really a NUL. */
			*yyg->yy_c_buf_p = '\0';

		else
			{ /* need more input */
			int offset = (int) (yyg->yy_c_buf_p - yyg->yytext_ptr);
			++yyg->yy_c_buf_p;

			switch ( yy_get_next_buffer( yyscanner ) )
				{
				case EOB_ACT_LAST_MATCH:
					/* This happens because yy_g_n_b()
					 * sees that we've accumulated a
					 * token and flags that we need to
					 * try matching the token before
					 * proceeding.  But for input(),
					 * there's no matching to consider.
					 * So convert the EOB_ACT_LAST_MATCH
					 * to EOB_ACT_END_OF_FILE.
					 */

					/* Reset buffer status. */
					yyrestart( yyin , yyscanner);

					/*FALLTHROUGH*/

				case EOB_ACT_END_OF_FILE:
					{
					if ( yywrap( yyscanner ) )
						return 0;

					if ( ! yyg->yy_did_buffer_switch_on_eof )
						YY_NEW_FILE;
#ifdef __cplusplus
					return yyinput(yyscanner);
#else
					return input(yyscanner);
#endif
					}

				case EOB_ACT_CONTINUE_SCAN:
					yyg->yy_c_buf_p = yyg->yytext_ptr + offset;
					break;
				}
			}
		}

	c = *(unsigned char *) yyg->yy_c_buf_p;	/* cast for 8-bit char's */
	*yyg->yy_c_buf_p = '\0';	/* preserve yytext */
	yyg->yy_hold_char = *++yyg->yy_c_buf_p;

	if ( c == '\n' )
		
    do{ yylineno++;
        yycolumn=0;
    }while(0)
;

	return c;
}
#endif	/* ifndef YY_NO_INPUT */

/** Immediately switch to a different input stream.
 * @param input_file A readable stream.
 * @param yyscanner The scanner object.
 * @note This function does not reset the start condition to @c INITIAL .
 */
    void yyrestart  (FILE * input_file , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	if ( ! YY_CURRENT_BUFFER ){
        yyensure_buffer_stack (yyscanner);
		YY_CURRENT_BUFFER_LVALUE =
            yy_create_buffer( yyin, YY_BUF_SIZE , yyscanner);
	}

	yy_init_buffer( YY_CURRENT_BUFFER, input_file , yyscanner);
	yy_load_buffer_state( yyscanner );
}

/** Switch to a different input buffer.
 * @param new_buffer The new input buffer.
 * @param yyscanner The scanner object.
 */
    void yy_switch_to_buffer  (YY_BUFFER_STATE  new_buffer , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	/* TODO. We should be able to replace this entire function body
	 * with
	 *		yypop_buffer_state();
	 *		yypush_buffer_state(new_buffer);
     */
	yyensure_buffer_stack (yyscanner);
	if ( YY_CURRENT_BUFFER == new_buffer )
		return;

	if ( YY_CURRENT_BUFFER )
		{
		/* Flush out information for old buffer. */
		*yyg->yy_c_buf_p = yyg->yy_hold_char;
		YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = yyg->yy_c_buf_p;
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yyg->yy_n_chars;
		}

	YY_CURRENT_BUFFER_LVALUE = new_buffer;
	yy_load_buffer_state( yyscanner );

	/* We don't actually know whether we did this switch during
	 * EOF (yywrap()) processing, but the only time this flag
	 * is looked at is after yywrap() is called, so it's safe
	 * to go ahead and always set it.
	 */
	yyg->yy_did_buffer_switch_on_eof = 1;
}

static void yy_load_buffer_state  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	yyg->yy_n_chars = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
	yyg->yytext_ptr = yyg->yy_c_buf_p = YY_CURRENT_BUFFER_LVALUE->yy_buf_pos;
	yyin = YY_CURRENT_BUFFER_LVALUE->yy_input_file;
	yyg->yy_hold_char = *yyg->yy_c_buf_p;
}

/** Allocate and initialize an input buffer state.
 * @param file A readable stream.
 * @param size The character buffer size in bytes. When in doubt, use @c YY_BUF_SIZE.
 * @param yyscanner The scanner object.
 * @return the allocated buffer state.
 */
    YY_BUFFER_STATE yy_create_buffer  (FILE * file, int  size , yyscan_t yyscanner)
{
	YY_BUFFER_STATE b;
    
	b = (YY_BUFFER_STATE) yyalloc( sizeof( struct yy_buffer_state ) , yyscanner );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in yy_create_buffer()" );

	b->yy_buf_size = size;

	/* yy_ch_buf has to be 2 characters longer than the size given because
	 * we need to put in 2 end-of-buffer characters.
	 */
	b->yy_ch_buf = (char *) yyalloc( (yy_size_t) (b->yy_buf_size + 2) , yyscanner );
	if ( ! b->yy_ch_buf )
		YY_FATAL_ERROR( "out of dynamic memory in yy_create_buffer()" );

	b->yy_is_our_buffer = 1;

	yy_init_buffer( b, file , yyscanner);

	return b;
}

/** Destroy the buffer.
 * @param b a buffer created with yy_create_buffer()
 * @param yyscanner The scanner object.
 */
    void yy_delete_buffer (YY_BUFFER_STATE  b , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	if ( ! b )
		return;

	if ( b == YY_CURRENT_BUFFER ) /* Not sure if we should pop here. */
		YY_CURRENT_BUFFER_LVALUE = (YY_BUFFER_STATE) 0;

	if ( b->yy_is_our_buffer )
		yyfree( (void *) b->yy_ch_buf , yyscanner );

	yyfree( (void *) b , yyscanner );
}

/* Initializes or reinitializes a buffer.
 * This function is sometimes called more than once on the same buffer,
 * such as during a yyrestart() or at EOF.
 */
    static void yy_init_buffer  (YY_BUFFER_STATE  b, FILE * file , yyscan_t yyscanner)

{
	int oerrno = errno;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	yy_flush_buffer( b , yyscanner);

	b->yy_input_file = file;
	b->yy_fill_buffer = 1;

    /* If b is the current buffer, then yy_init_buffer was _probably_
     * called from yyrestart() or through yy_get_next_buffer.
     * In that case, we don't want to reset the lineno or column.
     */
    if (b != YY_CURRENT_BUFFER){
        b->yy_bs_lineno = 1;
        b->yy_bs_column = 0;
    }

        b->yy_is_interactive = file ? (isatty( fileno(file) ) > 0) : 0;
    
	errno = oerrno;
}

/** Discard all buffered characters. On the next scan, YY_INPUT will be called.
 * @param b the buffer state to be flushed, usually @c YY_CURRENT_BUFFER.
 * @param yyscanner The scanner object.
 */
    void yy_flush_buffer (YY_BUFFER_STATE  b , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	if ( ! b )
		return;

	b->yy_n_chars = 0;

	/* We always need two end-of-buffer characters.  The first causes
	 * a transition to the end-of-buffer state.  The second causes
	 * a jam in that state.
	 */
	b->yy_ch_buf[0] = YY_END_OF_BUFFER_CHAR;
	b->yy_ch_buf[1] = YY_END_OF_BUFFER_CHAR;

	b->yy_buf_pos = &b->yy_ch_buf[0];

	b->yy_at_bol = 1;
	b->yy_buffer_status = YY_BUFFER_NEW;

	if ( b == YY_CURRENT_BUFFER )
		yy_load_buffer_state( yyscanner );
}

/** Pushes the new state onto the stack. The new state becomes
 *  the current state. This function will allocate the stack
 *  if necessary.
 *  @param new_buffer The new state.
 *  @param yyscanner The scanner object.
 */
void yypush_buffer_state (YY_BUFFER_STATE new_buffer , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	if (new_buffer == NULL)
		return;

	yyensure_buffer_stack(yyscanner);

	/* This block is copied from yy_switch_to_buffer. */
	if ( YY_CURRENT_BUFFER )
		{
		/* Flush out information for old buffer. */
		*yyg->yy_c_buf_p = yyg->yy_hold_char;
		YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = yyg->yy_c_buf_p;
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = yyg->yy_n_chars;
		}

	/* Only push if top exists. Otherwise, replace top. */
	if (YY_CURRENT_BUFFER)
		yyg->yy_buffer_stack_top++;
	YY_CURRENT_BUFFER_LVALUE = new_buffer;

	/* copied from yy_switch_to_buffer. */
	yy_load_buffer_state( yyscanner );
	yyg->yy_did_buffer_switch_on_eof = 1;
}

/** Removes and deletes the top of the stack, if present.
 *  The next element becomes the new top.
 *  @param yyscanner The scanner object.
 */
void yypop_buffer_state (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	if (!YY_CURRENT_BUFFER)
		return;

	yy_delete_buffer(YY_CURRENT_BUFFER , yyscanner);
	YY_CURRENT_BUFFER_LVALUE = NULL;
	if (yyg->yy_buffer_stack_top > 0)
		--yyg->yy_buffer_stack_top;

	if (YY_CURRENT_BUFFER) {
		yy_load_buffer_state( yyscanner );
		yyg->yy_did_buffer_switch_on_eof = 1;
	}
}

/* Allocates the stack if it does not exist.
 *  Guarantees space for at least one push.
 */
static void yyensure_buffer_stack (yyscan_t yyscanner)
{
	yy_size_t num_to_alloc;
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

	if (!yyg->yy_buffer_stack) {

		/* First allocation is just for 2 elements, since we don't know if this
		 * scanner will even need a stack. We use 2 instead of 1 to avoid an
		 * immediate realloc on the next call.
         */
      num_to_alloc = 1; /* After all that talk, this was set to 1 anyways... */
		yyg->yy_buffer_stack = (struct yy_buffer_state**)yyalloc
								(num_to_alloc * sizeof(struct yy_buffer_state*)
								, yyscanner);
		if ( ! yyg->yy_buffer_stack )
			YY_FATAL_ERROR( "out of dynamic memory in yyensure_buffer_stack()" );

		memset(yyg->yy_buffer_stack, 0, num_to_alloc * sizeof(struct yy_buffer_state*));

		yyg->yy_buffer_stack_max = num_to_alloc;
		yyg->yy_buffer_stack_top = 0;
		return;
	}

	if (yyg->yy_buffer_stack_top >= (yyg->yy_buffer_stack_max) - 1){

		/* Increase the buffer to prepare for a possible push. */
		yy_size_t grow_size = 8 /* arbitrary grow size */;

		num_to_alloc = yyg->yy_buffer_stack_max + grow_size;
		yyg->yy_buffer_stack = (struct yy_buffer_state**)yyrealloc
								(yyg->yy_buffer_stack,
								num_to_alloc * sizeof(struct yy_buffer_state*)
								, yyscanner);
		if ( ! yyg->yy_buffer_stack )
			YY_FATAL_ERROR( "out of dynamic memory in yyensure_buffer_stack()" );

		/* zero only the new slots.*/
		memset(yyg->yy_buffer_stack + yyg->yy_buffer_stack_max, 0, grow_size * sizeof(struct yy_buffer_state*));
		yyg->yy_buffer_stack_max = num_to_alloc;
	}
}

/** Setup the input buffer state to scan directly from a user-specified character buffer.
 * @param base the character buffer
 * @param size the size in bytes of the character buffer
 * @param yyscanner The scanner object.
 * @return the newly allocated buffer state object.
 */
YY_BUFFER_STATE yy_scan_buffer  (char * base, yy_size_t  size , yyscan_t yyscanner)
{
	YY_BUFFER_STATE b;
    
	if ( size < 2 ||
	     base[size-2] != YY_END_OF_BUFFER_CHAR ||
	     base[size-1] != YY_END_OF_BUFFER_CHAR )
		/* They forgot to leave room for the EOB's. */
		return NULL;

	b = (YY_BUFFER_STATE) yyalloc( sizeof( struct yy_buffer_state ) , yyscanner );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in yy_scan_buffer()" );

	b->yy_buf_size = (int) (size - 2);	/* "- 2" to take care of EOB's */
	b->yy_buf_pos = b->yy_ch_buf = base;
	b->yy_is_our_buffer = 0;
	b->yy_input_file = NULL;
	b->yy_n_chars = b->yy_buf_size;
	b->yy_is_interactive = 0;
	b->yy_at_bol = 1;
	b->yy_fill_buffer = 0;
	b->yy_buffer_status = YY_BUFFER_NEW;

	yy_switch_to_buffer( b , yyscanner );

	return b;
}

/** Setup the input buffer state to scan a string. The next call to yylex() will
 * scan from a @e copy of @a str.
 * @param yystr a NUL-terminated string to scan
 * @param yyscanner The scanner object.
 * @return the newly allocated buffer state object.
 * @note If you want to scan bytes that may contain NUL values, then use
 *       yy_scan_bytes() instead.
 */
YY_BUFFER_STATE yy_scan_string (const char * yystr , yyscan_t yyscanner)
{
    
	return yy_scan_bytes( yystr, (int) strlen(yystr) , yyscanner);
}

/** Setup the input buffer state to scan the given bytes. The next call to yylex() will
 * scan from a @e copy of @a bytes.
 * @param yybytes the byte buffer to scan
 * @param _yybytes_len the number of bytes in the buffer pointed to by @a bytes.
 * @param yyscanner The scanner object.
 * @return the newly allocated buffer state object.
 */
YY_BUFFER_STATE yy_scan_bytes  (const char * yybytes, int  _yybytes_len , yyscan_t yyscanner)
{
	YY_BUFFER_STATE b;
	char *buf;
	yy_size_t n;
	int i;
    
	/* Get memory for full buffer, including space for trailing EOB's. */
	n = (yy_size_t) (_yybytes_len + 2);
	buf = (char *) yyalloc( n , yyscanner );
	if ( ! buf )
		YY_FATAL_ERROR( "out of dynamic memory in yy_scan_bytes()" );

	for ( i = 0; i < _yybytes_len; ++i )
		buf[i] = yybytes[i];

	buf[_yybytes_len] = buf[_yybytes_len+1] = YY_END_OF_BUFFER_CHAR;

	b = yy_scan_buffer( buf, n , yyscanner);
	if ( ! b )
		YY_FATAL_ERROR( "bad buffer in yy_scan_bytes()" );

	/* It's okay to grow etc. this buffer, and we should throw it
	 * away when we're done.
	 */
	b->yy_is_our_buffer = 1;

	return b;
}

#ifndef YY_EXIT_FAILURE
#define YY_EXIT_FAILURE 2
#endif

static void yynoreturn yy_fatal_error (const char* msg , yyscan_t yyscanner)
{
	struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	(void)yyg;
	fprintf( stderr, "%s\n", msg );
	exit( YY_EXIT_FAILURE );
}

/* Redefine yyless() so it works in section 3 code. */

#undef yyless
#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up yytext. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		yytext[yyleng] = yyg->yy_hold_char; \
		yyg->yy_c_buf_p = yytext + yyless_macro_arg; \
		yyg->yy_hold_char = *yyg->yy_c_buf_p; \
		*yyg->yy_c_buf_p = '\0'; \
		yyleng = yyless_macro_arg; \
		} \
	while ( 0 )

/* Accessor  methods (get/set functions) to struct members. */

/** Get the user-defined data for this scanner.
 * @param yyscanner The scanner object.
 */
YY_EXTRA_TYPE yyget_extra  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yyextra;
}

/** Get the current line number.
 * @param yyscanner The scanner object.
 */
int yyget_lineno  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

        if (! YY_CURRENT_BUFFER)
            return 0;
    
    return yylineno;
}

/** Get the current column number.
 * @param yyscanner The scanner object.
 */
int yyget_column  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

        if (! YY_CURRENT_BUFFER)
            return 0;
    
    return yycolumn;
}

/** Get the input stream.
 * @param yyscanner The scanner object.
 */
FILE *yyget_in  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yyin;
}

/** Get the output stream.
 * @param yyscanner The scanner object.
 */
FILE *yyget_out  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yyout;
}

/** Get the length of the current token.
 * @param yyscanner The scanner object.
 */
int yyget_leng  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yyleng;
}

/** Get the current token.
 * @param yyscanner The scanner object.
 */

char *yyget_text  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yytext;
}

/** Set the user-defined data. This data is never touched by the scanner.
 * @param user_defined The data to be associated with this scanner.
 * @param yyscanner The scanner object.
 */
void yyset_extra (YY_EXTRA_TYPE  user_defined , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yyextra = user_defined ;
}

/** Set the current line number.
 * @param _line_number line number
 * @param yyscanner The scanner object.
 */
void yyset_lineno (int  _line_number , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

        /* lineno is only valid if an input buffer exists. */
        if (! YY_CURRENT_BUFFER )
           YY_FATAL_ERROR( "yyset_lineno called with no buffer" );
    
    yylineno = _line_number;
}

/** Set the current column.
 * @param _column_no column number
 * @param yyscanner The scanner object.
 */
void yyset_column (int  _column_no , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

        /* column is only valid if an input buffer exists. */
        if (! YY_CURRENT_BUFFER )
           YY_FATAL_ERROR( "yyset_column called with no buffer" );
    
    yycolumn = _column_no;
}

/** Set the input stream. This does not discard the current
 * input buffer.
 * @param _in_str A readable stream.
 * @param yyscanner The scanner object.
 * @see yy_switch_to_buffer
 */
void yyset_in (FILE *  _in_str , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yyin = _in_str ;
}

void yyset_out (FILE *  _out_str , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yyout = _out_str ;
}

int yyget_debug  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    return yy_flex_debug;
}

void yyset_debug (int  _bdebug , yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    yy_flex_debug = _bdebug ;
}

/* Accessor methods for yylval and yylloc */

/* User-visible API */

/* yylex_init is special because it creates the scanner itself, so it is
 * the ONLY reentrant function that doesn't take the scanner as the last argument.
 * That's why we explicitly handle the declaration, instead of using our macros.
 */
int yylex_init(yyscan_t* ptr_yy_globals)
{
    if (ptr_yy_globals == NULL){
        errno = EINVAL;
        return 1;
    }

    *ptr_yy_globals = (yyscan_t) yyalloc ( sizeof( struct yyguts_t ), NULL );

    if (*ptr_yy_globals == NULL){
        errno = ENOMEM;
        return 1;
    }

    /* By setting to 0xAA, we expose bugs in yy_init_globals. Leave at 0x00 for releases. */
    memset(*ptr_yy_globals,0x00,sizeof(struct yyguts_t));

    return yy_init_globals ( *ptr_yy_globals );
}

/* yylex_init_extra has the same functionality as yylex_init, but follows the
 * convention of taking the scanner as the last argument. Note however, that
 * this is a *pointer* to a scanner, as it will be allocated by this call (and
 * is the reason, too, why this function also must handle its own declaration).
 * The user defined value in the first argument will be available to yyalloc in
 * the yyextra field.
 */
int yylex_init_extra( YY_EXTRA_TYPE yy_user_defined, yyscan_t* ptr_yy_globals )
{
    struct yyguts_t dummy_yyguts;

    yyset_extra (yy_user_defined, &dummy_yyguts);

    if (ptr_yy_globals == NULL){
        errno = EINVAL;
        return 1;
    }

    *ptr_yy_globals = (yyscan_t) yyalloc ( sizeof( struct yyguts_t ), &dummy_yyguts );

    if (*ptr_yy_globals == NULL){
        errno = ENOMEM;
        return 1;
    }

    /* By setting to 0xAA, we expose bugs in
    yy_init_globals. Leave at 0x00 for releases. */
    memset(*ptr_yy_globals,0x00,sizeof(struct yyguts_t));

    yyset_extra (yy_user_defined, *ptr_yy_globals);

    return yy_init_globals ( *ptr_yy_globals );
}

static int yy_init_globals (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
    /* Initialization is the same as for the non-reentrant scanner.
     * This function is called from yylex_destroy(), so don't allocate here.
     */

    yyg->yy_buffer_stack = NULL;
    yyg->yy_buffer_stack_top = 0;
    yyg->yy_buffer_stack_max = 0;
    yyg->yy_c_buf_p = NULL;
    yyg->yy_init = 0;
    yyg->yy_start = 0;

    yyg->yy_start_stack_ptr = 0;
    yyg->yy_start_stack_depth = 0;
    yyg->yy_start_stack =  NULL;

/* Defined in main.c */
#ifdef YY_STDINIT
    yyin = stdin;
    yyout = stdout;
#else
    yyin = NULL;
    yyout = NULL;
#endif

    /* For future reference: Set errno on error, since we are called by
     * yylex_init()
     */
    return 0;
}

/* yylex_destroy is for both reentrant and non-reentrant scanners. */
int yylex_destroy  (yyscan_t yyscanner)
{
    struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;

    /* Pop the buffer stack, destroying each element. */
	while(YY_CURRENT_BUFFER){
		yy_delete_buffer( YY_CURRENT_BUFFER , yyscanner );
		YY_CURRENT_BUFFER_LVALUE = NULL;
		yypop_buffer_state(yyscanner);
	}

	/* Destroy the stack itself. */
	yyfree(yyg->yy_buffer_stack , yyscanner);
	yyg->yy_buffer_stack = NULL;

    /* Destroy the start condition stack. */
        yyfree( yyg->yy_start_stack , yyscanner );
        yyg->yy_start_stack = NULL;

    /* Reset the globals. This is important in a non-reentrant scanner so the next time
     * yylex() is called, initialization will occur. */
    yy_init_globals( yyscanner);

    /* Destroy the main struct (reentrant only). */
    yyfree ( yyscanner , yyscanner );
    yyscanner = NULL;
    return 0;
}

/*
 * Internal utility routines.
 */

#ifndef yytext_ptr
static void yy_flex_strncpy (char* s1, const char * s2, int n , yyscan_t yyscanner)
{
	struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	(void)yyg;

	int i;
	for ( i = 0; i < n; ++i )
		s1[i] = s2[i];
}
#endif

#ifdef YY_NEED_STRLEN
static int yy_flex_strlen (const char * s , yyscan_t yyscanner)
{
	int n;
	for ( n = 0; s[n]; ++n )
		;

	return n;
}
#endif

void *yyalloc (yy_size_t  size , yyscan_t yyscanner)
{
	struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	(void)yyg;
	return malloc(size);
}

void *yyrealloc  (void * ptr, yy_size_t  size , yyscan_t yyscanner)
{
	struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	(void)yyg;

	/* The cast to (char *) in the following accommodates both
	 * implementations that use char* generic pointers, and those
	 * that use void* generic pointers.  It works with the latter
	 * because both ANSI C and C++ allow castless assignment from
	 * any pointer type to void*, and deal with argument conversions
	 * as though doing an assignment.
	 */
	return realloc(ptr, size);
}

void yyfree (void * ptr , yyscan_t yyscanner)
{
	struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	(void)yyg;
	free( (char *) ptr );	/* see yyrealloc() for (char *) cast */
}

#define YYTABLES_NAME "yytables"

/*--------------------------------------------------------------------------*/
static void cmListFileLexerSetToken(cmListFileLexer* lexer, const char* text,
                                    int length)
{
  /* Set the token line and column number.  */
  lexer->token.line = lexer->line;
  lexer->token.column = lexer->column;

  /* Use the same buffer if possible.  */
  if (lexer->token.text) {
    if (text && length < lexer->size) {
      strcpy(lexer->token.text, text);
      lexer->token.length = length;
      return;
    }
    free(lexer->token.text);
    lexer->token.text = 0;
    lexer->size = 0;
  }

  /* Need to extend the buffer.  */
  if (text) {
    lexer->token.text = strdup(text);
    lexer->token.length = length;
    lexer->size = length + 1;
  } else {
    lexer->token.length = 0;
  }
}

/*--------------------------------------------------------------------------*/
static void cmListFileLexerAppend(cmListFileLexer* lexer, const char* text,
                                  int length)
{
  char* temp;
  int newSize;

  /* If the appended text will fit in the buffer, do not reallocate.  */
  newSize = lexer->token.length + length + 1;
  if (lexer->token.text && newSize <= lexer->size) {
    strcpy(lexer->token.text + lexer->token.length, text);
    lexer->token.length += length;
    return;
  }

  /* We need to extend the buffer.  */
  temp = (char *)malloc(newSize);
  if (lexer->token.text) {
    memcpy(temp, lexer->token.text, lexer->token.length);
    free(lexer->token.text);
  }
  memcpy(temp + lexer->token.length, text, length);
  temp[lexer->token.length + length] = 0;
  lexer->token.text = temp;
  lexer->token.length += length;
  lexer->size = newSize;
}

/*--------------------------------------------------------------------------*/
static int cmListFileLexerInput(cmListFileLexer* lexer, char* buffer,
                                size_t bufferSize)
{
  if (lexer) {
    if (lexer->file) {
      /* Convert CRLF -> LF explicitly.  The C FILE "t"ext mode
         does not convert newlines on all platforms.  Move any
         trailing CR to the start of the buffer for the next read. */
      size_t cr = lexer->cr;
      size_t n;
      buffer[0] = '\r';
      n = fread(buffer + cr, 1, bufferSize - cr, lexer->file);
      if (n) {
        char* o = buffer;
        const char* i = buffer;
        const char* e;
        n += cr;
        cr = (buffer[n - 1] == '\r') ? 1 : 0;
        e = buffer + n - cr;
        while (i != e) {
          if (i[0] == '\r' && i[1] == '\n') {
            ++i;
          }
          *o++ = *i++;
        }
        n = o - buffer;
      } else {
        n = cr;
        cr = 0;
      }
      lexer->cr = cr;
      return n;
    } else if (lexer->string_left) {
      int length = lexer->string_left;
      if ((int)bufferSize < length) {
        length = (int)bufferSize;
      }
      memcpy(buffer, lexer->string_position, length);
      lexer->string_position += length;
      lexer->string_left -= length;
      return length;
    }
  }
  return 0;
}

/*--------------------------------------------------------------------------*/
static void cmListFileLexerInit(cmListFileLexer* lexer)
{
  if (lexer->file || lexer->string_buffer) {
    cmListFileLexer_yylex_init(&lexer->scanner);
    cmListFileLexer_yyset_extra(lexer, lexer->scanner);
  }
}

/*--------------------------------------------------------------------------*/
static void cmListFileLexerDestroy(cmListFileLexer* lexer)
{
  cmListFileLexerSetToken(lexer, 0, 0);
  if (lexer->file || lexer->string_buffer) {
    cmListFileLexer_yylex_destroy(lexer->scanner);
    if (lexer->file) {
      fclose(lexer->file);
      lexer->file = 0;
    }
    if (lexer->string_buffer) {
      lexer->string_buffer = 0;
      lexer->string_left = 0;
      lexer->string_position = 0;
    }
  }
}

/*--------------------------------------------------------------------------*/
cmListFileLexer* cmListFileLexer_New(void)
{
  cmListFileLexer* lexer = (cmListFileLexer*)malloc(sizeof(cmListFileLexer));
  if (!lexer) {
    return 0;
  }
  memset(lexer, 0, sizeof(*lexer));
  lexer->line = 1;
  lexer->column = 1;
  return lexer;
}

/*--------------------------------------------------------------------------*/
void cmListFileLexer_Delete(cmListFileLexer* lexer)
{
  cmListFileLexer_SetFileName(lexer, 0, 0);
  free(lexer);
}

/*--------------------------------------------------------------------------*/
static cmListFileLexer_BOM cmListFileLexer_ReadBOM(FILE* f)
{
  unsigned char b[2];
  if (fread(b, 1, 2, f) == 2) {
    if (b[0] == 0xEF && b[1] == 0xBB) {
      if (fread(b, 1, 1, f) == 1 && b[0] == 0xBF) {
        return cmListFileLexer_BOM_UTF8;
      }
    } else if (b[0] == 0xFE && b[1] == 0xFF) {
      /* UTF-16 BE */
      return cmListFileLexer_BOM_UTF16BE;
    } else if (b[0] == 0 && b[1] == 0) {
      if (fread(b, 1, 2, f) == 2 && b[0] == 0xFE && b[1] == 0xFF) {
        return cmListFileLexer_BOM_UTF32BE;
      }
    } else if (b[0] == 0xFF && b[1] == 0xFE) {
      fpos_t p;
      fgetpos(f, &p);
      if (fread(b, 1, 2, f) == 2 && b[0] == 0 && b[1] == 0) {
        return cmListFileLexer_BOM_UTF32LE;
      }
      if (fsetpos(f, &p) != 0) {
        return cmListFileLexer_BOM_Broken;
      }
      return cmListFileLexer_BOM_UTF16LE;
    }
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    return cmListFileLexer_BOM_Broken;
  }
  return cmListFileLexer_BOM_None;
}

/*--------------------------------------------------------------------------*/
int cmListFileLexer_SetFileName(cmListFileLexer* lexer, const char* name,
                                cmListFileLexer_BOM* bom)
{
  int result = 1;
  cmListFileLexerDestroy(lexer);
  if (name) {
    lexer->file = fopen(name, "rb");
    if (lexer->file) {
      if (bom) {
        *bom = cmListFileLexer_ReadBOM(lexer->file);
      }
    } else {
      result = 0;
    }
  }
  cmListFileLexerInit(lexer);
  return result;
}

/*--------------------------------------------------------------------------*/
int cmListFileLexer_SetString(cmListFileLexer* lexer, const char* text, int length)
{
  int result = 1;
  cmListFileLexerDestroy(lexer);
  if (text) {
    lexer->string_buffer = (char *) text;
    lexer->string_position = lexer->string_buffer;
    lexer->string_left = length;
  }
  cmListFileLexerInit(lexer);
  return result;
}

/*--------------------------------------------------------------------------*/
cmListFileLexer_Token* cmListFileLexer_Scan(cmListFileLexer* lexer)
{
  if (!lexer->file && !lexer->string_buffer) {
    return 0;
  }
  if (cmListFileLexer_yylex(lexer->scanner, lexer)) {
    return &lexer->token;
  } else {
    cmListFileLexer_SetFileName(lexer, 0, 0);
    return 0;
  }
}

/*--------------------------------------------------------------------------*/
long cmListFileLexer_GetCurrentLine(cmListFileLexer* lexer)
{
  return lexer->line;
}

/*--------------------------------------------------------------------------*/
long cmListFileLexer_GetCurrentColumn(cmListFileLexer* lexer)
{
  return lexer->column;
}

/*--------------------------------------------------------------------------*/
const char* cmListFileLexer_GetTypeAsString(cmListFileLexer* lexer,
                                            cmListFileLexer_Type type)
{
  (void)lexer;
  switch (type) {
    case cmListFileLexer_Token_None:
      return "nothing";
    case cmListFileLexer_Token_Space:
      return "space";
    case cmListFileLexer_Token_Newline:
      return "newline";
    case cmListFileLexer_Token_Identifier:
      return "identifier";
    case cmListFileLexer_Token_ParenLeft:
      return "left paren";
    case cmListFileLexer_Token_ParenRight:
      return "right paren";
    case cmListFileLexer_Token_ArgumentUnquoted:
      return "unquoted argument";
    case cmListFileLexer_Token_ArgumentQuoted:
      return "quoted argument";
    case cmListFileLexer_Token_ArgumentBracket:
      return "bracket argument";
    case cmListFileLexer_Token_CommentBracket:
      return "bracket comment";
    case cmListFileLexer_Token_BadCharacter:
      return "bad character";
    case cmListFileLexer_Token_BadBracket:
      return "unterminated bracket";
    case cmListFileLexer_Token_BadString:
      return "unterminated string";
  }
  return "unknown token";
}

