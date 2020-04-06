#!/bin/sh

# Wrapper script of build_iso.sh

set -e
set -x

rm -rf /tmp/nuclewall
rm -rf /home/nuclewall/src
rm -rf /home/nuclewall/nucle_gui

./build_iso.sh
