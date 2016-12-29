#include "Arduino.h"
#include "mruby.h"
#include "mruby/compile.h"
#include "mruby/proc.h"
#include "mruby/string.h"

/* Redirecting #p to Serial */
static void
p(mrb_state *mrb, mrb_value obj, int prompt)
{
  obj = mrb_funcall(mrb, obj, "inspect", 0);
  if (prompt) {
    if (!mrb->exc) {
      Serial.write(" => ");
    }
    else {
      obj = mrb_funcall(mrb, mrb_obj_value(mrb->exc), "inspect", 0);
    }
  }
  Serial.println(RSTRING_PTR(obj));
}

mrb_value
my_p(mrb_state *mrb, mrb_value self)
{
  mrb_value argv;

  mrb_get_args(mrb, "o", &argv);
  p(mrb, argv, 0);

  return argv;
}

static void
printstr(mrb_state *mrb, mrb_value obj, int new_line)
{
  mrb_value str;

  str = mrb_funcall(mrb, obj, "to_s", 0);
  if (new_line)
    Serial.println(RSTRING_PTR(str));
  else
    Serial.write(RSTRING_PTR(str));
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
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("IAS - Interactive Arduino Shell");
  Serial.println("");
  Serial.println("This is a very early version, please test and report errors.");
  Serial.println("Thanks :)");
  Serial.println("");
}

mrb_state *mrb;
mrbc_context *cxt;
int ai;
struct mrb_parser_state *parser;
int n;
mrb_value result;
struct RClass *krn;
struct RClass *led;

void
mrb_setup_arduino() {
  mrb = mrb_open();
  print_hint();

  cxt = mrbc_context_new(mrb);
  cxt->capture_errors = 1;
  ai = mrb_gc_arena_save(mrb);

  krn = mrb->kernel_module;
  mrb_define_method(mrb, krn, "p", my_p, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, krn, "print", my_print, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, krn, "puts", my_puts, MRB_ARGS_REQ(1));
}

byte incommingByte;
char last_code_line[1024] = { 0 };
char ruby_code[1024] = { 0 };
int char_index;
int code_block_open = false;
unsigned int stack_keep = 0;

void setup() {
  pinMode(PIN_LED0, OUTPUT);
  digitalWrite(PIN_LED0, LOW);

  Serial.begin(115200);

  mrb_setup_arduino();

  Serial.write("> ");

  ai = mrb_gc_arena_save(mrb);
}

void loop() {
  if (Serial.available() > 0) {
    char_index = 0;
    while (true) {
      if (Serial.available() > 0) {
        incommingByte = Serial.read();
        if (incommingByte == 13) {
          // New Line
          last_code_line[char_index] = '\0';
          break;
        } else {
          last_code_line[char_index++] = incommingByte;
          Serial.write(incommingByte);
        }
      }
    }
    Serial.println("");
    Serial.flush();

    strcpy(ruby_code, last_code_line);

    /* parse code */
    parser = mrb_parser_new(mrb);
    if (parser == NULL) {
      Serial.println("create parser state error");
      exit(-1);
    }
    parser->s = ruby_code;
    parser->send = ruby_code + strlen(ruby_code);
    parser->lineno = 1;
    mrb_parser_parse(parser, cxt);

    if (0 < parser->nerr) {
      /* syntax error */
      Serial.write("Syntax Error: ");
      Serial.println(parser->error_buffer[0].message);
    }
    else {
      Serial.println("Syntax OK");
      /* generate bytecode */
      struct RProc *proc = mrb_generate_code(mrb, parser);
      if (proc == NULL) {
        Serial.println("mrb_generate_code error");
        mrb_parser_free(parser);
        exit(-1);
      }

      Serial.println("Generate bytecode OK");

    //
    //   /* pass a proc for evaluation */
    //   /* evaluate the bytecode */
    //   result = mrb_vm_run(mrb,
    //       proc,
    //       mrb_top_self(mrb),
    //       stack_keep);
    //   stack_keep = proc->body.irep->nlocals;
    //   /* did an exception occur? */
    //   if (mrb->exc) {
    //     /* yes */
    //     p(mrb, mrb_obj_value(mrb->exc), 0);
    //     mrb->exc = 0;
    //   }
    //   else {
    //     /* no */
    //     if (!mrb_respond_to(mrb, result, mrb_intern_lit(mrb, "inspect"))){
    //       result = mrb_any_to_s(mrb, result);
    //     }
    //     p(mrb, result, 1);
    //   }
    }
    ruby_code[0] = '\0';
    last_code_line[0] = '\0';
    mrb_gc_arena_restore(mrb, ai);

    mrb_parser_free(parser);
    mrb_full_gc(mrb);
    Serial.write("> ");
  }
}
