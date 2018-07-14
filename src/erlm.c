
/* let b:ale_c_clang_options='-std=c11 -Wall -Ilib/argparse -Ilib/duktape -Isrc
 * I/usr/local/Cellar/erlang/20.2.4/lib/erlang/lib/erl_interface-3.10.1/include'
 */

#include "argparse.h"
#include "duk_config.h"
#include "duktape.h"
#include "ei.h"
#include "erl_interface.h"
#include "port.h"
#include "timers.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/event.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PORT_COUNT 128

static const char *const usage[] = {"erlm [options] path/to/compiled-elm.js"};

static const char *const description =
    "\nWraps an elm application into an erlang port";

static const char *const more_info =
    "\nMore info at https://github.com/alvivi/erlm";

struct erlm_config {
  int no_run;
  int verbose;
};

struct packet {
  size_t size;
  void *data;
};

void debug_stack(duk_context *ctx) {
  duk_push_context_dump(ctx);
  fprintf(stderr, "%s\n", duk_to_string(ctx, -1));
  duk_pop(ctx);
}

void erlm_put_first_prop_name(duk_context *ctx) {
  duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
  duk_next(ctx, -1, 0);
  duk_insert(ctx, -2);
  duk_pop(ctx);
}

int erlm_is_input_port(duk_context *ctx) {
  if (!duk_check_type(ctx, -1, DUK_TYPE_OBJECT)) {
    return 0;
  }
  return duk_has_prop_string(ctx, -1, "send");
}

int erlm_is_output_port(duk_context *ctx) {
  if (!duk_check_type(ctx, -1, DUK_TYPE_OBJECT)) {
    return 0;
  }
  return duk_has_prop_string(ctx, -1, "subscribe") &&
         duk_has_prop_string(ctx, -1, "unsubscribe");
}

int erlm_get_input_ports(duk_context *ctx, const char *ports[],
                         int port_count) {
  int nport = 0;

  if (!duk_check_type(ctx, -1, DUK_TYPE_OBJECT)) {
    return -1;
  }
  duk_get_prop_string(ctx, -1, "ports");
  if (!duk_check_type(ctx, -1, DUK_TYPE_OBJECT)) {
    return -1;
  }
  duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
  while (duk_next(ctx, -1, 1)) {
    if (nport >= port_count) {
      duk_pop_2(ctx);
      break;
    }
    if (!erlm_is_input_port(ctx)) {
      duk_pop_2(ctx);
      continue;
    }
    ports[nport] = duk_to_string(ctx, -2);
    nport += 1;
    duk_pop_2(ctx);
  }
  duk_pop_2(ctx);
  return nport;
}

int erlm_get_output_ports(duk_context *ctx, const char *ports[],
                          int port_count) {
  int nport = 0;

  if (!duk_check_type(ctx, -1, DUK_TYPE_OBJECT)) {
    return -1;
  }
  duk_get_prop_string(ctx, -1, "ports");
  if (!duk_check_type(ctx, -1, DUK_TYPE_OBJECT)) {
    return -1;
  }
  duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
  while (duk_next(ctx, -1, 1)) {
    if (nport >= port_count) {
      break;
    }
    if (!erlm_is_output_port(ctx)) {
      duk_pop_2(ctx);
      continue;
    }
    ports[nport] = duk_to_string(ctx, -2);
    nport += 1;
    duk_pop_2(ctx);
  }
  duk_pop_2(ctx);
  return nport;
}

int erlm_peval_file(duk_context *ctx, const char *filepath) {
  int fid, file_size, result;
  const char *file_contents;

  fid = open(filepath, O_RDONLY);
  if (fid < 0) {
    return -1;
  }

  file_size = lseek(fid, 0, SEEK_END);
  if (file_size < 0) {
    close(fid);
    return -1;
  }

  file_contents = mmap(0, file_size, PROT_READ, MAP_PRIVATE, fid, 0);
  if (file_contents == MAP_FAILED) {
    close(fid);
    return -1;
  }

  result = duk_peval_string(ctx, file_contents);

  munmap((void *)file_contents, file_size);
  close(fid);
  return result;
}

