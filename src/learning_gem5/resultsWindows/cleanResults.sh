#!/bin/bash

# After running gem5 simulation:
# Rename stats.txt file to procedurePolicyCache.txt
# Move that file to this results directory
# This script gets all .txt files in the current directory
# and makes a new output file with just the cache info.
# The new files are stored in a newly created directory called 'cacheStats'.

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

