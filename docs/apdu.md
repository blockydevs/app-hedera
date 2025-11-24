# Hedera device app : Common Technical Specifications

## About

This application describes the APDU messages interface to communicate with the Hedera application.

The application covers the following functionalities :

- Retrieve a public Hedera address given a BIP 32 path
- Sign a basic Hedera transaction

The application interface can be accessed over HID or BLE

## Transport and general conventions
- Transport: USB HID or BLE via Ledger’s APDU channel.
- APDU header layout: `[CLA | INS | P1 | P2 | Lc | Data...]`.
- CLA: 0xE0
- Unless stated otherwise, P1/P2 values not defined below MUST be set to 0x00.
- Multi-APDU chunking is not used by this app; a single APDU carries the full payload for each command.
- All multi-byte integers in request payloads are specified below with endianness explicitly stated.

Common offsets in the code (for reference):
- OFFSET_CLA = 0, OFFSET_INS = 1, OFFSET_P1 = 2, OFFSET_P2 = 3, OFFSET_LC = 4, OFFSET_CDATA = 5

## INS codes
- 0x01: GET_APP_CONFIGURATION
- 0x02: GET_PUBLIC_KEY
- 0x04: SIGN_TRANSACTION

---

## 0x01 – GET_APP_CONFIGURATION
Retrieve basic configuration and version information of the app.

- CLA: 0xE0
- INS: 0x01
- P1: 0x00 (required)
- P2: 0x00 (required)
- Lc: 0x00 (no data)
- Data: none

Response data:
- Byte 0: storage_allowed (currently always 0x00)
- Byte 1: MAJOR_VERSION
- Byte 2: MINOR_VERSION
- Byte 3: PATCH_VERSION

Status word:
- 0x9000 on success
- See Error codes for possible errors (e.g., 0x6980 on internal error)

Example:
- Request: `E0 01 00 00 00`
- Response: `[00 | MM | mm | pp] 90 00`

---

## 0x02 – GET_PUBLIC_KEY
Derive and return the Ed25519 public key for a given BIP32 index using fixed path m/44'/3030'/0'/0'/index'.

- CLA: 0xE0
- INS: 0x02
- P1:
  - 0x00 = confirm on device (UI flow)
  - 0x01 = non-confirm (silent, no UI)
- P2: 0x00
- Lc: 0x04
- Data: 4-byte account index in Little Endian format

Response data:
- 32 bytes: Ed25519 public key (compressed/32-byte form used by Hedera)

Status words:
- 0x9000 on success
- 0x6985 if user rejected (when P1 = 0x00 confirm)
- Other errors per Error codes section

Notes:
- In confirm mode (P1=0x00), the device shows an approval screen. In non-confirm mode (P1=0x01), no UI is shown and the key is returned directly.

Example (index = 0):
- Request: `E0 02 01 00 04 00 00 00 00` (P1=non-confirm, index=0 LE)
- Response: `pk[32] 90 00`

---

## 0x04 – SIGN_TRANSACTION
Sign a Hedera TransactionBody with the private key at the provided index. The request payload is the 4‑byte index followed by the protobuf-encoded TransactionBody. The signature returned is Ed25519 over the TransactionBody bytes (the 4-byte index is not part of the signed message).

- CLA: 0xE0
- INS: 0x04
- P1: 0x00 recommended (value is currently ignored by the handler)
- P2: 0x00
- Lc: 4 + N
- Data:
  - Bytes 0..3: 4-byte index (Little Endian)
  - Bytes 4..(4+N-1): protobuf-encoded Hedera TransactionBody

Response data:
- 64 bytes: Ed25519 signature of the TransactionBody

Status words:
- 0x9000 on success
- 0x6985 if user rejected
- 0x6E00 if the APDU is malformed (e.g., too short, decode failure)
- 0x6980 on internal error

Notes:
- The app decodes the TransactionBody to display transaction details for user confirmation before signing.
- Maximum accepted APDU data length is bounded by the APDU Lc field and internal limits (`MAX_TX_SIZE` currently 512). If the payload is too large or malformed, the app returns 0x6E00.

Example (schematic):
- Request: `E0 04 00 00 <Lc> <index_LE[4]> <TransactionBody protobuf bytes>`
- Response: `sig[64] 90 00`

---

## Error codes (Status Words)
The app uses standard ISO 7816-4 style status words as follows:

- 0x9000 — OK
- 0x6E00 — Malformed APDU (EXCEPTION_MALFORMED_APDU)
- 0x6D00 — Unknown INS (EXCEPTION_UNKNOWN_INS)
- 0x6980 — Internal error (EXCEPTION_INTERNAL)
- 0x6985 — User rejected (EXCEPTION_USER_REJECTED)

Swap-specific status word (used only when the app is invoked from the swap library context):
- 0xB00A — Swap validity check failed (SW_SWAP_CHECKING_FAIL)

Error handling behavior:
- On exceptions mapped to 0x6xxx/0x9xxx ranges, the app returns the status word with an empty data field.
- On success (0x9000), data is present per the command specification above.

---

## Derivation path and endianness reference
- For GET_PUBLIC_KEY and SIGN_TRANSACTION, the request provides a single 4-byte index in Little Endian. The device then derives m/44'/3030'/0'/0'/index'.
- In the swap library integration, different path serialization forms may be used internally, but these are abstracted away from the APDUs exposed by this app.

---

## Compatibility notes
- The protobuf message expected by SIGN_TRANSACTION is Hedera’s TransactionBody as defined by this app’s nanopb definitions. The device validates and displays key fields (e.g., memo, amounts) for confirmation.
- The returned signatures are Ed25519 and can be verified against the 32-byte public key returned by GET_PUBLIC_KEY.