int erlm_peval_commonjs_file(duk_context *ctx, const char *filepath) {
  int result;

  duk_push_object(ctx);
  duk_put_global_string(ctx, "module");

  result = erlm_peval_file(ctx, filepath);
  if (result != 0) {
    return -1;
  }

  duk_pop(ctx);
  duk_get_global_string(ctx, "module");
  duk_get_prop_string(ctx, -1, "exports");
  duk_insert(ctx, -2);
  duk_pop(ctx);
  return 0;
}

int erlm_peval_elm_file(duk_context *ctx, const char *filepath) {
  int result;

  result = erlm_peval_commonjs_file(ctx, filepath);
  if (result != 0) {
    return result;
  }

  erlm_put_first_prop_name(ctx);
  duk_get_prop(ctx, -2);
  duk_push_string(ctx, "worker");
  result = duk_pcall_prop(ctx, -2, 0);
  duk_insert(ctx, -3);
  duk_pop(ctx);
  duk_pop(ctx);
  return result;
}

duk_context *erlm_create_context(int qid) {
  duk_context *ctx;
  ctx = duk_create_heap_default();

  duk_push_global_stash(ctx);
  duk_push_int(ctx, qid);
  duk_put_prop_string(ctx, -2, "qid");
  duk_pop(ctx);

  return ctx;
}

ETERM *erlm_get_term(duk_context *ctx, duk_idx_t idx) {
  switch (duk_get_type(ctx, idx)) {
  case DUK_TYPE_BOOLEAN:
    if (duk_get_boolean(ctx, idx)) {
      return erl_mk_atom("true");
    }
    return erl_mk_atom("false");
  case DUK_TYPE_NUMBER:
    return erl_mk_float(duk_get_number(ctx, idx));
  case DUK_TYPE_STRING:
    return erl_mk_string(duk_get_string(ctx, idx));
  case DUK_TYPE_OBJECT:
    if (duk_is_array(ctx, idx)) {
      ETERM *list = erl_mk_empty_list();
      duk_enum(ctx, idx, DUK_ENUM_ARRAY_INDICES_ONLY);
      while (duk_next(ctx, -1, 1)) {
        list = erl_cons(erlm_get_term(ctx, -1), list);
        duk_pop_2(ctx);
      }
      duk_pop(ctx);
      return list;
    } else {
      ETERM *keyword_list = erl_mk_empty_list();
      ETERM *tuple_values[2];
      duk_enum(ctx, idx, DUK_ENUM_OWN_PROPERTIES_ONLY);
      while (duk_next(ctx, -1, 1)) {
        tuple_values[0] = erl_mk_string(duk_get_string(ctx, -2));
        tuple_values[1] = erlm_get_term(ctx, -1);
        keyword_list = erl_cons(erl_mk_tuple(tuple_values, 2), keyword_list);
        duk_pop_2(ctx);
      }
      duk_pop(ctx);
      return keyword_list;
    }
  default:
    return erl_mk_atom("nil");
  }
}

duk_ret_t erlm_output_port_handler(duk_context *ctx) {
  ETERM *term;
  byte *buffer;
  const char *port_name;
  int term_size, qid;
  struct kevent event;
  struct packet *packet;

  fprintf(stderr, "OUTPUT PORT\n");

  // [c] portName = [js] this.portName;
  duk_push_this(ctx);
  duk_get_prop_index(ctx, -1, 0);
  duk_get_prop_string(ctx, -1, "portName");
  port_name = duk_get_string(ctx, -1);
  duk_pop_3(ctx);

  fprintf(stderr, "NOW CALL %s()\n", port_name);

  // [c] qid = [js] global.qid;
  duk_push_global_stash(ctx);
  duk_get_prop_string(ctx, -1, "qid");
  qid = duk_get_int(ctx, -1);
  duk_pop_2(ctx);

  term = erlm_get_term(ctx, -1);
  term_size = erl_term_len(term);
  buffer = malloc(term_size);
  erl_encode(term, buffer);
  erl_free_compound(term);

  packet = malloc(sizeof(packet));
  packet->size = term_size;
  packet->data = buffer;

  EV_SET(&event, 1, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, packet);
  kevent((int)qid, &event, 1, NULL, 0, NULL);

  return 0;
}

