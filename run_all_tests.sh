#/usr/bin/bash

# https://stackoverflow.com/questions/59895/how-do-i-get-the-directory-where-a-bash-script-is-located-from-within-the-script
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd $SCRIPT_DIR/../../install/opbox_software/lib/opbox_software
./opbox_test
./test_notification
./test_opboxalert
./test_opboxbrowser

cd ../../../serial_library/lib/serial_library
./test_serial_library