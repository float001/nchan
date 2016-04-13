//don't edit this please, it was auto-generated

typedef struct {
  //input:  keys: [], values: [channel_id, number]
  //output: current_fake_subscribers
  char *add_fakesub;

  //input:  keys: [], values: [channel_id, ttl]
  // ttl is for when there are no messages but at least 1 subscriber.
  //output: seconds until next keepalive is expected, or -1 for "let it disappear"
  char *channel_keepalive;

  //input: keys: [],  values: [ channel_id ]
  //output: channel_hash {ttl, time_last_seen, subscribers, messages} or nil
  // delete this channel and all its messages
  char *delete;

  //input: keys: [],  values: [ channel_id ]
  //output: channel_hash {ttl, time_last_seen, subscribers, messages} or nil
  // finds and return the info hash of a channel, or nil of channel not found
  char *find_channel;

  //input:  keys: [], values: [channel_id, msg_time, msg_tag, no_msgid_order, create_channel_ttl]
  //output: result_code, msg_ttl, msg_time, msg_tag, prev_msg_time, prev_msg_tag, message, content_type, eventsource_event, channel_subscriber_count
  // no_msgid_order: 'FILO' for oldest message, 'FIFO' for most recent
  // create_channel_ttl - make new channel if it's absent, with ttl set to this. 0 to disable.
  // result_code can be: 200 - ok, 404 - not found, 410 - gone, 418 - not yet available
  char *get_message;

  //input:  keys: [message_key], values: []
  //output: msg_ttl, msg_time, msg_tag, prev_msg_time, prev_msg_tag, message, content_type, eventsource_event, channel_subscriber_count
  char *get_message_from_key;

  //input:  keys: [], values: [channel_id, time, message, content_type, eventsource_event, msg_ttl, max_msg_buf_size]
  //output: message_tag, channel_hash {ttl, time_last_seen, subscribers, messages}
  char *publish;

  //input:  keys: [], values: [channel_id, status_code]
  //output: current_subscribers
  char *publish_status;

  //redis-store consistency check
  char *rsck;

  //input: keys: [], values: [channel_id, subscriber_id, active_ttl]
  //  'subscriber_id' can be '-' for new id, or an existing id
  //  'active_ttl' is channel ttl with non-zero subscribers. -1 to persist, >0 ttl in sec
  //output: subscriber_id, num_current_subscribers, next_keepalive_time
  char *subscriber_register;

  //input: keys: [], values: [channel_id, subscriber_id, empty_ttl]
  // 'subscriber_id' is an existing id
  // 'empty_ttl' is channel ttl when without subscribers. 0 to delete immediately, -1 to persist, >0 ttl in sec
  //output: subscriber_id, num_current_subscribers
  char *subscriber_unregister;

} store_redis_lua_scripts_t;

static store_redis_lua_scripts_t store_rds_lua_hashes = {
  "f33207af23f0efab740207e8faf45a29acbb4c0a",
  "10e94d78fbc53ac62171eb118e27bf444c38bb07",
  "4c4337c33f0d3d2172a6caa039fd3d059082f536",
  "943cf0946ceac8f3701dc33e9754bbf2bc406ce7",
  "c4e3c2367116f8983649da7a850e2260a5244aea",
  "173ff5fb759e434296433d6ff2a554ec7a57cbdb",
  "b0fd2d7467ec704e1144360b0f04f30771a0f6b1",
  "d5d78200ad5cdc84538f4672e81fce7919c88616",
  "6cb1fdcbfa17f7565eb71728833016441c158d15",
  "4f8568521a7bca4ef4c6d591b878953b3aad77b9",
  "a199de4c5df64050328dea509707a48ef10d00d8"
};

#define REDIS_LUA_HASH_LENGTH 40

static store_redis_lua_scripts_t store_rds_lua_script_names = {
  "add_fakesub",
  "channel_keepalive",
  "delete",
  "find_channel",
  "get_message",
  "get_message_from_key",
  "publish",
  "publish_status",
  "rsck",
  "subscriber_register",
  "subscriber_unregister",
};

