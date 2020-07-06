#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TK_RESERVED, //operator
    TK_NUM, // integer
    TK_EOF, // EOF
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind; // token
    char *str; // token's character
    Token *next; // next token
    int len; // Token length
    long val; // if TokenKind == TK_NUM, its value.
};

// current token
Token *token;

// Input program
char *user_input;


// Reports an error and exit.
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Reports an error location and exit.
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 次のトークンに進める関数
void forward_token() {
    token = token->next;
}

// 記号ならtrue,それ以外はfalseでtokenを一つ進める
bool consume(char *op) {
  if (token->kind != TK_RESERVED || token->len != strlen(op)
     || strncmp(token->str, op, token->len))
    return false;
  forward_token();
  return true;
}

// 記号ならトークンを一つ進める。それ以外はエラーを返す
void expect(char *op) {
  if (token->kind != TK_RESERVED || token->len != strlen(op)
     || strncmp(token->str, op, token->len))
      error_at(token->str, "expected %s", op);
  forward_token();
}

// 現在のトークンが数値ならば数値を返し。それ以外ならエラー
long expect_number(void) {
    if (token->kind != TK_NUM)
        error_at(token->str, "expected number, but %c", token->str[0]);
    long val = token->val;
    forward_token();
    return val;
}

// 文の終わり（EOFかどうか）を判定する関数
bool is_eof(void) {
    return  token->kind == TK_EOF;
}

// Create a new token and add it as the next token of `cur`.
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

Token *tokenize(void) {
    char *p = user_input;

    Token head = {};
    Token *cur;
    cur = &head;
    head.next = NULL;

    while(*p) {

        // 空白なら飛ばす
        if(isspace(*p)) {
            p++;
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p -q;
            continue;
        }

        // single-letter punctuator
        if (ispunct(*p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

    }
    cur = new_token(TK_EOF, cur, p, 0);

    return head.next;
}

//
// Parser
//

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // Integer
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind; // Node kind
  Node *lhs;     // Left-hand side
  Node *rhs;     // Right-hand side
  long val;      // Used if kind == ND_NUM
};

static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

static Node *expr(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);

// expr = mul ("+" mul | "-" mul)*
static Node *expr(void) {
    Node *node = mul();

    for (;;) {
        if (consume("+"))
            node = new_binary(ND_ADD, node, mul());
        else if (consume("-"))
            node = new_binary(ND_SUB, node, mul());
        else
            return node;
    }
}

// mul = unary ( "*"unary | "/" unary)*;
static Node *mul(void) {
    Node *node = unary();

    for (;;) {
        if (consume("*"))
            node = new_binary(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_binary(ND_DIV, node, unary());
        else
            return node;
    }
}

// unary = ("+" | "-")? primary;
static Node *unary(void) {
    if (consume("+"))
        return primary();
    else if (consume("-"))
        return new_binary(ND_SUB, new_num(0), primary());
    return primary();
}
// primary = "(" expr ")" | num
static Node *primary(void) {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }
    return new_num(expect_number());
}

// ASTを解析してアセンブリを吐く関数
void gen(Node *node) {
    if (node->kind == ND_NUM) {
        printf("  push %ld\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    // rdiとraxにpop rdiにrhsがraxにlhsが入る
    printf("  pop rdi\n");
    printf("  pop rax \n");

    switch(node->kind) {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv rdi\n");
        break;
    }

    // スタックにプッシュ
    printf("  push rax\n");

}
void codegen() {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // astを作成
  Node *node = expr();

  gen(node);



  // スタックに結果が残ってるのでpop
  printf("  pop rax\n");


  printf("  ret\n");

}

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  user_input = argv[1];
  token = tokenize();

  codegen();
  return 0;
}
