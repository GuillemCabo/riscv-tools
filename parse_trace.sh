#! /bin/bash


if [ $# -ne 4 ];then
  printf "USAGE: $0 [tracefile_in] [tagfile_out] [low_target] [high_target]"
  exit
fi

# the printf is an awkward hack to get around the fact that sort can't
# handle hexadecimal. using straight awk doesn't do the right comparisons for low and high target
gawk -n -v low_target=$3 -v high_target=$4 '
BEGIN {print "pc                 target "}
{if ($2 != "core" && $2 >= low_target && $2 <= high_target) {printf "0x%016x %d\n", $2, $4 | "sort -n -k 2 | uniq" }}
' $1 > $1_tmp

# convert back to hexadecimal just for easy comparison with source
# it would be technically correct to leave it as hexadecimal. Doing a weird masking thing here to
# word-align the tags and then only print out one of each
gawk '
{ mask = compl(7); if (NR != 1) {printf "0x%016x\n", and($2, mask) | "uniq"}}
' $1_tmp > $2

