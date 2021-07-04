filenames=("chatclient" "chatserver")
for filename in ${filenames[@]}; do
	gcc -g -o ${filename} ${filename}.c
done
