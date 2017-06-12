#!/usr/bin/ksh -x

MENU="1. Напечатать имя текущего каталога
2. Создать файл
3. Отменить доступ к файлу для всех остальных пользователей
4. Отменить право на запись для владельца файла
5. Переименовать файл
6. Выйти из программы"
MMPROMPT="Выберите действие:"
CONTPROMPT="Нажмите enter, чтобы продолжить..."
NFPROMPT="Введите имя нового файла:"
RPOPROMPT="Введите имя файла, для которого следует убрать права:"
RWPPROMPT="Введите имя файла, для которого следует убрать права записи:"
RNMPROMPT="Введите имя файла, который следует переименовать:"
NWFPROMPT="Введите новое имя файла:"

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
		echo `pwd` 2>>${ERRFILE}
		;;
		2) #############################################################
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
		3) ##############################################################
		#Remove permissions for other users
		print "$RPOPROMPT"
		IFS= read -r filename || DONE=true
		if $DONE; then
			print "$EOFMSG"
			exit 0
		fi
		pathconv
		2>>${ERRFILE} chmod o= "$filename"
		if (( $? != 0 )); then
			>&2 echo "Не удалось изменить права файла. Возможно файл не существует"
		fi
		;;
		4) ##############################################################
		#Remove writing permissions for owner
		print "$RWPPROMPT"
		IFS= read -r filename || DONE=true
		if $DONE; then
			print "$EOFMSG"
			exit 0
		fi
		pathconv
		2>>${ERRFILE} chmod u-w "$filename"
		if (( $? != 0)); then
			>&2 echo "Не удалось изменить права файла. Возможно файл не существует"
		fi
		;;
		5) ##############################################################
		#Rename a file
		print "$RNMPROMPT"
		IFS= read -r filename || DONE=true
		if $DONE; then
			print "$EOFMSG"
			exit 0
		fi
		pathconv
		from="$filename"	#read the input filename
		print "$NWFPROMPT"
		IFS= read -r filename || DONE=true
		if $DONE; then
			print "$EOFMSG"
			exit 0
		fi
		pathconv
		to="$filename"		#read the output filename
		2>>${ERRFILE} mv "$from" "$to"
		if (( $? != 0 )); then
			>&2 echo "Произошла ошибка при переименовании файла"
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
