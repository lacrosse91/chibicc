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
bool consume(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    return false;
  forward_token();
  return true;
}

// 記号ならトークンを一つ進める。それ以外はエラーを返す
void expect(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op) {
      error_at(token->str, "expected %c", op);
  }
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
Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
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
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        if (*p == '+' || *p == '-') {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        }

    }
    cur = new_token(TK_EOF, cur, p);

    return head.next;
}
void codegen() {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // 最初のトークンは数値
  printf("  mov rax, %ld\n", expect_number());

  while(!is_eof()) {
      if (consume('+')) {
          printf("  add rax, %ld\n", expect_number());
          continue;
      }


      // +ではないなら-のはず
      expect('-');
      printf("  sub rax, %ld\n", expect_number());
  }

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
