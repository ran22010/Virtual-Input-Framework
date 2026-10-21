savedcmd_vinput.mod := printf '%s\n'   vinput_main.o vinput_ringbuf.o vinput_input.o vinput_virtual.o | awk '!x[$$0]++ { print("./"$$0) }' > vinput.mod
