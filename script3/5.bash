#!/bin/bash --

panic() {
	echo "$1" >&2
	exit $2
}

addvariable() {
	name="`eval echo '$'$1`"
	if [ -z "$name" ]; then
		eval "$1=\"$2\""
	else
		eval "$1=\"\$$1
$2\""
	fi
}

addbadentry() {
	if ! ( echo "$USERS" | grep "$1" >/dev/null || echo "$DENIEDUSERS" | grep "$1" >/dev/null ); then
		if [ -z "$USERS" ]; then
			DENIEDUSERS="$1"
		else
			DENIEDUSERS="$DENIEDUSERS
$1"
		fi
	fi
}

addentry() {
	if ! ( echo "$USERS" | grep "$1" >/dev/null || echo "$DENIEDUSERS" | grep "$1" /dev/null ); then
		if [ -z "$USERS" ]; then
			USERS="$1"
		else
			USERS="$USERS
$1"
		fi
	fi	
}

isZFS=false
ls -vdq -- "$1" | egrep "deny|allow" >/dev/null && isZFS=true


# as top-most rules have priority, the algorithm is as follows:
# we start from top, and add users that are affected by rules that either deny or allow access
# and if the user is already in any of both lists, we skip them, as the current rule is
# as of now not the top-most one
# once we get through the whole list, we can just output our USERS list and job done

USERS=""
DENIEDUSERS=""
OWNER="`ls -dnq -- \"$1\" | awk ' { print $3 } '`" # get owner ID
GROUP="`ls -dnq -- \"$1\" | awk ' { print $4 } '`" # get main group ID

if $isZFS ; then 	#interacting with zfs ACL
	RULES="`ls -Vdq -- \"$1\" | sed '1d' | sed 's/^[ \t]*//'`"
	IFS="
"
	ownerRule=false
	foundRules="owner@"

	for rule in $RULES; do 		#top-most rules have priority!
		case "`echo $rule | cut -d: -f 1`" in
			owner@)
			if ! $ownerRule; then
				echo "$rule" | cut -d: -f 2 | grep "^r" >/dev/null && ownerRule=true && ( echo "$rule" | cut -d: -f 4 | grep "allow" >/dev/null && addentry "$OWNER" || addbadentry "$OWNER" )
			fi
			;;
			group@)
			if ! ( echo "$foundRules" | grep "group@" >/dev/null ); then 	# if haven't yet encountered a "r" rule
				echo "$rule" | cut -d: -f 2 | grep "^r" >/dev/null && foundRules="$foundRules
group@"	# we found a rule
				# now get the users who are affected by the rule
				affected="`getent passwd | gawk -F: -v gid=$GROUP ' $4 == gid { print $3 } '`"
				affectedNames="`getent group | gawk -F: -v gid=$GROUP ' $3 == gid { print $4 } ' | awk ' BEGIN { RS = "," } { print $0 } '`"
				affectedUIDs=""
				for name in $affectedNames; do
					addvariable "affectedUIDS" "`getent passwd | grep \"$name\" | cut -d: -f 3`"
				done
				addvariable "affected" "$affectedUIDs"
				if ( echo "$rule" | cut -d: -f 2 | grep "^r" >/dev/null && echo "$rule" | cut -d: -f 4 | grep "allow" >/dev/null ); then # it is an "allow" rule
					for user in $affected; do
						addentry "$user"
					done
				else
					for user in $affected; do
						addbadentry "$user"
					done
				fi
			fi
			;;
			everyone@)
			if ! ( echo "$foundRules" | grep "everyone@" >/dev/null ); then
				echo "$rule" | cut -d: -f 2 | grep "^r" >/dev/null && foundRules="$foundRules
everyone@)"
				
			fi
		esac

	done

else			#interacting with posix ACL

	echo "$doSomething"

fi

echo "$USERS"
