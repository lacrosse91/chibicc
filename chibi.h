#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;

//
// tokenize.c
//

// Token
typedef enum {
  TK_RESERVED, // Keywords or punctuators
  TK_IDENT,    // Identifiers
  TK_NUM,      // Integer literals
  TK_EOF,      // End-of-file markers
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind; // Token kind
  Token *next;    // Next token
  long val;       // If kind is TK_NUM, its value
  char *str;      // Token string
  int len;        // Token length
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
Token *peek(char *s);
Token *consume(char *op);
Token *consume_ident(void);
void expect(char *op);
long expect_number(void);
char *expect_ident(void);
bool at_eof(void);
Token *tokenize(void);

extern char *user_input;
extern Token *token;

//
// parse.c
//

// Local variable
typedef struct Var Var;
struct Var {
    char *name; // Varible name
    Type *ty; // type
    int offset; // Offset from RBP
};

typedef struct VarList VarList;
struct VarList {
    VarList *next;
    Var *var;
};

// AST node
typedef enum {
  ND_ADD, // num + num
  ND_PTR_ADD, // ptr + num or num + ptr
  ND_SUB, // num - num
  ND_PTR_SUB, // ptr - num
  ND_PTR_DIFF,  // ptr -ptr
  ND_MUL, // *
  ND_DIV, // /
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_ASSIGN, // =
  ND_ADDR,   // unary &
  ND_DEREF,  // unary *
  ND_RETURN, // return
  ND_IF, // "if"
  ND_WHILE, // "while"
  ND_FOR,   // "for"
  ND_BLOCK, // "{...}"
  ND_FUNCALL,  // "Function call"
  ND_EXPR_STMT, // Expression statement
  ND_VAR,    // Variable
  ND_NUM, // Integer
  ND_NULL, // Empty statement
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind; // Node kind
  Node *next;    // Next node
  Type *ty;      // Type, e.g. int or pointer to int
  Token *tok;

  Node *lhs;     // Left-hand side
  Node *rhs;     // Right-hand side
  Var *var;     // Use if kind == ND_VAR
  long val;      // Used if kind == ND_NUM

  // "if" or "while" statement
  Node *cond;
  Node *then;
  Node *els;

  // "for" statement
  Node *init;
  Node *inc;

  // Block
  Node *body;

  // function call
  char *funcname;
  Node *args;
};

typedef struct Function Function;
struct Function {
    Function *next;
    char *name;
    VarList *params;

    Node *node;
    VarList *locals;
    int stack_size;
};

Function *program(void);

//
// typing.c
//
typedef enum { TY_INT, TY_PTR, TY_ARRAY } TypeKind;

struct Type {
    TypeKind kind;
    int size;
    Type *base;
    int array_len;
};

extern Type *int_type;
bool is_integer(Type *ty);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int size);
void add_type(Node *node);

//
// codegen.c
//

void codegen(Function *prog);