void erlm_subscribe_all_output_ports(struct erlm_config *config,
                                     duk_context *ctx) {
  const char *ports[MAX_PORT_COUNT];
  int ports_count;

  ports_count = erlm_get_output_ports(ctx, ports, MAX_PORT_COUNT);
  duk_get_prop_string(ctx, -1, "ports");
  for (int i = 0; i < ports_count; i++) {
    if (config->verbose) {
      fprintf(stderr, "Subscribed to port \"%s\"\n", ports[i]);
    }
    duk_get_prop_string(ctx, -1, ports[i]);
    duk_get_prop_string(ctx, -1, "subscribe");
    duk_push_c_function(ctx, erlm_output_port_handler, DUK_VARARGS);
    duk_push_string(ctx, ports[i]);
    duk_put_prop_string(ctx, -2, "portName");
    duk_call(ctx, 1);
    duk_pop_2(ctx);
  }
  duk_pop(ctx);
}

void erlm_print_ports_info(duk_context *ctx) {
  const char *ports[MAX_PORT_COUNT];
  int ports_count;

  ports_count = erlm_get_input_ports(ctx, ports, MAX_PORT_COUNT);
  fprintf(stderr, "Input ports (%d in total):\n", ports_count);
  for (int i = 0; i < ports_count; i++) {
    fprintf(stderr, "\t%s", ports[i]);
  }
  if (ports_count > 0) {
    fprintf(stderr, "\n");
  }

  ports_count = erlm_get_output_ports(ctx, ports, MAX_PORT_COUNT);
  fprintf(stderr, "Output ports (%d in total):\n", ports_count);
  for (int i = 0; i < ports_count; i++) {
    fprintf(stderr, "\t%s", ports[i]);
  }
  if (ports_count > 0) {
    fprintf(stderr, "\n");
  }
}

void erlm_listen_input_event(int qid) {
  struct kevent event;
  EV_SET(&event, 0, EVFILT_READ, EV_ADD, 0, 0, NULL);
  kevent(qid, &event, 1, NULL, 0, NULL);
}

void erlm_push_term(duk_context *ctx, ETERM *term) {
  duk_idx_t array_index;
  int list_length;
  if (ERL_IS_INTEGER(term)) {
    duk_push_int(ctx, ERL_INT_VALUE(term));
    return;
  }
  if (ERL_IS_UNSIGNED_INTEGER(term)) {
    duk_push_int(ctx, ERL_INT_VALUE(term));
    return;
  }
  if (ERL_IS_FLOAT(term)) {
    duk_push_number(ctx, ERL_FLOAT_VALUE(term));
    return;
  }
  if (ERL_IS_LIST(term)) {
    list_length = erl_length(term);
    array_index = duk_push_array(ctx);
    for (int i = 0; i < list_length; i++) {
      erlm_push_term(ctx, erl_hd(term));
      duk_put_prop_index(ctx, array_index, i);
      term = erl_tl(term);
    }
    return;
  }

  fprintf(stderr, "error: unsupported term type\n");
  abort();
}

int erlm_unroll_array(duk_context *ctx) {
  // [JS Ctx] = [..., [a1, a2 ... an]] -> [..., a1, a2 ... an]
  // TODO check array
  int enum_index, count = 0;
  duk_enum(ctx, -1, DUK_ENUM_ARRAY_INDICES_ONLY);
  enum_index = duk_normalize_index(ctx, -1);
  while (duk_next(ctx, enum_index, 1)) {
    duk_remove(ctx, -2);
    count += 1;
  }
  duk_remove(ctx, enum_index);
  duk_remove(ctx, enum_index - 1);
  return count;
}

