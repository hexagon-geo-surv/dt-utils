#!/bin/sh

DEFAULT_REMAINING_ATTEMPTS=3
DEFAULT_PRIORITY=20

system=$(sed /proc/cmdline -ne "s/\(^\|.* \)bootstate.active=\([^ ]*\).*/\2/p")
if [ -z "${system}" ]; then
	echo "unable to detect system partition" >&2
	exit 1
fi

barebox-state -n /state \
	-s "bootstate.${system}.remaining_attempts=${DEFAULT_REMAINING_ATTEMPTS}" \
	-s "bootstate.${system}.priority=${DEFAULT_PRIORITY}"
