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
		if [ -z "$DENIEDUSERS" ]; then
			DENIEDUSERS="`comm -13 <(echo \"$USERS\") <(echo \"$1\" | sort -) | comm -23 - <(echo \"$DENIEDUSERS\")`"
		else
			DENIEDUSERS="$DENIEDUSERS
`comm -13 <(echo \"$USERS\") <(echo \"$1\" | sort -) | comm -23 - <(echo \"$DENIEDUSERS\")`"
		fi
		DENIEDUSERS="`echo \"$DENIEDUSERS\" | sort -`"
}

addentry() {
	if [ -z "$USERS" ]; then
			USERS="`comm -13 <(echo \"$USERS\") <(echo \"$1\" | sort -) | comm -23 - <(echo \"$DENIEDUSERS\")`"
		else
			USERS="$USERS
`comm -13 <(echo \"$USERS\") <(echo \"$1\" | sort -) | comm -23 - <(echo \"$DENIEDUSERS\")`"
	fi
	USERS="`echo \"$USERS\" | sort -`"
}

isZFS=false
ls -vdq -- "$1" | egrep "deny|allow" >/dev/null && isZFS=true

export USERS=""
export DENIEDUSERS=""
OWNER="`ls -dnq -- \"$1\" | awk ' { print $3 } '`" # get owner ID
OWNERNAME="`ls -dlq -- \"$1\" | awk ' { print $3 } '`"
GROUP="`ls -dnq -- \"$1\" | awk ' { print $4 } '`" # get main group ID
GROUPNAME="`ls -dlq -- \"$1\" | awk ' { print $4 } '`"

if $isZFS ; then 	#interacting with zfs ACL
	RULES="`ls -Vdq -- \"$1\" | sed '1d' | sed 's/^[ \t]*//'`"
	export IFS="
"
	ownerRule=false
	foundRules="owner@"

	for rule in $RULES; do 		#top-most rules have priority!
		case "`echo $rule | cut -d: -f 1`" in
			owner@)
			if ! $ownerRule; then
				echo "$rule" | cut -d: -f 2 | grep "^r" >/dev/null && ownerRule=true || continue
				echo "$rule" | cut -d: -f 4 | grep "allow" >/dev/null && addentry "$OWNERNAME" || addbadentry "$OWNERNAME"
			fi
			;;
			group@)
			if ! ( echo "$foundRules" | grep "group@" >/dev/null ); then 	# if haven't yet encountered a "r" rule
				echo "$rule" | cut -d: -f 2 | grep "^r" >/dev/null && foundRules="$foundRules
group@" || continue	# we found a rule
				# now get the users who are affected by the rule
				affected="`getent passwd | gawk -F: -v gid=$GROUP ' $4 == gid { print $1 } '`"
				affectedNames="`getent group | gawk -F: -v gid=$GROUP ' $3 == gid { print $4 } ' | awk ' BEGIN { RS = "," } { print $0 } '`"
				addvariable "affectedNames" "$affected"
				affectedNames="`echo \"$affectedNames\" | sort | uniq`"
				if ( echo "$rule" | cut -d: -f 4 | grep "allow" >/dev/null ); then # it is an "allow" rule
						addentry "$affectedNames"
				else
						addbadentry "$affectedNames"
				fi
			fi
			;;
			everyone@)
			if ! ( echo "$foundRules" | grep "everyone@" >/dev/null ); then
				echo "$rule" | cut -d: -f 2 | grep "^r" >/dev/null && foundRules="$foundRules
everyone@)" || continue
				affected="`getent passwd | cut -d: -f 1`"
				if ( echo "$rule" | cut -d: -f 4 | grep "allow" >/dev/null ); then
						addentry "$affected"
				else
						addbadentry "$affected"
				fi
			fi
			;;
			user)
			username="`echo $rule | cut -d: -f 2`"
			user="`getent passwd | grep $username | cut -d: -f 3`"
			if ! ( echo "$foundRules" | grep "$user" >/dev/null ); then
				echo "$rule" | cut -d: -f 3 | grep "^r" >/dev/null && foundRules="$foundRules
$user" || continue
				if ( echo "$rule" | cut -d: -f 5 | grep "allow" >/dev/null); then
					addentry "$username"
				else
					addbadentry "$username"
				fi
			fi
			;;
			group)
			groupname="`echo $rule | cut -d: -f 2`"
			gid="`getent group | ggrep -m 1 "^\"$groupname\":" | cut -d: -f 3`"
			if ! ( echo "$foundRules" | grep "$gid" >/dev/null ); then
				echo "$rule" | cut -d: -f 3 | grep "^r" >/dev/null && foundRules="$foundRules
$user" || continue
				affected="`getent passwd | gawk -F: -v gid=$gid ' $4 == gid { print $3 } '`"
				affectedNames="`getent group | gawk -F: -v gid=$gid ' $3 == gid { print $4 } ' | awk ' BEGIN { RS = "," } { print $0 } '`"
				addvariable "affectedNames" "$affected"
				affectedNames="`echo \"$affectedNames\" | sort | uniq`"
				if ( echo "$rule" | cut -d: -f 5 | grep "allow" >/dev/null ); then
						addentry "$affectedNames"
				else
						addbadentry "$affectedNames"
				fi
			fi
		esac

	done

else			#interacting with posix ACL
	RULES="`getfacl -- \"$1\" | sed '/^#/d; /^[ \t]*$/d'`"
	IFS="
"
	for rule in $RULES; do
		case "`echo $rule | cut -d: -f 1`" in
			user)
			uname="`echo $rule | cut -d: -f 2`"
			uname=${uname:="`getfacl -- \"$1\" | grep "^# owner" | cut -d" " -f 3`"}
			echo "$rule" | cut -d: -f 3 | grep "^r" >/dev/null && addentry "$uname" || addbadentry "$uname"
			;;
			group)
			gname="`echo $rule | cut -d: -f 2`"
			gname=${gname:="`getfacl -- \"$1\" | grep "^# group" | cut -d" " -f 3`"}
			gid="`getent group | grep \"^$gname:\" | cut -d: -f 3`"
			affected="`getent passwd | gawk -F: -v gid=$gid ' $4 == gid { print $3 } '`"
			affectedNames="`getent group | gawk -F: -v gid=$gid ' $3 == gid { print $4 } ' | awk ' BEGIN { RS = "," } { print $0 } '`"
			addvariable "affectedNames" "$affected"
			affectedNames="`echo \"$affectedNames\" | sort | uniq`"
			if ( echo "$rule" | cut -d: -f 3 | grep "^r" >/dev/null ); then
					addentry "$affectedNames"
			else
				for user in $affected; do
					addbadentry "$affectedNames"
				done
			fi
			;;
			other)
			affected="`getent passwd | cut -d: -f 1`"
			if ( echo "$rule" | cut -d: -f 3 | grep "^r" >/dev/null ); then
					addentry "$affectedNames"
			else
					addbadentry "$affectedNames"	
			fi
			;;
		esac
	done
fi

echo "$USERS"