static store_redis_lua_scripts_t store_rds_lua_scripts = {
  //add_fakesub
  "--input:  keys: [], values: [channel_id, number]\n"
  "--output: current_fake_subscribers\n"
  "  \n"
  "redis.call('echo', ' ####### FAKESUBS ####### ')\n"
  "local id=ARGV[1]\n"
  "local num=tonumber(ARGV[2])\n"
  "if num==nil then\n"
  "  return {err=\"fakesub number not given\"}\n"
  "end\n"
  "\n"
  "local chan_key = 'channel:'..id\n"
  "local exists = false\n"
  "if redis.call('EXISTS', chan_key) == 1 then\n"
  "  exists = true\n"
  "end\n"
  "\n"
  "local cur = 0\n"
  "\n"
  "if exists or (not exists and num > 0) then\n"
  "  cur = redis.call('HINCRBY', chan_key, 'fake_subscribers', num)\n"
  "  if not exists then\n"
  "    redis.call('EXPIRE', chan_key, 5) --something small\n"
  "  end\n"
  "end\n"
  "\n"
  "return cur\n",

  //channel_keepalive
  "--input:  keys: [], values: [channel_id, ttl]\n"
  "-- ttl is for when there are no messages but at least 1 subscriber.\n"
  "--output: seconds until next keepalive is expected, or -1 for \"let it disappear\"\n"
  "local dbg = function(...) redis.call('echo', table.concat({...})); end\n"
  "dbg(' ####### CHANNEL KEEPALIVE ####### ')\n"
  "local id=ARGV[1]\n"
  "local ttl=tonumber(ARGV[2])\n"
  "if not ttl then\n"
  "  return {err=\"Invalid channel keepalive TTL (2nd arg)\"}\n"
  "end\n"
  "\n"
  "local random_safe_next_ttl = function(ttl)\n"
  "  return math.floor(ttl/2 + ttl/2.1 * math.random())\n"
  "end\n"
  "\n"
  "local key={\n"
  "  channel=      'channel:'..id, --hash\n"
  "  messages=     'channel:messages:'..id, --list\n"
  "}\n"
  "  \n"
  "local subs_count = tonumber(redis.call('HGET', key.channel, \"subscribers\")) or 0\n"
  "local msgs_count = tonumber(redis.call('LLEN', key.messages)) or 0\n"
  "local actual_ttl = tonumber(redis.call('TTL',  key.channel))\n"
  "\n"
  "if subs_count > 0 then\n"
  "  if msgs_count > 0 and actual_ttl > ttl then\n"
  "    return random_safe_next_ttl(actual_ttl)\n"
  "  end\n"
  "  --refresh ttl\n"
  "  redis.call('expire', key.channel, ttl);\n"
  "  redis.call('expire', key.messages, ttl);\n"
  "  return random_safe_next_ttl(ttl)\n"
  "else\n"
  "  return -1\n"
  "end\n",

  //delete
  "--input: keys: [],  values: [ channel_id ]\n"
  "--output: channel_hash {ttl, time_last_seen, subscribers, messages} or nil\n"
  "-- delete this channel and all its messages\n"
  "local id = ARGV[1]\n"
  "local key_msg=    'channel:msg:%s:'..id --not finished yet\n"
  "local key_channel='channel:'..id\n"
  "local messages=   'channel:messages:'..id\n"
  "local subscribers='channel:subscribers:'..id\n"
  "local pubsub=     'channel:pubsub:'..id\n"
  "\n"
  "local dbg = function(...) redis.call('echo', table.concat({...})); end\n"
  "\n"
  "dbg(' ####### DELETE #######')\n"
  "local num_messages = 0\n"
  "--delete all the messages right now mister!\n"
  "local msg\n"
  "while true do\n"
  "  msg = redis.call('LPOP', messages)\n"
  "  if msg then\n"
  "    num_messages = num_messages + 1\n"
  "    redis.call('DEL', key_msg:format(msg))\n"
  "  else\n"
  "    break\n"
  "  end\n"
  "end\n"
  "\n"
  "local del_msgpack =cmsgpack.pack({\"alert\", \"delete channel\", id})\n"
  "for k,channel_key in pairs(redis.call('SMEMBERS', subscribers)) do\n"
  "  redis.call('PUBLISH', channel_key, del_msgpack)\n"
  "end\n"
  "\n"
  "local nearly_departed = nil\n"
  "if redis.call('EXISTS', key_channel) ~= 0 then\n"
  "  nearly_departed = redis.call('hmget', key_channel, 'ttl', 'time_last_seen', 'subscribers')\n"
  "  for i = 1, #nearly_departed do\n"
  "    nearly_departed[i]=tonumber(nearly_departed[i]) or 0\n"
  "  end\n"
  "  \n"
  "  --leave some crumbs behind showing this channel was just deleted\n"
  "  redis.call('setex', \"channel:deleted:\"..id, 5, 1)\n"
  "  \n"
  "  table.insert(nearly_departed, num_messages)\n"
  "end\n"
  "\n"
  "redis.call('DEL', key_channel, messages, subscribers)\n"
  "\n"
  "if redis.call('PUBSUB','NUMSUB', pubsub)[2] > 0 then\n"
  "  redis.call('PUBLISH', pubsub, del_msgpack)\n"
  "end\n"
  "\n"
  "return nearly_departed\n",

  //find_channel
  "--input: keys: [],  values: [ channel_id ]\n"
  "--output: channel_hash {ttl, time_last_seen, subscribers, messages} or nil\n"
  "-- finds and return the info hash of a channel, or nil of channel not found\n"
  "local id = ARGV[1]\n"
  "local key_channel='channel:'..id\n"
  "\n"
  "redis.call('echo', ' #######  FIND_CHANNEL ######## ')\n"
  "\n"
  "if redis.call('EXISTS', key_channel) ~= 0 then\n"
  "  local ch = redis.call('hmget', key_channel, 'ttl', 'time_last_seen', 'subscribers', 'fake_subscribers')\n"
  "  if(ch[4]) then\n"
  "    --replace subscribers count with fake_subscribers\n"
  "    ch[3]=ch[4]\n"
  "    table.remove(ch, 4)\n"
  "  end\n"
  "  for i = 1, #ch do\n"
  "    ch[i]=tonumber(ch[i]) or 0\n"
  "  end\n"
  "  table.insert(ch, redis.call('llen', \"channel:messages:\"..id))\n"
  "  return ch\n"
  "else\n"
  "  return nil\n"
  "end\n",

  //get_message
  "--input:  keys: [], values: [channel_id, msg_time, msg_tag, no_msgid_order, create_channel_ttl]\n"
  "--output: result_code, msg_ttl, msg_time, msg_tag, prev_msg_time, prev_msg_tag, message, content_type, eventsource_event, channel_subscriber_count\n"
  "-- no_msgid_order: 'FILO' for oldest message, 'FIFO' for most recent\n"
  "-- create_channel_ttl - make new channel if it's absent, with ttl set to this. 0 to disable.\n"
  "-- result_code can be: 200 - ok, 404 - not found, 410 - gone, 418 - not yet available\n"
  "local id, time, tag, subscribe_if_current = ARGV[1], tonumber(ARGV[2]), tonumber(ARGV[3])\n"
  "local no_msgid_order=ARGV[4]\n"
  "local create_channel_ttl=tonumber(ARGV[5]) or 0\n"
  "local msg_id\n"
  "if time and time ~= 0 and tag then\n"
  "  msg_id=(\"%s:%s\"):format(time, tag)\n"
  "end\n"
  "\n"
  "-- This script has gotten big and ugly, but there are a few good reasons \n"
  "-- to keep it big and ugly. It needs to do a lot of stuff atomically, and \n"
  "-- redis doesn't do includes. It could be generated pre-insertion into redis, \n"
  "-- but then error messages become less useful, complicating debugging. If you \n"
  "-- have a solution to this, please help.\n"
  "\n"
  "local key={\n"
  "  next_message= 'channel:msg:%s:'..id, --hash\n"
  "  message=      'channel:msg:%s:%s', --hash\n"
  "  channel=      'channel:'..id, --hash\n"
  "  messages=     'channel:messages:'..id, --list\n"
  "--  pubsub=       'channel:subscribers:'..id, --set\n"
  "}\n"
  "\n"
  "local dbg = function(...) redis.call('echo', table.concat({...})); end\n"
  "\n"
  "dbg(' #######  GET_MESSAGE ######## ')\n"
  "\n"
  "local oldestmsg=function(list_key, old_fmt)\n"
  "  local old, oldkey\n"
  "  local n, del=0,0\n"
  "  while true do\n"
  "    n=n+1\n"
  "    old=redis.call('lindex', list_key, -1)\n"
  "    if old then\n"
  "      oldkey=old_fmt:format(old)\n"
  "      local ex=redis.call('exists', oldkey)\n"
  "      if ex==1 then\n"
  "        return oldkey\n"
  "      else\n"
  "        redis.call('rpop', list_key)\n"
  "        del=del+1\n"
  "      end \n"
  "    else\n"
  "      dbg(list_key, \" is empty\")\n"
  "      break\n"
  "    end\n"
  "  end\n"
  "end\n"
  "\n"
  "local tohash=function(arr)\n"
  "  if type(arr)~=\"table\" then\n"
  "    return nil\n"
  "  end\n"
  "  local h = {}\n"
  "  local k=nil\n"
  "  for i, v in ipairs(arr) do\n"
  "    if k == nil then\n"
  "      k=v\n"
  "    else\n"
  "      --dbg(k..\"=\"..v)\n"
  "      h[k]=v; k=nil\n"
  "    end\n"
  "  end\n"
  "  return h\n"
  "end\n"
  "\n"
  "if no_msgid_order ~= 'FIFO' then\n"
  "  no_msgid_order = 'FILO'\n"
  "end\n"
  "\n"
  "local channel = tohash(redis.call('HGETALL', key.channel))\n"
  "local new_channel = false\n"
  "if next(channel) == nil then\n"
  "  if create_channel_ttl==0 then\n"
  "    return {404, nil}\n"
  "  end\n"
  "  redis.call('HSET', key.channel, 'time', time)\n"
  "  redis.call('EXPIRE', key.channel, create_channel_ttl)\n"
  "  channel = {time=time}\n"
  "  new_channel = true\n"
  "end\n"
  "\n"
  "local subs_count = tonumber(channel.subscribers)\n"
  "\n"
  "local found_msg_id\n"
  "if msg_id==nil then\n"
  "  if new_channel then\n"
  "    dbg(\"new channel\")\n"
  "    return {418, \"\", \"\", \"\", \"\", subs_count}\n"
  "  else\n"
  "    dbg(\"no msg id given, ord=\"..no_msgid_order)\n"
  "    \n"
  "    if no_msgid_order == 'FIFO' then --most recent message\n"
  "      dbg(\"get most recent\")\n"
  "      found_msg_id=channel.current_message\n"
  "    elseif no_msgid_order == 'FILO' then --oldest message\n"
  "      dbg(\"get oldest\")\n"
  "      \n"
  "      found_msg_id=oldestmsg(key.messages, ('channel:msg:%s:'..id))\n"
  "    end\n"
  "    if found_msg_id == nil then\n"
  "      --we await a message\n"
  "      return {418, \"\", \"\", \"\", \"\", subs_count}\n"
  "    else\n"
  "      msg_id = found_msg_id\n"
  "      local msg=tohash(redis.call('HGETALL', msg_id))\n"
  "      if not next(msg) then --empty\n"
  "        return {404, \"\", \"\", \"\", \"\", subs_count}\n"
  "      else\n"
  "        dbg((\"found msg %s:%s  after %s:%s\"):format(tostring(msg.time), tostring(msg.tag), tostring(time), tostring(tag)))\n"
  "        local ttl = redis.call('TTL', msg_id)\n"
  "        return {200, ttl, tonumber(msg.time) or \"\", tonumber(msg.tag) or \"\", tonumber(msg.prev_time) or \"\", tonumber(msg.prev_tag) or \"\", msg.data or \"\", msg.content_type or \"\", msg.eventsource_event or \"\", subs_count}\n"
  "      end\n"
  "    end\n"
  "  end\n"
  "else\n"
  "  if msg_id and channel.current_message == msg_id\n"
  "   or not channel.current_message then\n"
  "    return {418, \"\", \"\", \"\", \"\", subs_count}\n"
  "  end\n"
  "\n"
  "  key.message=key.message:format(msg_id, id)\n"
  "  local msg=tohash(redis.call('HGETALL', key.message))\n"
  "\n"
  "  if next(msg) == nil then -- no such message. it might've expired, or maybe it was never there\n"
  "    dbg(\"MESSAGE NOT FOUND\")\n"
  "    return {404, nil}\n"
  "  end\n"
  "\n"
  "  local next_msg, next_msgtime, next_msgtag\n"
  "  if not msg.next then --this should have been taken care of by the channel.current_message check\n"
  "    dbg(\"NEXT MESSAGE KEY NOT PRESENT. ERROR, ERROR!\")\n"
  "    return {404, nil}\n"
  "  else\n"
  "    dbg(\"NEXT MESSAGE KEY PRESENT: \" .. msg.next)\n"
  "    key.next_message=key.next_message:format(msg.next)\n"
  "    if redis.call('EXISTS', key.next_message)~=0 then\n"
  "      local ntime, ntag, prev_time, prev_tag, ndata, ncontenttype, neventsource_event=unpack(redis.call('HMGET', key.next_message, 'time', 'tag', 'prev_time', 'prev_tag', 'data', 'content_type', 'eventsource_event'))\n"
  "      local ttl = redis.call('TTL', key.next_message)\n"
  "      dbg((\"found msg2 %i:%i  after %i:%i\"):format(ntime, ntag, time, tag))\n"
  "      return {200, ttl, tonumber(ntime) or \"\", tonumber(ntag) or \"\", tonumber(prev_time) or \"\", tonumber(prev_tag) or \"\", ndata or \"\", ncontenttype or \"\", neventsource_event or \"\", subs_count}\n"
  "    else\n"
  "      dbg(\"NEXT MESSAGE NOT FOUND\")\n"
  "      return {404, nil}\n"
  "    end\n"
  "  end\n"
  "end\n",

  //get_message_from_key
  "--input:  keys: [message_key], values: []\n"
  "--output: msg_ttl, msg_time, msg_tag, prev_msg_time, prev_msg_tag, message, content_type, eventsource_event, channel_subscriber_count\n"
  "\n"
  "local key = KEYS[1]\n"
  "\n"
  "local ttl = redis.call('TTL', key)\n"
  "local time, tag, prev_time, prev_tag, data, content_type, es_event = unpack(redis.call('HMGET', key, 'time', 'tag', 'prev_time', 'prev_tag', 'data', 'content_type', 'eventsource_event'))\n"
  "\n"
  "return {ttl, time, tag, prev_time or 0, prev_tag or 0, data or \"\", content_type or \"\", es_event or \"\"}\n",

  //publish
  "--input:  keys: [], values: [channel_id, time, message, content_type, eventsource_event, msg_ttl, max_msg_buf_size]\n"
  "--output: message_tag, channel_hash {ttl, time_last_seen, subscribers, messages}\n"
  "\n"
  "local id=ARGV[1]\n"
  "local time=tonumber(ARGV[2])\n"
  "local msg={\n"
  "  id=nil,\n"
  "  data= ARGV[3],\n"
  "  content_type=ARGV[4],\n"
  "  eventsource_event=ARGV[5],\n"
  "  ttl= tonumber(ARGV[6]),\n"
  "  time= time,\n"
  "  tag= 0\n"
  "}\n"
  "if msg.ttl == 0 then\n"
  "  msg.ttl = 126144000 --4 years\n"
  "end\n"
  "local store_at_most_n_messages = tonumber(ARGV[7])\n"
  "if store_at_most_n_messages == nil or store_at_most_n_messages == \"\" then\n"
  "  return {err=\"Argument 7, max_msg_buf_size, can't be empty\"}\n"
  "end\n"
  "if store_at_most_n_messages == 0 then\n"
  "  msg.unbuffered = 1\n"
  "end\n"
  "\n"
  "local dbg = function(...) \n"
  "  local arg = {...}\n"
  "  for i = 1, #arg do\n"
  "    arg[i]=tostring(arg[i])\n"
  "  end\n"
  "  redis.call('echo', table.concat(arg))\n"
  "end\n"
  "\n"
  "if type(msg.content_type)=='string' and msg.content_type:find(':') then\n"
  "  return {err='Message content-type cannot contain \":\" character.'}\n"
  "end\n"
  "\n"
  "dbg(' #######  PUBLISH   ######## ')\n"
  "\n"
  "-- sets all fields for a hash from a dictionary\n"
  "local hmset = function (key, dict)\n"
  "  if next(dict) == nil then return nil end\n"
  "  local bulk = {}\n"
  "  for k, v in pairs(dict) do\n"
  "    table.insert(bulk, k)\n"
  "    table.insert(bulk, v)\n"
  "  end\n"
  "  return redis.call('HMSET', key, unpack(bulk))\n"
  "end\n"
  "\n"
  "local tohash=function(arr)\n"
  "  if type(arr)~=\"table\" then\n"
  "    return nil\n"
  "  end\n"
  "  local h = {}\n"
  "  local k=nil\n"
  "  for i, v in ipairs(arr) do\n"
  "    if k == nil then\n"
  "      k=v\n"
  "    else\n"
  "      h[k]=v; k=nil\n"
  "    end\n"
  "  end\n"
  "  return h\n"
  "end\n"
  "\n"
  "local key={\n"
  "  time_offset=  'nchan:message_time_offset',\n"
  "  last_message= nil,\n"
  "  message=      'channel:msg:%s:'..id, --not finished yet\n"
  "  channel=      'channel:'..id,\n"
  "  messages=     'channel:messages:'..id,\n"
  "  subscribers=  'channel:subscribers:'..id\n"
  "}\n"
  "\n"
  "local channel_pubsub = 'channel:pubsub:'..id\n"
  "\n"
  "local new_channel\n"
  "local channel\n"
  "if redis.call('EXISTS', key.channel) ~= 0 then\n"
  "  channel=tohash(redis.call('HGETALL', key.channel))\n"
  "end\n"
  "\n"
  "if channel~=nil then\n"
  "  dbg(\"channel present\")\n"
  "  if channel.current_message ~= nil then\n"
  "    dbg(\"channel current_message present\")\n"
  "    key.last_message=('channel:msg:%s:%s'):format(channel.current_message, id)\n"
  "  else\n"
  "    dbg(\"channel current_message absent\")\n"
  "    key.last_message=nil\n"
  "  end\n"
  "  new_channel=false\n"
  "else\n"
  "  dbg(\"channel missing\")\n"
  "  channel={}\n"
  "  new_channel=true\n"
  "  key.last_message=nil\n"
  "end\n"
  "\n"
  "--set new message id\n"
  "if key.last_message then\n"
  "  local lastmsg = redis.call('HMGET', key.last_message, 'time', 'tag')\n"
  "  local lasttime, lasttag = tonumber(lastmsg[1]), tonumber(lastmsg[2])\n"
  "  dbg(\"New message id: last_time \", lasttime, \" last_tag \", lasttag, \" msg_time \", msg.time)\n"
  "  if lasttime==msg.time then\n"
  "    msg.tag=lasttag+1\n"
  "  end\n"
  "  msg.prev_time = lasttime\n"
  "  msg.prev_tag = lasttag\n"
  "else\n"
  "  msg.prev_time = 0\n"
  "  msg.prev_tag = 0\n"
  "end\n"
  "msg.id=('%i:%i'):format(msg.time, msg.tag)\n"
  "\n"
  "key.message=key.message:format(msg.id)\n"
  "if redis.call('EXISTS', key.message) ~= 0 then\n"
  "  return {err=(\"Message for channel %s id %s already exists\"):format(id, msg.id)}\n"
  "end\n"
  "\n"
  "msg.prev=channel.current_message\n"
  "if key.last_message and redis.call('exists', key.last_message) == 1 then\n"
  "  redis.call('HSET', key.last_message, 'next', msg.id)\n"
  "end\n"
  "\n"
  "--update channel\n"
  "redis.call('HSET', key.channel, 'current_message', msg.id)\n"
  "if msg.prev then\n"
  "  redis.call('HSET', key.channel, 'prev_message', msg.prev)\n"
  "end\n"
  "if time then\n"
  "  redis.call('HSET', key.channel, 'time', time)\n"
  "end\n"
  "if not channel.ttl then\n"
  "  channel.ttl=msg.ttl\n"
  "  redis.call('HSET', key.channel, 'ttl', channel.ttl)\n"
  "end\n"
  "\n"
  "if not channel.max_stored_messages then\n"
  "  channel.max_stored_messages = store_at_most_n_messages\n"
  "  redis.call('HSET', key.channel, 'max_stored_messages', store_at_most_n_messages)\n"
  "  dbg(\"channel.max_stored_messages was not set, but is now \", store_at_most_n_messages)\n"
  "else\n"
  "  channel.max_stored_messages =tonumber(channel.max_stored_messages)\n"
  "  dbg(\"channel.mas_stored_messages == \" , channel.max_stored_messages)\n"
  "end\n"
  "\n"
  "--write message\n"
  "hmset(key.message, msg)\n"
  "\n"
  "\n"
  "--check old entries\n"
  "local oldestmsg=function(list_key, old_fmt)\n"
  "  local old, oldkey\n"
  "  local n, del=0,0\n"
  "  while true do\n"
  "    n=n+1\n"
  "    old=redis.call('lindex', list_key, -1)\n"
  "    if old then\n"
  "      oldkey=old_fmt:format(old)\n"
  "      local ex=redis.call('exists', oldkey)\n"
  "      if ex==1 then\n"
  "        return oldkey\n"
  "      else\n"
  "        redis.call('rpop', list_key)\n"
  "        del=del+1\n"
  "      end \n"
  "    else\n"
  "      break\n"
  "    end\n"
  "  end\n"
  "end\n"
  "\n"
  "local max_stored_msgs = tonumber(redis.call('HGET', key.channel, 'max_stored_messages')) or -1\n"
  "\n"
  "if max_stored_msgs < 0 then --no limit\n"
  "  oldestmsg(key.messages, 'channel:msg:%s:'..id)\n"
  "  redis.call('LPUSH', key.messages, msg.id)\n"
  "elseif max_stored_msgs > 0 then\n"
  "  local stored_messages = tonumber(redis.call('LLEN', key.messages))\n"
  "  redis.call('LPUSH', key.messages, msg.id)\n"
  "  if stored_messages > max_stored_msgs then\n"
  "    local oldmsgid = redis.call('RPOP', key.messages)\n"
  "    redis.call('DEL', 'channel:msg:'..id..':'..oldmsgid)\n"
  "  end\n"
  "  oldestmsg(key.messages, 'channel:msg:%s:'..id)\n"
  "end\n"
  "\n"
  "\n"
  "--set expiration times for all the things\n"
  "  redis.call('EXPIRE', key.message, msg.ttl)\n"
  "  redis.call('EXPIRE', key.time_offset, channel.ttl)\n"
  "  redis.call('EXPIRE', key.channel, channel.ttl)\n"
  "  redis.call('EXPIRE', key.messages, channel.ttl)\n"
  "  redis.call('EXPIRE', key.subscribers, channel.ttl)\n"
  "\n"
  "--publish message\n"
  "local unpacked\n"
  "\n"
  "if #msg.data < 5*1024 then\n"
  "  unpacked= {\n"
  "    \"msg\",\n"
  "    msg.ttl or 0,\n"
  "    msg.time,\n"
  "    tonumber(msg.tag) or 0,\n"
  "    msg.prev_time or 0,\n"
  "    msg.prev_tag or 0,\n"
  "    msg.data or \"\",\n"
  "    msg.content_type or \"\",\n"
  "    msg.eventsource_event or \"\"\n"
  "  }\n"
  "else\n"
  "  unpacked= {\n"
  "    \"msgkey\",\n"
  "    msg.time,\n"
  "    tonumber(msg.tag) or 0,\n"
  "    key.message\n"
  "  }\n"
  "end\n"
  "\n"
  "local msgpacked\n"
  "\n"
  "dbg((\"Stored message with id %i:%i => %s\"):format(msg.time, msg.tag, msg.data))\n"
  "\n"
  "--now publish to the efficient channel\n"
  "local numsub = redis.call('PUBSUB','NUMSUB', channel_pubsub)[2]\n"
  "if tonumber(numsub) > 0 then\n"
  "  msgpacked = cmsgpack.pack(unpacked)\n"
  "  redis.call('PUBLISH', channel_pubsub, msgpacked)\n"
  "end\n"
  "\n"
  "local num_messages = redis.call('llen', key.messages)\n"
  "\n"
  "dbg(\"channel \", id, \" ttl: \",channel.ttl, \", subscribers: \", channel.subscribers, \"(fake: \", channel.fake_subscribers or \"nil\", \"), messages: \", num_messages)\n"
  "return { msg.tag, {tonumber(channel.ttl or msg.ttl), tonumber(channel.time or msg.time), tonumber(channel.fake_subscribers or channel.subscribers or 0), tonumber(num_messages)}, new_channel}\n",

  //publish_status
  "--input:  keys: [], values: [channel_id, status_code]\n"
  "--output: current_subscribers\n"
  "local dbg = function(...) redis.call('echo', table.concat({...})); end\n"
  "dbg(' ####### PUBLISH STATUS ####### ')\n"
  "local id=ARGV[1]\n"
  "local code=tonumber(ARGV[2])\n"
  "if code==nil then\n"
  "  return {err=\"non-numeric status code given, bailing!\"}\n"
  "end\n"
  "\n"
  "local pubmsg = \"status:\"..code\n"
  "local subs_key = 'channel:subscribers:'..id\n"
  "local chan_key = 'channel:'..id\n"
  "\n"
  "for k,channel_key in pairs(redis.call('SMEMBERS', subs_key)) do\n"
  "  --not efficient, but useful for a few short-term subscriptions\n"
  "  redis.call('PUBLISH', channel_key, pubmsg)\n"
  "end\n"
  "--clear short-term subscriber list\n"
  "redis.call('DEL', subs_key)\n"
  "--now publish to the efficient channel\n"
  "redis.call('PUBLISH', channel_pubsub, pubmsg)\n"
  "return redis.call('HGET', chan_key, 'subscribers') or 0\n",

  //rsck
  "--redis-store consistency check\n"
  "\n"
  "local concat = function(...)\n"
  "  local arg = {...}\n"
  "  for i = 1, #arg do\n"
  "    arg[i]=tostring(arg[i])\n"
  "  end\n"
  "  return table.concat(arg, \" \")\n"
  "end\n"
  "local dbg =function(...) redis.call('echo', concat(...)); end\n"
  "local errors={}\n"
  "local err = function(...)\n"
  "  local msg = concat(...)\n"
  "  dbg(msg)\n"
  "  table.insert(errors, msg)\n"
  "end\n"
  "\n"
  "local tp=function(t, max_n)\n"
  "  local tt={}\n"
  "  for i, v in pairs(t) do\n"
  "    local val = tostring(v)\n"
  "    if max_n and #val > max_n then\n"
  "      val = val:sub(1, max_n) .. \"[...]\"\n"
  "    end\n"
  "    table.insert(tt, tostring(i) .. \": \" .. val)\n"
  "  end\n"
  "  return \"{\" .. table.concat(tt, \", \") .. \"}\"\n"
  "end\n"
  "\n"
  "local tohash=function(arr)\n"
  "  if type(arr)~=\"table\" then\n"
  "    return nil\n"
  "  end\n"
  "  local h = {}\n"
  "  local k=nil\n"
  "  for i, v in ipairs(arr) do\n"
  "    if k == nil then\n"
  "      k=v\n"
  "    else\n"
  "      h[k]=v; k=nil\n"
  "    end\n"
  "  end\n"
  "  return h\n"
  "end\n"
  "local type_is = function(key, _type, description)\n"
  "  local t = redis.call(\"TYPE\", key)['ok']\n"
  "  local type_ok = true\n"
  "  if type(_type) == \"table\" then\n"
  "    type_ok = false\n"
  "    for i, v in pairs(_type) do\n"
  "      if v == _type then\n"
  "        type_ok = true\n"
  "        break\n"
  "      end\n"
  "    end\n"
  "  elseif t ~= _type then\n"
  "    err((description or \"\"), key, \"should be type \" .. _type .. \", is\", t)\n"
  "    type_ok = false\n"
  "  end\n"
  "  return type_ok, t\n"
  "end\n"
  "\n"
  "local known_msgs_count=0\n"
  "local known_msgkeys = {}\n"
  "local known_channel_keys = {}\n"
  "\n"
  "local check_msg = function(chid, msgid, prev_msgid, next_msgid, description)\n"
  "  description = description and \"msg (\" .. description ..\")\" or \"msg\"\n"
  "  local msgkey = \"channel:msg:\"..msgid..\":\"..chid\n"
  "  if not known_msgkeys[msgkey] then\n"
  "    known_msgs_count = known_msgs_count + 1\n"
  "  end\n"
  "  known_msgkeys[msgkey]=true\n"
  "  local _, t = type_is(msgkey, {\"hash\", \"none\"}, \"message hash\")\n"
  "  local msg = tohash(redis.call('HGETALL', msgkey))\n"
  "  local ttl = tonumber(redis.call('TTL', msgkey))\n"
  "  local n = tonumber(redis.call(\"HLEN\", msgkey))\n"
  "  if n > 0 and (msg.data == nil or msg.id == nil or msg.time == nil or msg.tag == nil)then\n"
  "    err(\"incomplete \" .. description ..\"(ttl \"..ttl..\")\", msgkey, tp(msg))\n"
  "    return false\n"
  "  end\n"
  "  if t == \"hash\" and tonumber(ttl) < 0 then\n"
  "    err(\"message\", msgkey, \"ttl =\", ttl)\n"
  "  end\n"
  "  if ttl ~= -2 then\n"
  "    if prev_msgid ~= false and msg.prev ~= prev_msgid then\n"
  "      err(description, chid, msgid, \"prev_message wrong. expected\", prev_msgid, \"got\", msg.prev)\n"
  "    end\n"
  "    if next_msgid ~= false and msg.next ~= next_msgid then\n"
  "      err(description, chid, msgid, \"next_message wrong. expected\", next_msgid, \"got\", msg.next)\n"
  "    end\n"
  "  end\n"
  "end\n"
  "\n"
  "local check_orphan_msg = function()\n"
  "\n"
  "end\n"
  "\n"
  "local check_channel = function(id)\n"
  "  local key={\n"
  "    ch = \"channel:\"..id,\n"
  "    msgs = \"channel:messages:\" .. id\n"
  "  }\n"
  "  \n"
  "  local ok, chkey_type = type_is(key.ch, \"hash\", \"channel hash\")\n"
  "  if not ok then\n"
  "    if chkey_type ~= \"none\" then\n"
  "      err(\"unecpected channel key\", key.ch, \"type:\", chkey_type);\n"
  "    end\n"
  "    return false\n"
  "  end\n"
  "  local _, msgs_list_type = type_is(key.msgs, {\"list\", \"none\"}, \"channel messages list\")\n"
  "  \n"
  "  local ch = tohash(redis.call('HGETALL', key.ch))\n"
  "  local len = tonumber(redis.call(\"HLEN\", key.ch))\n"
  "  local ttl = tonumber(redis.call('TTL',  key.ch))\n"
  "  if not ch.current_message or not ch.time then\n"
  "    if msgs_list_type == \"list\" then\n"
  "      err(\"incomplete channel (ttl \" .. ttl ..\")\", key.ch, tp(ch))\n"
  "    end  \n"
  "  elseif (ch.current_message or ch.prev_message) and msgs_list_type ~= \"list\" then\n"
  "    err(\"channel\", key.ch, \"has a current_message but no message list\")\n"
  "  end\n"
  "  \n"
  "  local msgids = redis.call('LRANGE', key.msgs, 0, -1)\n"
  "  for i, msgid in ipairs(msgids) do\n"
  "    check_msg(id, msgid, msgids[i+1], msgids[i-1], \"msglist\")\n"
  "  end\n"
  "  \n"
  "  if ch.prev_message then\n"
  "    if redis.call('LINDEX', key.msgs, 1) ~= ch.prev_message then\n"
  "      err(\"channel\", key.ch, \"prev_message doesn't correspond to\", key.msgs, \"second list element\")\n"
  "    end\n"
  "    check_msg(id, ch.prev_message, false, ch.current_message, \"channel prev_message\")\n"
  "  end\n"
  "  if ch.current_message then\n"
  "    if redis.call('LINDEX', key.msgs, 0) ~= ch.current_message then\n"
  "      err(\"channel\", key.ch, \"current_message doesn't correspond to\", key.msgs, \"first list element\")\n"
  "    end\n"
  "    check_msg(id, ch.current_message, ch.prev_message, false, \"channel current_message\")\n"
  "  end\n"
  "  \n"
  "end\n"
  "\n"
  "local channel_ids = {}\n"
  "\n"
  "for i, chkey in ipairs(redis.call(\"KEYS\", \"channel:*\")) do\n"
  "  local msgs_chid_match = chkey:match(\"^channel:messages:(.*)\")\n"
  "  if msgs_chid_match then\n"
  "    local ok, chtype = type_is(\"channel:\" .. msgs_chid_match, \"hash\")\n"
  "    if not ok then\n"
  "      \n"
  "    end\n"
  "    \n"
  "  elseif not chkey:match(\"^channel:msg:\") then\n"
  "    table.insert(channel_ids, chkey);\n"
  "    known_channel_keys[chkey] = true\n"
  "  end\n"
  "end\n"
  "\n"
  "dbg(\"found\", #channel_ids, \"channels\")\n"
  "for i, chkey in ipairs(channel_ids) do\n"
  "  local chid = chkey:match(\"^channel:(.*)\")\n"
  "  check_channel(chid)\n"
  "end\n"
  "\n"
  "for i, msgkey in ipairs(redis.call(\"KEYS\", \"channel:msg:*\")) do\n"
  "  if not known_msgkeys[msgkey] then\n"
  "    local ok, t = type_is(msgkey, \"hash\")\n"
  "    if ok then\n"
  "      if not redis.call('HGET', msgkey, 'unbuffered') then\n"
  "        err(\"orphan message\", msgkey, \"(ttl: \" .. redis.call('TTL', msgkey) .. \")\", tp(tohash(redis.call('HGETALL', msgkey)), 15))\n"
  "      end\n"
  "    else\n"
  "      err(\"orphan message\", msgkey, \"wrong type\", t) \n"
  "    end\n"
  "  end\n"
  "end\n"
  "\n"
  "if errors then\n"
  "  table.insert(errors, 1, concat(#channel_ids, \"channels,\",known_msgs_count,\"messages found\", #errors, \"problems\"))\n"
  "  return errors\n"
  "else\n"
  "  return concat(#channel_ids, \"channels,\", known_msgs_count, \"messages, all ok\")\n"
  "end\n",

  //subscriber_register
  "--input: keys: [], values: [channel_id, subscriber_id, active_ttl]\n"
  "--  'subscriber_id' can be '-' for new id, or an existing id\n"
  "--  'active_ttl' is channel ttl with non-zero subscribers. -1 to persist, >0 ttl in sec\n"
  "--output: subscriber_id, num_current_subscribers, next_keepalive_time\n"
  "\n"
  "local id, sub_id, active_ttl, concurrency = ARGV[1], ARGV[2], tonumber(ARGV[3]) or 20, ARGV[4]\n"
  "\n"
  "local dbg = function(...) redis.call('echo', table.concat({...})); end\n"
  "\n"
  "dbg(' ######## SUBSCRIBER REGISTER SCRIPT ####### ')\n"
  "\n"
  "local keys = {\n"
  "  channel =     'channel:'..id,\n"
  "  messages =    'channel:messages:'..id,\n"
  "  subscribers = 'channel:subscribers:'..id\n"
  "}\n"
  "\n"
  "local setkeyttl=function(ttl)\n"
  "  for i,v in pairs(keys) do\n"
  "    if ttl > 0 then\n"
  "      redis.call('expire', v, ttl)\n"
  "    else\n"
  "      redis.call('persist', v)\n"
  "    end\n"
  "  end\n"
  "end\n"
  "\n"
  "local random_safe_next_ttl = function(ttl)\n"
  "  return math.floor(ttl/2 + ttl/2.1 * math.random())\n"
  "end\n"
  "\n"
  "local sub_count\n"
  "\n"
  "if sub_id == \"-\" then\n"
  "  sub_id = tonumber(redis.call('HINCRBY', keys.channel, \"last_subscriber_id\", 1))\n"
  "  sub_count=tonumber(redis.call('hincrby', keys.channel, 'subscribers', 1))\n"
  "else\n"
  "  sub_count=tonumber(redis.call('hget', keys.channel, 'subscribers'))\n"
  "end\n"
  "\n"
  "local next_keepalive \n"
  "local actual_ttl = tonumber(redis.call('ttl', keys.channel))\n"
  "if actual_ttl < active_ttl then\n"
  "  setkeyttl(active_ttl)\n"
  "  next_keepalive = random_safe_next_ttl(active_ttl)\n"
  "else\n"
  "  next_keepalive = random_safe_next_ttl(actual_ttl)\n"
  "end\n"
  "\n"
  "return {sub_id, sub_count, next_keepalive}\n",

  //subscriber_unregister
  "--input: keys: [], values: [channel_id, subscriber_id, empty_ttl]\n"
  "-- 'subscriber_id' is an existing id\n"
  "-- 'empty_ttl' is channel ttl when without subscribers. 0 to delete immediately, -1 to persist, >0 ttl in sec\n"
  "--output: subscriber_id, num_current_subscribers\n"
  "\n"
  "local id, sub_id, empty_ttl = ARGV[1], ARGV[2], tonumber(ARGV[3]) or 20\n"
  "\n"
  "local dbg = function(...) redis.call('echo', table.concat({...})); end\n"
  "\n"
  "dbg(' ######## SUBSCRIBER UNREGISTER SCRIPT ####### ')\n"
  "\n"
  "local keys = {\n"
  "  channel =     'channel:'..id,\n"
  "  messages =    'channel:messages:'..id,\n"
  "  subscribers = 'channel:subscribers:'..id,\n"
  "}\n"
  "\n"
  "local setkeyttl=function(ttl)\n"
  "  for i,v in pairs(keys) do\n"
  "    if ttl > 0 then\n"
  "      redis.call('expire', v, ttl)\n"
  "    elseif ttl < 0 then\n"
  "      redis.call('persist', v)\n"
  "    else\n"
  "      redis.call('del', v)\n"
  "    end\n"
  "  end\n"
  "end\n"
  "\n"
  "local sub_count = 0\n"
  "if redis.call('EXISTS', keys.channel) ~= 0 then\n"
  "   sub_count = redis.call('hincrby', keys.channel, 'subscribers', -1)\n"
  "\n"
  "  if sub_count == 0 then\n"
  "    setkeyttl(empty_ttl)\n"
  "  elseif sub_count < 0 then\n"
  "    return {err=\"Subscriber count for channel \" .. id .. \" less than zero: \" .. sub_count}\n"
  "  end\n"
  "else\n"
  "  dbg(\"channel \", id, \" already gone\")\n"
  "end\n"
  "\n"
  "return {sub_id, sub_count}\n"
};

