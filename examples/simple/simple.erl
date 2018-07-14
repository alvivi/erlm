-module(simple).

-export([test/0, start/1, init/1, plus_one/1]).

test() ->
  start('../../bin/erlm simple.js'),
  receive
    after 1000 ->
      plus_one(1000)
  end,
  nil.

start(ExternalProgram) ->
  spawn(?MODULE, init, [ExternalProgram]).


init(ExternalProgram) ->
  register(simple, self()),
  process_flag(trap_exit, true),
  Port = open_port({spawn, ExternalProgram}, [binary, {packet, 2}]),
  loop(Port).

plus_one(X) ->
  simple ! {call, {plusOne, [X]}}.

loop(Port) ->
  receive
    {Port, {data, Data}} ->
      io:format("~s ~p ~n", ["got something!", binary_to_term(Data)]),
      loop(Port);
    {call, Msg} ->
      Port ! {self(), {command, term_to_binary(Msg)}},
      loop(Port);
    stop ->
      Port ! {self(), close};
    {'EXIT', _Port, _Reason} ->
      exit(port_terminated)
  end.


