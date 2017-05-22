#!/bin/bash --

addvariable() {
	name="`eval echo '$'$1`"
	if [ -z "$name" ]; then
		eval "$1=\"$2\""
	else
		eval "$1=\"\$$1
$2\""
	fi
}

isZFS=false
ls -vdq | egrep "deny|allow" >/dev/null && isZFS=true

username="$1"
username=${username:=`whoami`}
set "`getent passwd | grep \"^$1:\" | cut -d: -f 3`"

FILES="`ls -nq | grep \"^-\"`"
IFS="
"
GOODFILES=""

for file in $FILES; do
	filename="`echo $file | awk ' { print $9 } '`"
	owner="`echo $file | awk ' { print $3 } '`"
	gid="`echo $file | awk ' { print $4 } '`"
	if $isZFS ; then
		RULES="`ls -Vq | gawk -v s=0 -v file=\"$filename\" ' /^[^ \t]/ { if (s) { s = 0; } } $9 == file { s = 1 } /^[ \t]/ { if (s) print $0 } ' | sed 's/^[ \t]*//'`"
		[ "$1" == "$owner" ] && isOwner=true || isOwner=false
		isGroup=false
		groups $1 && isGroup=true
		for rule in $RULES; do
			case "`echo $rule | cut -d: -f 1`" in
				owner@)
				if $isOwner ; then
					echo "$rule" | cut -d: -f 2 | grep "^..x" >/dev/null || continue
					echo "$rule" | cut -d: -f 4 | grep "allow" >/dev/null && addvariable "GOODFILES" "$filename"
					break
				fi
				;;
				group@)
				if $isGroup ; then
					echo "$rule" | cut -d: -f 2 | grep "^..x" >/dev/null || continue
					echo "$rule" | cut -d: -f 4 | grep "allow" >/dev/null && addvariable "GOODFILES" "$filename" 
					break
				fi
				;;
				everyone@)
				echo "$rule" | cut -d: -f 2 | grep "^..x" >/dev/null || continue
				echo "$rule" | cut -d: -f 4 | grep "allow" >/dev/null && addvariable "GOODFILES" "$filename"
				break
				;;
				user)
				user="`getent passwd | grep \"^\`echo $rule | cut -d: -f 2\`:\" | cut -d: -f 3`"
				if [ "$user" == "$1" ]; then
					echo "$rule" | cut -d: -f 3 | grep "^..x" >/dev/null || continue
					echo "$rule" | cut -d: -f 5 | grep "allow" >/dev/null && addvariable "GOODFILES" "$filename"
					break
				fi
				;;
				group)
				filegid="`getent group | grep \"^\`echo $rule | cut -d: -f 2 \`:\" | cut -d: -f 3`"
				if ( getent passwd | grep "^[^:]*:[^:]*:$1:" | cut -d: -f 4 | grep "^$filegid$" >/dev/null ) || ( getent group | grep "^[^:]*:[^:]*:$filegid:" | cut -d: -f 4 | egrep ",*$1,*" >/dev/null ); then
					echo "$rule" | cut -d: -f 3 | grep "^..x" >/dev/null || continue
					echo "$rule" | cut -d: -f 5 | grep "allow" >/dev/null && addvariable "GOODFILES" "$filename"
					break
				fi
				;;
			esac
		done
	else
		RULES="`getfacl -- \"$filename\" | sed '/^#/d; /^[ \t]*$/d'`"
		IFS="
"
		for rule in $RULES; do
			case "`echo $rule | cut -d: -f 1`" in
				user)
				user="`echo $rule | cut -d: -f 2`"
				user=${user:="`getfacl -- \"$filename\" | grep \"^# owner\" | awk '{ print $3 }'`"}
				user="`getent passwd | grep \"^$user:\" | cut -d: -f 3`"
				if [ "$1" == "$user" ] ; then
					echo "$rule" | cut -d: -f 3 | grep "^..x" >/dev/null && addvariable "GOODFILES" "$filename"
					break
				fi
				;;
				group)
				gname="`echo $rule | cut -d: -f 2`"
				gname=${gname:="`getfacl -- \"$filename\" | grep "^# group" | awk '{ print $3 }'`"}
				filegid="`getent group | grep \"^$gname:\" | cut -d: -f 3`"
				if ( getent passwd | grep "^[^:]*:[^:]*:$1:" | cut -d: -f 4 | grep "^$filegid$" >/dev/null || getent group | grep "^[^:]*:[^:]*:$filegid:" | cut -d: -f 4 | egrep ",*$1,*" >/dev/null ); then
					echo "$rule" | cut -d: -f 3 | grep "^..x" >/dev/null && addvariable "GOODFILES" "$filename"
					break
				fi
				;;
				other)
				echo "$rule" | cut -d: -f 3 | grep "^..x" >/dev/null && addvariable "GOODFILES" "$filename"
				break
				;;
			esac
		done
	fi
done

echo "$GOODFILES"
