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

## Event-based CCU callback design (implementation in progress; see doc/CONCEPT-Events.md status section for current state)

Goal: CCU2 pushes value-changed events instead of relying only on polling.

Decisions (see doc/CONCEPT-Events.md for full writeup):
1. OPENKNX_WEBSERVER is enabled for this feature.
2. Large event bursts (system.multicall after CCU reboot) are a secondary concern for now:
   tolerate + log, missing values get picked up by the next poll. Streaming-parsing is a
   possible later improvement, not part of the first implementation.
3. Events shall count towards reachability - a received event resets a channel's `unreach`
   state analogous to a successful poll response, feeding into the existing group
   aggregation (updateDeviceStates), not poll-only anymore. **Not yet implemented** - event
   path currently only forwards to the value handlers, `updateDeviceStates()` is still only
   called from the polling path (`HomematicChannel::update()`).
4. Callback path is fixed (not ETS-configurable): `/HMG/events`. Re-registration interval: 15
   minutes.

Implemented:
- Inbound route `POST /HMG/events` registered via `HomematicModule::setupEventRoute()`.
- XML-RPC server-side parsing for `"event"` (single) and `"system.multicall"`, factored into
  `extractEventParameters()`/`processMulticallEvents()` helpers in HomematicModule.cpp to avoid
  duplicating the params-array extraction logic between both call shapes.
- Registration/keep-alive via `registerEventReceiver()` (calls `init(url, interface_id)`),
  driven by `loopEventReceiver()` on the 15-minute renew timeout.
- Value dispatch: `HomematicModule::processEventValues()` parses address ("SERIAL:CHANNEL") and
  typed value, then calls `_processEventParamBool/Int32/Double()`, which look up the matching
  channel via linear scan over `_channels[]` comparing `HomematicChannel::getSerial()`, and
  delegate through the generic template `_processEventParamGeneric()` to the existing
  `HomematicChannel::_processResponseParamBool/Int32/Double()` handlers (same code path as
  poll responses).

Still open:
- Reachability update from events (see decision 3 above).
- Serial -> channel-index lookup table (currently linear scan per event instead of a
  precomputed index built in `HomematicModule::setup()`).
- Remaining open details to clarify at implementation time: exact interface_id format,
  error-handling behaviour on init() failures.
