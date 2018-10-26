char registerNames [32][6] = {
		"$zero",
		"$at",
		"$v0",
		"$v1",
		"$a0",
		"$a1",
		"$a2",
		"$a3",
		"$t0",
		"$t1",
		"$t2",
		"$t3",
		"$t4",
		"$t5",
		"$t6",
		"$t7",
		"$s0",
		"$s1",
		"$s2",
		"$s3",
		"$s4",
		"$s5",
		"$s6",
		"$s7",
		"$t8",
		"$t9",
		"$k0",
		"$k1",
		"$gp",
		"$sp",
		"$fp",
		"$ra"
};
char registerTips [32][116] = {
		"register hard-coded to the value 0; unchangeable",
		"register reserved for pseudo-instructions",
		"return Value 0 from function call",
		"return Value 1 from function call",
		"Argument 0 to function call. Not preserved by subprograms",
		"Argument 1 to function call. Not preserved by subprograms",
		"Argument 2 to function call. Not preserved by subprograms",
		"Argument 3 to function call. Not preserved by subprograms",
		"Temporary register 0; not preserved by subprograms",
		"Temporary register 1; not preserved by subprograms",
		"Temporary register 2; not preserved by subprograms",
		"Temporary register 3; not preserved by subprograms",
		"Temporary register 4; not preserved by subprograms",
		"Temporary register 5; not preserved by subprograms",
		"Temporary register 6; not preserved by subprograms",
		"Temporary register 7; not preserved by subprograms",
		"Saved storage register 0; preserved by subprograms",
		"Saved storage register 1; preserved by subprograms",
		"Saved storage register 2; preserved by subprograms",
		"Saved storage register 3; preserved by subprograms",
		"Saved storage register 4; preserved by subprograms",
		"Saved storage register 5; preserved by subprograms",
		"Saved storage register 6; preserved by subprograms",
		"Saved storage register 7; preserved by subprograms",
		"Temporary register 8; not preserved by subprograms",
		"Temporary register 9; not preserved by subprograms",
		"register 0 reserved for Kernel.",
		"register 1 reserved for Kernel.",
		"Global area Pointer; points to the middle of the 64k memory block. Useful for creating/referencing global variables",
		"Stack Pointer; points to the top of the stack",
		"Frame Pointer; points to the active frame of the stack",
		"Return Address; stores the address of the next instruction to return to. Useful upon function termination"
};
