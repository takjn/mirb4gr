/*
** mirb - Embeddable Interactive Ruby Shell
**
** This program takes code from the user in
** an interactive way and executes it
** immediately. It's a REPL...
*/

// Define if you want to debug
// #define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(m,v)    { Serial.print("** "); Serial.print((m)); Serial.print(":"); Serial.println((v)); }
#else
#define DEBUG_PRINT(m,v)    // do nothing
#endif

#include "rx63n/reboot.h"
#include "Arduino.h"

#include <mruby.h>
#include <mruby/array.h>
#include <mruby/proc.h>
#include <mruby/compile.h>
#include <mruby/string.h>

static void
printstr(mrb_state *mrb, mrb_value obj, int new_line)
{
  mrb_value str;

  str = mrb_funcall(mrb, obj, "to_s", 0);
  Serial.write(RSTRING_PTR(str), RSTRING_LEN(str));
  if (new_line)
  {
    Serial.println("");
  }
}

/* Redirecting #p to Serial */
static void
p(mrb_state *mrb, mrb_value obj, int prompt)
{
  mrb_value val;

  val = mrb_funcall(mrb, obj, "inspect", 0);
  if (prompt) {
    if (!mrb->exc) {
      Serial.print(" => ");
    }
    else {
      val = mrb_funcall(mrb, mrb_obj_value(mrb->exc), "inspect", 0);
    }
  }
  if (!mrb_string_p(val)) {
    val = mrb_obj_as_string(mrb, obj);
  }
  printstr(mrb, val, 1);
}

mrb_value
my_p(mrb_state *mrb, mrb_value self)
{
  mrb_value argv;

  mrb_get_args(mrb, "o", &argv);
  p(mrb, argv, 0);

  return argv;
}

mrb_value
my_print(mrb_state *mrb, mrb_value self)
{
  mrb_value argv;

  mrb_get_args(mrb, "o", &argv);
  printstr(mrb, argv, 0);

  return argv;
}

mrb_value
my_puts(mrb_state *mrb, mrb_value self)
{
  mrb_value argv;

  mrb_get_args(mrb, "o", &argv);
  printstr(mrb, argv, 1);

  return argv;
}

/* Guess if the user might want to enter more
 * or if he wants an evaluation of his code now */
static mrb_bool
is_code_block_open(struct mrb_parser_state *parser)
{
  mrb_bool code_block_open = FALSE;

  /* check for heredoc */
  if (parser->parsing_heredoc != NULL) return TRUE;
  if (parser->heredoc_end_now) {
    parser->heredoc_end_now = FALSE;
    return FALSE;
  }

  /* check for unterminated string */
  if (parser->lex_strterm) return TRUE;

  /* check if parser error are available */
  if (0 < parser->nerr) {
    const char unexpected_end[] = "syntax error, unexpected $end";
    const char *message = parser->error_buffer[0].message;

    /* a parser error occur, we have to check if */
    /* we need to read one more line or if there is */
    /* a different issue which we have to show to */
    /* the user */

    if (strncmp(message, unexpected_end, sizeof(unexpected_end) - 1) == 0) {
      code_block_open = TRUE;
    }
    else if (strcmp(message, "syntax error, unexpected keyword_end") == 0) {
      code_block_open = FALSE;
    }
    else if (strcmp(message, "syntax error, unexpected tREGEXP_BEG") == 0) {
      code_block_open = FALSE;
    }
    return code_block_open;
  }

  switch (parser->lstate) {

  /* all states which need more code */

  case EXPR_BEG:
    /* beginning of a statement, */
    /* that means previous line ended */
    code_block_open = FALSE;
    break;
  case EXPR_DOT:
    /* a message dot was the last token, */
    /* there has to come more */
    code_block_open = TRUE;
    break;
  case EXPR_CLASS:
    /* a class keyword is not enough! */
    /* we need also a name of the class */
    code_block_open = TRUE;
    break;
  case EXPR_FNAME:
    /* a method name is necessary */
    code_block_open = TRUE;
    break;
  case EXPR_VALUE:
    /* if, elsif, etc. without condition */
    code_block_open = TRUE;
    break;

  /* now all the states which are closed */

  case EXPR_ARG:
    /* an argument is the last token */
    code_block_open = FALSE;
    break;

  /* all states which are unsure */

  case EXPR_CMDARG:
    break;
  case EXPR_END:
    /* an expression was ended */
    break;
  case EXPR_ENDARG:
    /* closing parenthese */
    break;
  case EXPR_ENDFN:
    /* definition end */
    break;
  case EXPR_MID:
    /* jump keyword like break, return, ... */
    break;
  case EXPR_MAX_STATE:
    /* don't know what to do with this token */
    break;
  default:
    /* this state is unexpected! */
    break;
  }

  return code_block_open;
}

/* Print a short remark for the user */
static void
print_hint(void)
{
  Serial.println("mirb4gr - Embeddable Interactive Ruby Shell for Gadget Renesas");
  Serial.println("  commands:");
  Serial.println("  quit, exit    system reboot");
  Serial.println("  help          show this screen");
}

#ifndef ENABLE_READLINE
/* Print the command line prompt of the REPL */
static void
print_cmdline(int code_block_open)
{
  if (code_block_open) {
    Serial.print("* ");
  }
  else {
    Serial.print("> ");
  }
}
#endif

void mrb_codedump_all(mrb_state*, struct RProc*);

static int
check_keyword(const char *buf, const char *word)
{
  const char *p = buf;
  size_t len = strlen(word);

  /* skip preceding spaces */
  while (*p && isspace((unsigned char)*p)) {
    p++;
  }
  /* check keyword */
  if (strncmp(p, word, len) != 0) {
    return 0;
  }
  p += len;
  /* skip trailing spaces */
  while (*p) {
    if (!isspace((unsigned char)*p)) return 0;
    p++;
  }
  return 1;
}

