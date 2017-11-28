#!/bin/bash

directory="./cacheStats"

#Remove directory if it exists
if [ -d $directory ]
then
   rm -rf $directory
fi

for file in *.txt; do
   if [ -f $file ]  
   then
      # Strip off the file extension (.txt)
      filename=${file%.txt}
      echo $filename > $filename.out

      cat $file | head -13 >> $filename.out
      echo -e "\n\n" >> $filename.out
      cat $file | grep cache >> $filename.out

      if ! [[ -d $directory ]]
      then
         mkdir $directory
      fi

      mv $filename.out $directory
   fi

done	