void erlm_handle_input_event(duk_context *ctx, int qid, struct kevent *event) {
  ETERM *tuple;
  byte buffer[0xffff];
  int nbytes, arg_count;
  const char *port_name;

  if (event->ident != 0 || event->filter != EVFILT_READ) {
    return;
  }
  fprintf(stderr, "INPUT MESSAGE %d\n", event->ident);

  nbytes = erlm_packed2_read(0, buffer, event->data);
  // TODO: check nbytes
  // TODO: cehck types
  tuple = erl_decode(buffer);
  port_name = ERL_ATOM_PTR(erl_element(1, tuple));

  fprintf(stderr, "Let's call %s\n", port_name);

  // [js] <Elm Instance>.ports.<Port Name>.send.apply(null, <Parameters>);
  // TODO: check port name

  duk_get_prop_string(ctx, -1, "ports");
  duk_get_prop_string(ctx, -1, port_name);
  duk_get_prop_string(ctx, -1, "send");
  erlm_push_term(ctx, erl_element(2, tuple));
  arg_count = erlm_unroll_array(ctx);
  duk_pcall(ctx, arg_count);
  duk_pop_n(ctx, 3);

  /*duk_get_prop_string(ctx, -1, "apply");*/
  /*duk_dup_top(ctx, -1);*/
  /*erlm_push_term(ctx, erl_element(2, tuple));*/

  /*duk_get_prop_string(ctx, -1, "ports");*/
  /*duk_get_prop_string(ctx, -1, "plusOne");*/
  /*duk_get_prop_string(ctx, -1, "send");*/
  /*duk_push_number(ctx, 69);*/
  /*duk_pcall(ctx, 1);*/
  debug_stack(ctx);
  /*
    duk_get_prop_string(ctx, -1, "apply");
    duk_dup(ctx, -2);
    erlm_push_term(ctx, erl_element(2, tuple));
    debug_stack(ctx);
    duk_pcall(ctx, 2);
    debug_stack(ctx);
    // TODO: check error
    duk_pop_n(ctx, 4);
  */
  /*ei_get_type(buffer, 0, &term_type, &term_size);*/
  /*fprintf(stderr, "GOT %d bytes, %d term size, %d term type", nbytes,
   * term_size,*/
  /*term_type);*/

  erl_free_compound(tuple);
}

void erlm_handle_output_event(struct kevent *event) {
  struct packet *packet;
  if (event->ident != 1 || event->filter != EVFILT_WRITE) {
    return;
  }

  packet = event->udata;
  if (packet->size > event->data) {
    fprintf(stderr, "error: data is too big for write buffer");
    abort();
  }

  erlm_packet2_write(1, packet->data, packet->size);
  free(packet->data);
  free(packet);
}

int do_something(struct erlm_config *config, const char *filepath) {
  duk_context *ctx;
  int qid, event_count, result;
  struct kevent event;

  if ((qid = kqueue()) == -1) {
    fprintf(stderr, "error: kqueue cannot be initialized\n");
    return -1;
  }

  ctx = erlm_create_context(qid);
  erlm_timers_register(ctx);

  result = erlm_peval_elm_file(ctx, filepath);
  if (result != 0) {
    if (duk_is_error(ctx, -1)) {
      fprintf(stderr, "error: %s\n", duk_safe_to_string(ctx, -1));
    } else {
      fprintf(stderr, "error: cannot read file %s\n", filepath);
    }
    duk_destroy_heap(ctx);
    close(qid);
    return result;
  }

  if (config->verbose) {
    erlm_print_ports_info(ctx);
  }

  if (!config->no_run) {
    erlm_subscribe_all_output_ports(config, ctx);
    erlm_listen_input_event(qid);

    for (int i = 0; i < 10; i++) {

      event_count = kevent(qid, NULL, 0, &event, 1, NULL);

      if (event_count < 0) {
        break;
      }
      if (event_count > 0) {
        if (event.flags & EV_ERROR) {
          break;
        }
      }

      erlm_timers_handle_event(ctx, &event);
      erlm_handle_input_event(ctx, qid, &event);
      erlm_handle_output_event(&event);
    }
  }

  duk_destroy_heap(ctx);
  close(qid);
  return 0;
}

