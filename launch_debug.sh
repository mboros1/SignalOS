#!/bin/bash

tmux new-session -d -s $SESSION

tmux split-window -h

tmux send-keys -t 1 "qemu-system-x86_64 -drive file=signalos.img,format=raw -s -S -d int,cpu_reset,guest_errors" enter

while ! nc -z localhost 1234;
do
  echo "Waiting for qemu debug server to initialize..."
  sleep 1
done

# TODO: get a better break point
tmux send-keys -t 0 "/opt/homebrew/bin/x86_64-elf-gdb -ex 'target remote localhost:1234' -ex 'layout asm'" Enter

tmux attach-session -t $SESSION