bool input_canceled = 0;
bool
char_handler(char chr)
{
  if (chr == 3) {
    input_canceled = 1;
    return 1;
  }
  return 0;
}

char ruby_code[4096] = { 0 };
char last_code_line[1024] = { 0 };
int last_char;
size_t char_index;
mrbc_context *cxt;
struct mrb_parser_state *parser;
mrb_state *mrb;
mrb_value result;
int n;
int i;
mrb_bool code_block_open = FALSE;
int ai;
unsigned int stack_keep = 0;

void
setup() {
  Serial.begin(115200);

  /* new interpreter instance */
  mrb = mrb_open();
  if (mrb == NULL) {
    Serial.println("Invalid mrb interpreter, exiting mirb");
    exit(EXIT_FAILURE);
  }

  // Redirecting stdout to Serial
  struct RClass *krn;
  krn = mrb->kernel_module;
  mrb_define_method(mrb, krn, "p", my_p, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, krn, "print", my_print, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, krn, "puts", my_puts, MRB_ARGS_REQ(1));

  print_hint();

  cxt = mrbc_context_new(mrb);
  cxt->capture_errors = TRUE;
  cxt->lineno = 1;
  mrbc_filename(mrb, cxt, "(mirb)");

  ai = mrb_gc_arena_save(mrb);

}

static char
getchar_from_serial(void)
{
  int key;

  while (true) {
    if (Serial.available() > 0) {
      key = Serial.read();
      DEBUG_PRINT("Serial.read", key);

      // Backspace (temporary code)
      if (key == 127 || key == 8) {
    		if (char_index > 0) {
          char_index--;
          Serial.print("\b");
          Serial.print(" ");
          Serial.print("\b");
        }
        continue;
    	}
      // Cursor (temporary code)
      if (key == 27) {
        // temporarily, negrect cursor key
        while (Serial.available() > 0) {
          key = Serial.read();
        }
        continue;
      }

      break;
    }
  }

  if (key == 13) {
    Serial.write('\r');
    key = '\n';
  }

  Serial.write(key);
  return key;
}

void
loop() {
  while (TRUE) {
    char *utf8;

    print_cmdline(code_block_open);

    char_index = 0;
    while ((last_char = getchar_from_serial()) != '\n') {
      if (last_char == EOF) break;
      if (char_handler(last_char)) break;
      if (char_index > sizeof(last_code_line)-2) {
        Serial.println("input string too long");
        continue;
      }
      last_code_line[char_index++] = last_char;
    }
    if (input_canceled) {
      ruby_code[0] = '\0';
      last_code_line[0] = '\0';
      code_block_open = FALSE;
      Serial.println("^C");
      input_canceled = 0;
      continue;
    }
    if (last_char == EOF) {
      Serial.println("");
      break;
    }

    last_code_line[char_index++] = '\n';
    last_code_line[char_index] = '\0';

done:

    if (code_block_open) {
      if (strlen(ruby_code)+strlen(last_code_line) > sizeof(ruby_code)-1) {
        Serial.println("concatenated input string too long");
        continue;
      }
      strcat(ruby_code, last_code_line);
    }
    else {
      if (check_keyword(last_code_line, "quit") || check_keyword(last_code_line, "exit")) {
        break;
      }
      else if (check_keyword(last_code_line, "help")) {
        print_hint();
        continue;
      }

      strcpy(ruby_code, last_code_line);
    }

    utf8 = mrb_utf8_from_locale(ruby_code, -1);
    if (!utf8) abort();

    /* parse code */
    parser = mrb_parser_new(mrb);
    if (parser == NULL) {
      Serial.println("create parser state error");
      break;
    }
    parser->s = utf8;
    parser->send = utf8 + strlen(utf8);
    parser->lineno = cxt->lineno;
    mrb_parser_parse(parser, cxt);
    code_block_open = is_code_block_open(parser);
    mrb_utf8_free(utf8);

    if (code_block_open) {
      /* no evaluation of code */
    }
    else {
      if (0 < parser->nerr) {
        /* syntax error */
        Serial.print("line ");
        Serial.print(parser->error_buffer[0].lineno);
        Serial.print(": ");
        Serial.println(parser->error_buffer[0].message);
      }
      else {
        /* generate bytecode */
        struct RProc *proc = mrb_generate_code(mrb, parser);
        if (proc == NULL) {
          Serial.println("codegen error");
          mrb_parser_free(parser);
          break;
        }

        /* pass a proc for evaluation */
        /* evaluate the bytecode */
        result = mrb_vm_run(mrb,
            proc,
            mrb_top_self(mrb),
            stack_keep);
        stack_keep = proc->body.irep->nlocals;
        /* did an exception occur? */
        if (mrb->exc) {
          p(mrb, mrb_obj_value(mrb->exc), 0);
          mrb->exc = 0;
        }
        else {
          /* no */
          if (!mrb_respond_to(mrb, result, mrb_intern_lit(mrb, "inspect"))){
            result = mrb_any_to_s(mrb, result);
          }
          p(mrb, result, 1);
        }
      }
      ruby_code[0] = '\0';
      last_code_line[0] = '\0';
      mrb_gc_arena_restore(mrb, ai);
    }
    mrb_parser_free(parser);
    cxt->lineno++;

    mrb_full_gc(mrb);
  }

  mrbc_context_free(mrb, cxt);
  mrb_close(mrb);

  system_reboot(REBOOT_USERAPP);
}
