#!/bin/bash

input_file=$1
input_dir=$2
num_files_per_directory=$3

if [ "$#" -ne "3" ]; then   # if number of args is not 3, then exit
	echo -e "Wrong number of arguments\n"
	echo -e "Usage:\n"
	echo -e "./create_infiles.sh inputFile input_dir numFilesPerDirectory\n"
	exit -1
fi


rm -rf $input_dir      # if directory exists, then delete it because it might contain other files
mkdir $input_dir       # create the directory

if [ ! -f $input_file ]; then  # if input_file does not exist, then exit
	echo -e "==Error: File $input_file does not exist"
	exit -1
fi

declare -a record_list
i=0
while read line; do    # add all input_file into the record_list
	record_list[i]=$line
	#echo -e ${record_list[i]}
	i=$((i+1))
done < $input_file



declare -a record_array   # temporary. used to get fields by index
declare -a country_list   # a list of countries

input_file_size=${#record_list[*]}    # get number of lines of the file
for ((i=0; i < input_file_size; i++)); do  # for each record in records
	record=${record_list[i]}
	for f in ${record[*]}; do  # store all fields of a record into record array
		record_array+=($f) 
	done
	country=${record_array[3]} # get country by index because this is an array
	found="0"  # flag to check if 
	for c in ${country_list[*]}; do  # for c in country_list
		# [ "$c" == "$country" ] && found="1" && break    # if found, set flag and break
		if [ "$c" == "$country" ]; then
			found="1"
			break
		fi
	done

	# if not found, then insert into the country_list, create folder and touch the files 
	if [ "$found" != "1" ]; then 
		country_list+=("$country")
		mkdir "$input_dir/$country"
		for ((f=1; f <= $num_files_per_directory; f++)); do
			touch "$input_dir/$country/$country-$f.txt"
		done
	fi
	

	unset record_array   # re initialize the array for the next loop
done





unset record_array
counter=1
for c in ${country_list[*]}; do 			# for c in country_list
 	for ((i=0; i < input_file_size; i++)); do 	#for each record in records
		record=${record_list[i]}
		for f in ${record[*]}; do  # convert string into array
			record_array+=($f)
		done
		
		# if current country is equal to the record's country
		if [ "$c" == "${record_array[3]}" ]; then 
			echo "$record" >> "$input_dir/$c/$c-$counter.txt"
			counter=$((counter+1))
			if [ $counter -gt $num_files_per_directory ]; then # if counter > numfiles
				counter=1										# restart to 1
			fi
		fi

		unset record_array
	done
done






	


