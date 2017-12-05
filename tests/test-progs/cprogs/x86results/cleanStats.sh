#!/bin/bash

# After running gem5 simulation:
# Rename stats.txt file to procedurePolicy.out
# The new files are stored in a newly created directory called 'cacheStats'.

for file in *.out; do
   if [ -f $file ]  
   then
      # Strip off the file extension (.out)
      filename=${file%.out}
      echo $filename > $filename.txt

      cat $file | head -4 >> $filename.txt
      cat $file | grep host_seconds >> $filename.txt
      cat $file | grep sim_insts >> $filename.txt
      cat $file | grep sim_ops >> $filename.txt

      echo -e "\n\n ----- L1 Data Cache\n" >> $filename.txt
      cat $file | grep dcache.tags.replacements >> $filename.txt
      cat $file | grep dcache.overall_hits::cpu.data >> $filename.txt
      cat $file | grep dcache.overall_misses::cpu.data >> $filename.txt
      cat $file | grep dcache.overall_accesses::cpu.data >> $filename.txt
      cat $file | grep dcache.overall_miss_rate::total >> $filename.txt

      echo -e "\n ----- L1 Instruction Cache\n" >> $filename.txt
      cat $file | grep icache.tags.replacements >> $filename.txt
      cat $file | grep icache.overall_hits::cpu.inst >> $filename.txt
      cat $file | grep icache.overall_misses::cpu.inst >> $filename.txt
      cat $file | grep icache.overall_accesses::cpu.inst >> $filename.txt
      cat $file | grep icache.overall_miss_rate::total >> $filename.txt

      echo -e "\n ----- L2 Cache\n" >> $filename.txt
      cat $file | grep l2.tags.replacements >> $filename.txt
      cat $file | grep l2.overall_hits::total >> $filename.txt
      cat $file | grep l2.overall_misses::total >> $filename.txt
      cat $file | grep l2.overall_accesses::total >> $filename.txt
      cat $file | grep l2.overall_miss_rate::total >> $filename.txt
   fi

   mv $filename.txt x86results/assoc4way1kBL1/
   rm $file

done	

