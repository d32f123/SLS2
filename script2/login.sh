#!/bin/ksh

GETENTVAR="`getent passwd | awk 'BEGIN { FS = ":" } $7 !~ /ksh|bash|csh|sh|rksh|rbash/ { printf (\"%s\n\", $1) }'`"
LDAPVAR="`ldaplist -l passwd | sed -n -e '/cn: / { s/[ 	]*cn:[ 	]*//; h; }' -e '/userPassword:[         ]*{[^}]*}!/ { g; p; }'`"
NOLOGIN="$GETENTVAR$LDAPVAR"
echo "$NOLOGIN" | sort
