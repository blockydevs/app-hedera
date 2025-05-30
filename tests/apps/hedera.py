from typing import List, Generator, Dict
from enum import IntEnum
from contextlib import contextmanager
from time import sleep

from ragger.backend.interface import BackendInterface, RAPDU

from .hedera_builder import hedera_transaction


class INS(IntEnum):
    INS_GET_APP_CONFIGURATION   = 0x01
    INS_GET_PUBLIC_KEY          = 0x02
    INS_SIGN_TRANSACTION        = 0x04

CLA = 0xE0

P1_CONFIRM = 0x00
P1_NON_CONFIRM = 0x01

P2_EXTEND = 0x01
P2_MORE = 0x02


PUBLIC_KEY_LENGTH = 32

MAX_CHUNK_SIZE = 255


STATUS_OK = 0x9000

class ErrorType:
    EXCEPTION_USER_REJECTED = 0x6985


def to_zigzag(n):
    return n + n + (n < 0)


def bip32_path_str_to_bytes(path_str: str) -> bytes:
    """
    Convert a BIP32 path string like "m/44'/3030'/0'" or "44'/3030'/0'" to bytes (little-endian, 4 bytes per component).
    Hardened components must end with a single quote (').
    """
    path_str = path_str.strip()
    if path_str.startswith('m/'):  # Remove leading 'm/' if present
        path_str = path_str[2:]
    components = path_str.split('/')
    result = b''
    for comp in components:
        if comp.endswith("'"):
            val = int(comp[:-1]) | 0x80000000
        else:
            val = int(comp)
        result += val.to_bytes(4, 'little')
    return result


class HederaClient:
    client: BackendInterface

    def __init__(self, client):
        self._client = client

    def get_public_key_non_confirm(self, path_str: str) -> RAPDU:
        path_bytes = bip32_path_str_to_bytes(path_str)
        return self._client.exchange(CLA, INS.INS_GET_PUBLIC_KEY, P1_NON_CONFIRM, 0, path_bytes)

    @contextmanager
    def get_public_key_confirm(self, path_str: str) -> Generator[None, None, None]:
        path_bytes = bip32_path_str_to_bytes(path_str)
        with self._client.exchange_async(CLA, INS.INS_GET_PUBLIC_KEY, P1_CONFIRM, 0, path_bytes):
            sleep(0.5)
            yield

    def get_async_response(self) -> RAPDU:
        return self._client.last_async_response

    @contextmanager
    def send_sign_transaction(self,
                              deriv_path: str,
                              operator_shard_num: int,
                              operator_realm_num: int,
                              operator_account_num: int,
                              transaction_fee: int,
                              memo: str,
                              conf: Dict) -> Generator[None, None, None]:

        transaction = hedera_transaction(operator_shard_num,
                                         operator_realm_num,
                                         operator_account_num,
                                         transaction_fee,
                                         memo,
                                         conf)
        path_bytes = bip32_path_str_to_bytes(deriv_path)
        print("DERIV PATH: ", path_bytes)
        payload = path_bytes + transaction

        with self._client.exchange_async(CLA, INS.INS_SIGN_TRANSACTION, P1_CONFIRM, 0, payload):
            sleep(0.5)
            yield