/*


struct erlm_vm_context {
  int qid;
  const char *filepath;
};

void *erlm_virtual_machine(void *vm_context) {
  int qid;
  const char *filepath;

  qid = ((struct erlm_vm_context *)vm_context)->qid;
  filepath = ((struct erlm_vm_context *)vm_context)->filepath;

  int result;
  duk_context *ctx = duk_create_heap_default();

  duk_push_c_function(ctx, erlm_set_timeout, DUK_VARARGS);
  duk_put_global_string(ctx, "setTimeout");
  duk_push_c_function(ctx, erlm_clear_timeout, DUK_VARARGS);
  duk_put_global_string(ctx, "clearTimeout");

  result = erlm_peval_elm_file(ctx, filepath);
  if (result != 0) {
    if (duk_is_error(ctx, -1)) {
      printf("error: %s\n", duk_safe_to_string(ctx, -1));
    } else {
      printf("error: cannot read file %s\n", filepath);
    }
    duk_destroy_heap(ctx);
    exit(-1);
  }

  *
   THIS WORKS
  struct kevent event;

  sleep(1);
  printf("sending event 6969\n");
  EV_SET(&event, 1, EVFILT_USER, EV_ADD | EV_ONESHOT, 0, 0, 0);
  kevent((int)qid, &event, 1, NULL, 0, NULL);

  EV_SET(&event, 1, EVFILT_USER, 0, NOTE_TRIGGER, 0, (void *)6969);
  kevent((int)qid, &event, 1, NULL, 0, NULL);

  [>sleep(2);<]
  printf("sending event 9001\n");
  EV_SET(&event, 2, EVFILT_USER, EV_ADD | EV_ONESHOT, 0, 0, 0);
  kevent((int)qid, &event, 1, NULL, 0, NULL);

  EV_SET(&event, 2, EVFILT_USER, 0, NOTE_TRIGGER, 0, (void *)9001);
  kevent((int)qid, &event, 1, NULL, 0, NULL);*
  return NULL;
}

int do_something(const char *filepath) {
  *struct kevent change;*
  struct kevent event;
  struct erlm_vm_context vm_context;

  int qid, event_count;
  pthread_t vm;

  if ((qid = kqueue()) == -1) {
    abort();
  }

  vm_context.qid = qid;
  vm_context.filepath = filepath;

  pthread_create(&vm, NULL, *erlm_virtual_machine, (void *)&vm_context);

  *EV_SET(&change, 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, 5000, 0);*

  for (int i = 0; i < 10; i++) {
    event_count = kevent(qid, NULL, 0, &event, 1, NULL);

    if (event_count < 0) {
      abort();
    }
    if (event_count > 0) {
      if (event.flags & EV_ERROR) {
        abort();
      }
    }

    printf("got  %d\n", (int)event.udata);
    *EV_SET(&change, 1, EVFILT_USER, EV_DELETE, 0, 0, NULL);*
    *kevent(qid, &change, 1, NULL, 0, NULL);*
  }

  *
  EVALUANDO ELM

  int result;
  duk_context *ctx = duk_create_heap_default();

  result = erlm_peval_elm_file(ctx, filepath);
  if (result != 0) {
    if (duk_is_error(ctx, -1)) {
      printf("error: %s\n", duk_safe_to_string(ctx, -1));
    } else {
      printf("error: cannot read file %s\n", filepath);
    }
    duk_destroy_heap(ctx);
    exit(-1);
  }

  printf("numer of elems: %d\n", duk_get_top(ctx));
  printf("result %d\n", result);
  duk_destroy_heap(ctx);
  *

  return 0;
}
*/

void erlm_config_set_defaults(struct erlm_config *config) {
  config->no_run = 0;
  config->verbose = 0;
}

int main(int argc, const char **argv) {
  struct erlm_config config;
  struct argparse_option options[] = {
      OPT_HELP(),
      OPT_BOOLEAN('n', "no-run", &config.no_run,
                  "avoid running the elm program", NULL, 0, 0),
      OPT_BOOLEAN('v', "verbose", &config.verbose,
                  "prints logging information to the stderr", NULL, 0, 0),
      OPT_END()};
  struct argparse argparse;

  erl_init(NULL, 0);
  erlm_config_set_defaults(&config);
  argparse_init(&argparse, options, usage, 0);
  argparse_describe(&argparse, description, more_info);
  argc = argparse_parse(&argparse, argc, argv);
  if (argc != 1) {
    argparse_usage(&argparse);
    return -1;
  }
  return do_something(&config, argv[0]);
}
