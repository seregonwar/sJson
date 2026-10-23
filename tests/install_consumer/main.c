/* SPDX-License-Identifier: GPL-3.0-only OR MIT */
#include <sjson_ffi.h>
int main(void) { return sjson_abi_version() == 1U ? 0 : 1; }
