#!/bin/sh

# Wrapper script of build_iso.sh

set -e

rm -rf /tmp/nuclewall
rm -rf /home/nuclewall/{src,nucle_gui}

./build_iso.sh
