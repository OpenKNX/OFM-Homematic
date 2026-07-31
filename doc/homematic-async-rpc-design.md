# OFM-Homematic: Async RPC Design (Sketch only, not implemented)

Status: RpcUtil::sendRequestGetResponseDoc currently uses openknxNetwork.webclient but
still blocks synchronously (busy-loops webclient.loop()+yield() until onDone or timeout).
This is step 1 (done). Step 2 (full async) was only sketched/discussed, NOT implemented.

## Constraints from user
- Module may have at most 1 RPC request in flight at any time (business rule, independent
  of OPENKNX_WEBCLIENT_SLOTS which is shared across modules).
- KO write commands (setValue) must be sent ASAP; multiple distinct commands must not be lost.

## Proposed design (discussed, not coded)
- New central scheduler (e.g. inside RpcUtil or new RpcScheduler owned by HomematicModule).
- 3-tier priority queues, scheduler services highest non-empty queue when idle:
  1. Commands (KO writes, rpcSetValue*) - highest priority
  2. On-demand user actions (Function-Property ScanResult/DeviceInfo) - medium
  3. Routine cyclic polling (getParamset per channel) - lowest/background
- Command queue: fixed-capacity array, coalesced by key (channelIndex, paramName) -
  last-value-wins per key, but distinct keys never overwrite each other (no loss).
- Poll "jobs" can be multi-step (HomematicChannel::update() currently does 2 sequential
  getParamset calls: channel 0, then device channel) - model as job+step index, aggregate
  success across steps like today's `&&` short-circuit.
- Scheduler tick runs once per HomematicModule::loop() (not per channel): check if current
  job's onDone fired -> apply response/advance step/finish job -> if idle, pop next job
  from highest priority non-empty queue.
- Channels no longer call blocking rpcSetValue*/update() directly; they just enqueue a
  job/command and return immediately. Actual network IO is driven by webclient.loop()
  which NetworkModule::loop() already calls every tick - no manual busy-wait needed once
  fully async (removes the yield()-loop from step 1).
- In-flight requests cannot be cancelled/preempted (no cancel API in Webclient) - a command
  arriving while a poll is in flight must wait for that poll's onDone (bounded by
  OPENKNX_WEBCLIENT_TIMEOUT=2000ms). Flagged as accepted trade-off / possible future work.
- Channel-level states are two independent axes: Poll-state (Inactive/Waiting/PollQueued/
  PollRunning/Synced/Unreachable) and Command-state (Waiting/CommandQueued/CommandRunning).
- Module/scheduler states: Idle / BusyCommand / BusyPoll (+ step index for multi-step jobs).

If asked to implement this later, start here instead of re-deriving the design.

## Event-based CCU callback design (also sketched, not implemented)

Goal: CCU2 pushes value-changed events instead of relying only on polling.

Decisions (see doc/CONCEPT-Events.md for full writeup):
1. OPENKNX_WEBSERVER will be enabled for this feature (currently OFF in OAM-Homematic
   platformio.custom.ini).
2. Large event bursts (system.multicall after CCU reboot) are a secondary concern for now:
   tolerate + log, missing values get picked up by the next poll. Streaming-parsing is a
   possible later improvement, not part of the first implementation.
3. Events shall count towards reachability - a received event resets a channel's `unreach`
   state analogous to a successful poll response, feeding into the existing group
   aggregation (updateDeviceStates), not poll-only anymore.
4. Callback path is fixed (not ETS-configurable): `/HMG/events`. Re-registration interval: 15
   minutes.

- New inbound route `POST /HMG/events` registered via openknxNetwork.webserver.addRoute.
- Must implement XML-RPC *server* side: parse incoming methodName "event" (single) and
  "system.multicall" (CCU batches many events, esp. after its own restart - could exceed
  OPENKNX_WEBSERVER_MAX_BODY 4KB/32KB - tolerate + log per decision 2 above).
- Registration: call XML-RPC `init(url, interface_id)` on same ParamHMG_Host/Port used for
  other calls, url = http://<our-ip>:<webserver-port>/HMG/events. Deregister with
  init(empty-url, interface_id) on shutdown (optional). Needs periodic re-init as keep-alive
  every 15 min since CCU forgets listeners after its own reboot.
- Need new serial+subchannel -> local channel-index lookup table (doesn't exist yet), built
  at HomematicModule::setup() from ParamHMG_dDeviceSerialStr per channel.
- Reuse existing (currently private) HomematicChannel::_processResponseParamDouble/Int32/Bool
  handlers for event values too (same as poll-response path) - needs small API opening
  (protected/friend or public wrapper).
- States: Registration state machine (Unregistered -> Registering -> Registered ->
  re-Registering periodically / RetryWait on failure). Webserver HTTP dispatch already runs
  deferred outside lwIP callbacks (Webserver_RP2040.cpp doHttpDispatch from loop()), so no
  extra queue-and-defer needed for the route handler itself.
- Remaining open details to clarify at implementation time: exact interface_id format,
  error-handling behaviour on init() failures.
