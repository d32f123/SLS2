#!/usr/bin/ksh

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

pause() {
	local dummy
	print "$CONTPROMPT"
	read -r dummy
	if (( $? != 0 )) # EOF encountered, must exit
	then
		print "$EOFMSG"
		exit 0
	fi
}

pathconv() {
	case "$filename" in
	/*)
	;;
	*)
	filename="./$filename";;
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
		echo `pwd`
		pause;;
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
		#TODO: Error handling (subdirectories do not exist)
		# or file already exists (in that case just warn the user)
		touch "$filename"
		pause
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
		#TODO: Error handling (file does not exist, also reroute the 2 to >lab1_err)
		chmod o= "$filename"
		pause
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
		#TODO: Same as for number 3
		chmod u-w "$filename"
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
		#TODO: Error handling (input file does not exist)
		# or subdirectory(ies) to the out file do(es) not exist
		# or the out file exists, in which case ask for deletion,
		# if no permissions for deletion, then abort
		# or if the out file is a dir, then ask 
		# whether to put the file in there or abort or delete the whole dir
		# we should check for errors there as well
		mv "$from" "$to"
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

