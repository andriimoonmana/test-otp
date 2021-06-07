%%
%% %CopyrightBegin%
%%
%% Copyright Ericsson AB 2010-2016. All Rights Reserved.
%%
%% Licensed under the Apache License, Version 2.0 (the "License");
%% you may not use this file except in compliance with the License.
%% You may obtain a copy of the License at
%%
%%     http://www.apache.org/licenses/LICENSE-2.0
%%
%% Unless required by applicable law or agreed to in writing, software
%% distributed under the License is distributed on an "AS IS" BASIS,
%% WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
%% See the License for the specific language governing permissions and
%% limitations under the License.
%%
%% %CopyrightEnd%
%%

%%%-------------------------------------------------------------------
%%% File: ct_config_SUITE
%%%
%%% Description:
%%% Config driver used in the CT's tests (config_2_SUITE)
%%%-------------------------------------------------------------------
-module(config_driver).
-export([read_config/1, check_parameter/1]).

read_config(ServerName)->
    ServerModule = list_to_atom(ServerName),
    ServerModule:start(),
    ServerModule:get_config().

check_parameter(ServerName)->
    ServerModule = list_to_atom(ServerName),
    case code:is_loaded(ServerModule) of
	{file, _}->
	    {ok, {config, ServerName}};
	false->
	    case code:load_file(ServerModule) of
		{module, ServerModule}->
		    {ok, {config, ServerName}};
		{error, nofile}->
		    {error, {wrong_config, "File not found: " ++ ServerName ++ ".beam"}}
	    end
    end.
