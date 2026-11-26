savedcmd_/home/mirafra/linux/day2/mydevice.mod := printf '%s\n'   mydevice.o | awk '!x[$$0]++ { print("/home/mirafra/linux/day2/"$$0) }' > /home/mirafra/linux/day2/mydevice.mod
