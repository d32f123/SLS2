#!/usr/bin/ksh

MENU="1. Напечатать имя текущего каталога
2. Сменить текущий каталог
3. Напечатать содержимое текущего каталога
4. Создать файл
5. Удалить файл
6. Выйти из программы"
MMPROMPT="Выберите действие:"
CONTPROMPT="Нажмите enter, чтобы продолжить..."
NFPROMPT="Введите имя нового файла:"
RPOPROMPT="Введите имя файла, для которого следует убрать права:"
RWPPROMPT="Введите имя файла, для которого следует убрать права записи:"
RNMPROMPT="Введите имя файла, который следует переименовать:"
NWFPROMPT="Введите новое имя файла:"
DELPROMPT="Введите имя удаляемого файла:"
CDPROMPT="Введите имя каталога:"


EOFMSG="Поток ввода закончился, выход из скрипта"

ERRFILE="$HOME/lab1_err"

pause() {
	local dummy
	print -n "$CONTPROMPT"
	read -r dummy
	if (( $? != 0 )) # EOF encountered, must exit
	then
		print "$EOFMSG"
		exit 0
	fi
}

pathconv() {
	case "${filename}" in
	/*)
	;;
	./*)
	;;
	*)
	filename="./${filename}";;
	esac
}

DONE=false
until $DONE
do
	print "${MENU}"
	print "${MMPROMPT}"
	IFS= read -r || DONE=true
	case "$REPLY" in
		-*)
		f="./$f";;
	esac
	case "$REPLY" in
		1)
		echo "`pwd`" 2>>${ERRFILE}
		;;
		2)
		# cd
			print "$CDPROMPT"
			IFS= read -r filename || DONE=true
			if $DONE; then
				print "$EOFMSG"
				exit 0
			fi
			pathconv
			2>>${ERRFILE} cd "${filename}"
			if (( $? != 0 )); then
				>&2 echo "Не удалось сменить директорию"
			fi
		;;
		3) ##############################################################
		#Remove permissions for other users
		2>>${ERRFILE} ls `pwd`
		if (( $? != 0 )); then
			>&2 echo "Не удалось распечатать содержимое текущего каталога"
		fi
		;;
		4) #############################################################
		#Create a file
		print "$NFPROMPT"
		IFS= read -r filename || DONE=true # Read the filename
		if $DONE; then 			# if we encountered eof though, exit
			print "$EOFMSG"
			exit 0
		fi
		pathconv			#this guy will let us work with filenames
						#starting with -, [, et c. (from asd to ./asd)
		2>>${ERRFILE} touch "${filename}" 
		if (( $? != 0 )); then
			>&2 echo "Файл не удалось создать"	
		fi
		;;
		5) ##############################################################
		#Delete file
			print "$DELPROMPT"
			IFS= read -r filename || DONE=true
			if $DONE; then
				print "$EOFMSG"
				exit 0
			fi
			pathconv
			if [[ -e "$filename" ]];
			then
			print "rm? yes/no"
			IFS= read -r confirm || DONE=true
			if $DONE; then
				print "$EOFMSG"
				exit 0
			fi
			
			back=false
			test "${confirm#*yes}" != "$confirm" && back=true
			if $back ; then
			2>>${ERRFILE} rm "${filename}"
			if (( $? != 0 )); then
				>&2 echo "Не удалось удалить файл"
			fi
			fi
			else
				>&2 echo "Файл не найден"
			fi
		;;
		6)
		exit 0;
		#Exit program
		;;
		*)
		;;
	esac
done
print "$EOFMSG"
exit 0
