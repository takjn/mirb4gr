#include "rx63n/reboot.h"

#include "Arduino.h"
#include "mruby.h"
#include "mruby/compile.h"
#include "mruby/proc.h"
#include "mruby/string.h"

// Define if you want to debug
// #define DEBUG

#ifdef DEBUG
#  define DEBUG_PRINT(m,v)    { Serial.print("** "); Serial.print((m)); Serial.print(":"); Serial.println((v)); }
#else
#  define DEBUG_PRINT(m,v)    // do nothing
#endif

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
      printf(" => ");
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

/* Print a short remark for the user */
static void
print_hint(void)
{
  Serial.println("mirb4gr - Embeddable Interactive Ruby Shell for Gadget Renesas");
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

void setup() {
  pinMode(PIN_LED0, OUTPUT);
  digitalWrite(PIN_LED0, LOW);

  Serial.begin(115200);

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

void loop() {
  while (TRUE) {
    char *utf8;

    print_cmdline(code_block_open);

    char_index = 0;
    while ((last_char = getchar_from_serial()) != '\n') {
      if (last_char == EOF) break;
      if (char_index > sizeof(last_code_line)-2) {
        Serial.println("input string too long");
        continue;
      }
      last_code_line[char_index++] = last_char;
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
      strcpy(ruby_code, last_code_line);
    }

    utf8 = mrb_utf8_from_locale(ruby_code, -1);
    if (!utf8) abort();

    /* parse code */
    parser = mrb_parser_new(mrb);
    if (parser == NULL) {
      Serial.println("create parser state error");
      exit(EXIT_FAILURE);
    }
    parser->s = utf8;
    parser->send = utf8 + strlen(utf8);
    parser->lineno = 1;
    mrb_parser_parse(parser, cxt);
    mrb_utf8_free(utf8);

    if (0 < parser->nerr) {
      /* syntax error */
      Serial.write("Syntax Error: ");
      Serial.println(parser->error_buffer[0].message);
    }
    else {
      /* generate bytecode */
      struct RProc *proc = mrb_generate_code(mrb, parser);
      if (proc == NULL) {
        Serial.println("mrb_generate_code error");
        mrb_parser_free(parser);
        exit(EXIT_FAILURE);
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
        /* yes */
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

    mrb_parser_free(parser);
    mrb_full_gc(mrb);
  }

  mrbc_context_free(mrb, cxt);
  mrb_close(mrb);

  system_reboot(REBOOT_USERAPP);
}
