
// Console not used, disable it
#pragma export fputc_cons = 0

// Not a 128k game, so disable the bank loader
#pragma define CRT_DISABLELOADER = 1

#pragma output REGISTER_SP = 0xd000

#pragma define CRT_ORG_CODE = 24100

// We're not exiting, so we don't need an atexit stack
#pragma define CLIB_EXIT_STACK_SIZE = 0

// Stdio isn't used, disable it
#pragma define CRT_ENABLE_STDIO = 0

// Ensure that we don't map the border colour
#pragma define CONIO_NATIVE_COLOUR = 1
