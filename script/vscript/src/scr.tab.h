/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TK_ERROR = 258,
     TK_INCLUDE = 259,
     TK_SUB = 260,
     TK_GOTO = 261,
     TK_DOUBLE_CONSTANT = 262,
     TK_STRING_CONSTANT = 263,
     TK_INT_CONSTANT = 264,
     TK_ID = 265,
     TK_CONTINUE = 266,
     TK_BREAK = 267,
     TK_RETURN = 268,
     TK_END = 269,
     TK_ELSIF = 270,
     TK_ELSE = 271,
     TK_ASSIGN = 272,
     TK_DO = 273,
     TK_IF = 274,
     TK_UNTIL = 275,
     TK_WHILE = 276,
     TK_OP_NUM_EQ = 277,
     TK_OP_NUM_NE = 278,
     TK_OP_NUM_LT = 279,
     TK_OP_NUM_GT = 280,
     TK_OP_NUM_LE = 281,
     TK_OP_NUM_GE = 282,
     TK_OK_DOT = 283,
     TK_OP_STR_EQ = 284,
     TK_OP_STR_NE = 285,
     TK_OP_STR_LT = 286,
     TK_OP_STR_GT = 287,
     TK_OP_STR_LE = 288,
     TK_OP_STR_GE = 289,
     TK_OP_STR_CAT = 290,
     TK_OP_NUM_ADD = 291,
     TK_OP_NUM_SUBST = 292,
     TK_OP_NUM_DIV = 293,
     TK_OP_NUM_MULT = 294,
     TK_OP_NUM_MOD = 295,
     TK_OP_NUM_AUTOINCR = 296,
     TK_OP_NUM_AUTODECR = 297,
     TK_OP_TOSTR = 298,
     TK_OP_TOINT = 299,
     TK_VAR_DEF = 300,
     TK_VAR_UNDEF = 301,
     TK_ARR_DEF = 302,
     TK_CODEREF_DEF = 303,
     TK_OP_STR_REGEXMATCH = 304,
     TK_COLON = 305,
     TK_SEMICOLON = 306,
     TK_COMMA = 307,
     TK_PARENTHESES_OPEN = 308,
     TK_PARENTHESES_CLOSE = 309,
     TK_BRACE_OPEN = 310,
     TK_BRACE_CLOSE = 311,
     TK_BRACKET_OPEN = 312,
     TK_BRACKET_CLOSE = 313,
     TK_CLASS = 314,
     TK_INTERFACE = 315
   };
#endif
/* Tokens.  */
#define TK_ERROR 258
#define TK_INCLUDE 259
#define TK_SUB 260
#define TK_GOTO 261
#define TK_DOUBLE_CONSTANT 262
#define TK_STRING_CONSTANT 263
#define TK_INT_CONSTANT 264
#define TK_ID 265
#define TK_CONTINUE 266
#define TK_BREAK 267
#define TK_RETURN 268
#define TK_END 269
#define TK_ELSIF 270
#define TK_ELSE 271
#define TK_ASSIGN 272
#define TK_DO 273
#define TK_IF 274
#define TK_UNTIL 275
#define TK_WHILE 276
#define TK_OP_NUM_EQ 277
#define TK_OP_NUM_NE 278
#define TK_OP_NUM_LT 279
#define TK_OP_NUM_GT 280
#define TK_OP_NUM_LE 281
#define TK_OP_NUM_GE 282
#define TK_OK_DOT 283
#define TK_OP_STR_EQ 284
#define TK_OP_STR_NE 285
#define TK_OP_STR_LT 286
#define TK_OP_STR_GT 287
#define TK_OP_STR_LE 288
#define TK_OP_STR_GE 289
#define TK_OP_STR_CAT 290
#define TK_OP_NUM_ADD 291
#define TK_OP_NUM_SUBST 292
#define TK_OP_NUM_DIV 293
#define TK_OP_NUM_MULT 294
#define TK_OP_NUM_MOD 295
#define TK_OP_NUM_AUTOINCR 296
#define TK_OP_NUM_AUTODECR 297
#define TK_OP_TOSTR 298
#define TK_OP_TOINT 299
#define TK_VAR_DEF 300
#define TK_VAR_UNDEF 301
#define TK_ARR_DEF 302
#define TK_CODEREF_DEF 303
#define TK_OP_STR_REGEXMATCH 304
#define TK_COLON 305
#define TK_SEMICOLON 306
#define TK_COMMA 307
#define TK_PARENTHESES_OPEN 308
#define TK_PARENTHESES_CLOSE 309
#define TK_BRACE_OPEN 310
#define TK_BRACE_CLOSE 311
#define TK_BRACKET_OPEN 312
#define TK_BRACKET_CLOSE 313
#define TK_CLASS 314
#define TK_INTERFACE 315




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE yylloc;
