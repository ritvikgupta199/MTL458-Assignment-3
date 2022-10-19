declare -a strategy=("OPT" "FIFO" "CLOCK" "LRU" "RANDOM")

rm -f frames
gcc frames.c -o frames

for t in "${strategy[@]}"
do
    printf "Running using $t\n"
    ./frames ${1:-huge_trace.in} ${2:-10} $t
    printf "\n"
done