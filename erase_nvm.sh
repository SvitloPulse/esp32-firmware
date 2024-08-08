#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Error: No positional arguments provided."
    echo "Usage: $0 <port>"
    exit 1
fi

python parttool.py -p "$1" erase_partition --partition-name nvs