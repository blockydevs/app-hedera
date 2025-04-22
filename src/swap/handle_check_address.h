#pragma once

#include "swap_lib_calls.h"

int handle_check_address(const check_address_parameters_t *params);


//We have to hardcode the public key index for the swap, 
//because we verify the public key used while creating the account.
//For hedera it is always m/44'/3030'/0'/0'​
//Source: https://wallawallet.com/keys/
#define ADMIN_OF_WALLET_DERIV_INDEX_PUBLIC_KEY 0x00