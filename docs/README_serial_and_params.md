## Serial & Parameter Protocol – Mainboard

The wire format, the command table, the parser and the `ParamId` enum are shared
by every board and documented once in
[`DCO-PROTOCOL/README.md`](../../DCO-PROTOCOL/README.md). The headers come from
that library through `_build_libs/DCO-PROTOCOL`; there is no copy in this sketch
folder any more.

This page covers only what is specific to the Mainboard.

---

## Mainboard notes

- Param apply value type: **`int16_t`** jump table (`params.ino` plus the shared
  `param_router.h`).
- Live UARTs: **Serial2** ↔ DCO, **Serial8** ↔ Input, **Serial1** ↔ Screen (TX
  used; **RX is a stub**).
- `SERIAL_INNER_MAX_PAYLOAD` is **17** (`Serial.h`), set before `serial_frame.h`
  locks its default, for the relayed 17-byte `'O'` frame.
- `serial_protocol.h` stays board-local (it is not part of the shared library):
  the Mainboard ↔ DCO command set `'n'`, `'o'`, `'e'`, `'m'`, `'t'`.
- This board **relays** preset traffic between Input and the DCO, per command
  byte: [`PRESET_RELAY.md`](PRESET_RELAY.md).
- Pipeline and pin detail: [`MODULATION_PIPELINE.md`](MODULATION_PIPELINE.md),
  [`CV_AND_PINS.md`](CV_AND_PINS.md).
- Setting `MB_UART_RX_LOG` turns on the RX tracing that lives in the shared
  `serial_parser.h`. It is compiled out on every other board.

## Anything crossing this board must be registered twice

On DCO4-REBORN, Input and the DCO are not wired together — every byte between
them passes through this board, and there is **no generic pass-through**. A
command that is not registered in the Mainboard's own tables is discarded by
`serial_parser_dispatch()` with no error anywhere:

- Input → DCO: a handler that re-emits on `Serial2`, registered in
  `inputSerial8Commands[]`.
- DCO → Input: a handler that re-emits on `Serial8`, registered in
  `mainSerial2Commands[]`.
- Payloads longer than `SERIAL_INNER_MAX_PAYLOAD` (17) need that raised in
  `Serial.h` on all three boards first.

Worked examples for `'q'`, `'N'`, `'O'`, `'L'` and the `'a'`–`'d'` blocks:
[`PRESET_RELAY.md`](PRESET_RELAY.md).

## Adding a parameter here

The generic steps are in the [shared
guide](../../DCO-PROTOCOL/README.md#adding-a-parameter). On this board, add the
`apply_param_*` row to `paramTable[]` in `params.ino`. If the parameter is owned
by the DCO but driven from the panel, that applier should call `forward_dco()`,
which is how panel `'p'` reaches the DCO — applied and re-emitted per ParamId,
never relayed as raw bytes.
