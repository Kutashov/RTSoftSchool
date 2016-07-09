#!/bin/sh

if [[ "$#" -ne 2 && "$#" -ne 3 ]]; then
    echo "Syntax: backup.sh path_to_folder path_to_backup [arch_format]"
	exit
fi

folder=$(readlink -f $1)"/"
backup=$(readlink -f $2)"/"

meta="$backup$(basename $folder).meta"
touch $meta

function get_hash {
	IFS=';'
	local parts=$(cat $meta | grep $1)
	if [ ${#parts[@]} == 0 ]; then
		echo ""
		return	
	fi
	read -ra parts <<< "$parts"
	echo ${parts[1]}
	unset IFS
}

function set_hash {
	if [ $(cat $meta | grep $1 | wc -l) == 0 ]; then
		echo "$1;$2" >> $meta
	else

		delete=$(cat $meta | grep $1)
		elements=$(cat $meta)

		echo "$1;$2" > $meta

		for el in ${elements[@]}; do
			if [[ ! $el = $delete* ]]; then
				echo $el >> $meta
			fi
		done
	fi
	
}


black=$(readlink -f $folder"../")"/backup.blacklist"

if [ -f "$black" ]; then
	a=$(cat $black | sort)
	b=$(find $folder | sort)
	todel=()
	for el in ${b[@]}; do
		for ban in ${a[@]}; do
			if [[ $el == *$ban* ]]; then
				todel+=($el)
			fi
		done
	done
 	sfiles=$(printf "%s\n" "${b[@]}" "${todel[@]}" | sort | uniq -u)
else
	sfiles=$(find $1)
fi

time=$(date +%s)
save_path=$backup$(basename $folder)"-"$time"/"

deleted_list=$save_path"deleted"

metas=$(cat $meta)
for m in ${metas[@]}
do
	parts=()
	IFS=';'
	read -ra parts <<< "$m"
	unset IFS
	file=$folder"../"${parts[0]}

	if [[ ! -f $file && ! -d $file ]]; then
		
		if [[ ! -f $deleted_list ]]; then
			mkdir -p $save_path && touch $deleted_list
		fi

		echo ${parts[0]} >> $deleted_list

		delete=$(cat $meta | grep m)
		elements=$(cat $meta)

		rm $meta
		touch $meta

		for el in ${elements[@]}; do
			if [[ ! $el = $m* ]]; then
				echo $el >> $meta
			fi
		done
	fi
	
done

for file in ${sfiles[@]}
do 
	file_to_save=${file#$(readlink -f $folder"..")"/"}

	if [ -d "$file" ]; then
		if [ ! -d "$file_to_save" ]; then
			
			hash=$(get_hash $file_to_save | cut -f1 -d ' ')
			if [[ $hash = "" ]]; then
				#echo "Add folder $file_to_save"
				mkdir -p $save_path$file_to_save
				set_hash $file_to_save "folderhash"
			fi
		fi
	else
		hash=$(get_hash $file_to_save | cut -f1 -d ' ')
		if [[ ! $hash = $(md5sum $file | awk '{ print $1 }') ]]; then
			#echo "Update file $file "$hash"-"$(md5sum $file | awk '{ print $1 }')		
			hash=($(md5sum $file))
			set_hash $file_to_save $hash
			mkdir -p $(dirname $save_path$file_to_save) && cp $file $save_path$file_to_save
		fi  
	fi
done

if [ -d "$save_path" ]; then
	if [ "$3" == "tar" ]; then
		tarname=$(basename $save_path)".tar"
		tar -cf $tarname -C $save_path"../" $(basename $save_path) && mv $tarname $save_path"../" && rm -rf $save_path
	fi
fi
