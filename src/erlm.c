
/* let b:ale_c_clang_options='-std=c11 -Wall -Ilib/argparse -Ilib/duktape -Isrc
 * I/usr/local/Cellar/erlang/20.2.4/lib/erlang/lib/erl_interface-3.10.1/include'
 */

#include "argparse.h"
#include "duk_config.h"
#include "duk_elm.h"
#include "duk_erlang.h"
#include "duk_timers.h"
#include "duk_util.h"
#include "duktape.h"
#include "ei.h"
#include "erl_interface.h"
#include "events.h"
#include "io.h"
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

duk_ret_t erlm_output_port_handler(duk_context *ctx) {
  ETERM *term;
  byte *buffer;
  const char *port_name;
  int term_size, events_manager;
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
  duk_get_prop_string(ctx, -1, "eventsManager");
  events_manager = duk_get_int(ctx, -1);
  duk_pop_2(ctx);

  term = duk_erlang_get_term(ctx, -1);
  term_size = erl_term_len(term);
  buffer = malloc(term_size);
  erl_encode(term, buffer);
  erl_free_compound(term);

  packet = malloc(sizeof(packet));
  packet->size = term_size;
  packet->data = buffer;

  events_subscribe_write(events_manager, 1, packet);

  return 0;
}

void erlm_subscribe_all_output_ports(struct erlm_config *config,
                                     duk_context *ctx) {
  const char *ports[MAX_PORT_COUNT];
  int ports_count;

  ports_count = duk_elm_get_output_ports(ctx, ports, MAX_PORT_COUNT);
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

  ports_count = duk_elm_get_input_ports(ctx, ports, MAX_PORT_COUNT);
  fprintf(stderr, "Input ports (%d in total):\n", ports_count);
  for (int i = 0; i < ports_count; i++) {
    fprintf(stderr, "\t%s", ports[i]);
  }
  if (ports_count > 0) {
    fprintf(stderr, "\n");
  }

  ports_count = duk_elm_get_output_ports(ctx, ports, MAX_PORT_COUNT);
  fprintf(stderr, "Output ports (%d in total):\n", ports_count);
  for (int i = 0; i < ports_count; i++) {
    fprintf(stderr, "\t%s", ports[i]);
  }
  if (ports_count > 0) {
    fprintf(stderr, "\n");
  }
}

void erlm_listen_input_event(int events_manager) {
  events_subscribe_read(events_manager, 0, NULL);
}

void erlm_handle_input_event(duk_context *ctx, struct event *event) {
  ETERM *tuple;
  byte buffer[0xffff];
  int nbytes, arg_count;
  const char *port_name;

  if (event->id != 0 || event->type != EVENT_READ) {
    return;
  }

  nbytes = io_read_packet2(0, buffer);
  // TODO: check event->data
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
  duk_erlang_push_term(ctx, erl_element(2, tuple));
  arg_count = duk_util_unroll_array(ctx);
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

void erlm_handle_output_event(int events_manager, struct event *event) {
  struct packet *packet;
  if (event->id != 1 || event->type != EVENT_WRITE) {
    return;
  }

  packet = event->data;
  io_write_packet2(1, packet->data, packet->size);
  free(packet->data);
  free(packet);
  events_unsubscribe(events_manager, event);
}

int do_something(struct erlm_config *config, const char *filepath) {
  duk_context *ctx;
  int events_manager, event_count, result;
  struct event *event;

  if ((events_manager = events_create_manager()) == -1) {
    fprintf(stderr, "error: events manager cannot be initialized\n");
    return -1;
  }

  ctx = duk_elm_create_context(events_manager);
  duk_timers_register(ctx, events_manager);

  result = duk_elm_peval_file(ctx, filepath);
  if (result != 0) {
    if (duk_is_error(ctx, -1)) {
      fprintf(stderr, "error: %s\n", duk_safe_to_string(ctx, -1));
    } else {
      fprintf(stderr, "error: cannot read file %s\n", filepath);
    }
    duk_destroy_heap(ctx);
    close(events_manager);
    return result;
  }

  if (config->verbose) {
    erlm_print_ports_info(ctx);
  }

  if (!config->no_run) {
    erlm_subscribe_all_output_ports(config, ctx);
    erlm_listen_input_event(events_manager);

    for (int i = 0; i < 10; i++) {
      event_count = events_wait(events_manager, &event, 1);

      if (event_count < 0) {
        break;
      }

      duk_timers_handle_event(ctx, event);
      erlm_handle_input_event(ctx, event);
      erlm_handle_output_event(events_manager, event);
    }
  }

  duk_destroy_heap(ctx);
  close(events_manager);
  return 0;
}

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
