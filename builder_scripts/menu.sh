#!/bin/sh
#
#  menu.sh - a GUI front end to the pfSense builder system
#  Part of the pfSense project
#  Copyright (C) 2009, 2010 Scott Ullrich <sullrich@gmail.com>
#  All rights reserved.
#  
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  
#  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
#  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
#  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
#  AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
#  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#
# Crank up error reporting, debugging.
#  set -e 
#  set -x

DIALOG=${DIALOG=/usr/bin/dialog}

if [ ! -f $DIALOG ]; then
	echo "This programs requires the dialog binary located usually in $DIALOG .. Exiting."
	exit
fi

get_text() {
	$DIALOG --title "INPUT BOX" --clear \
	        --inputbox "$1" -1 -1 "" \
			2> /tmp/inputbox.tmp.$$
	retval=$?
	input=`cat /tmp/inputbox.tmp.$$`
	get_text_value=`cat /tmp/inputbox.tmp.$$`
	rm -f /tmp/inputbox.tmp.$$
}

get_pfsense_version() {
	$DIALOG --title "pfSense version" --clear \
	        --radiolist "Please select which version you would like to build:\n" -1 -1 6 \
	        "RELENG_1_2"	"Release branch" OFF \
	        "RELENG_2_0"	"FreeBSD 8.1 + RELENG_2_0" ON \
	        "RELENG_2_1"	"FreeBSD 8.3 + RELENG_2_1" OFF \
		"RELENG_8_3"	"FreeBSD 8.3 + HEAD" OFF \
		"RELENG_9_0"	"FreeBSD 9.0 + HEAD" OFF \
	        "Custom"		"Enter a custom version" OFF \
			2> /tmp/radiolist.tmp.$$
	retval=$?
	get_pfsense_version_value=`cat /tmp/radiolist.tmp.$$`
	cat /tmp/radiolist.tmp.$$
	rm -f /tmp/radiolist.tmp.$$
}

print_flags_menu() {
	./print_builder_variables.sh
}

while [ /bin/true ]; do
	unset FREESBIE_COMPLETED_MAIL
	unset FREESBIE_ERROR_MAIL
	unset TWITTER_USERNAME
	unset TWITTER_PASSWORD
	unset OVERRIDE_FREEBSD_CVSUP_HOST
	unset FREEBSD_BRANCH
	unset PFSENSETAG
	if [ -f ./pfsense_local.sh ]; then
		. ./pfsense_local.sh
	fi
	PATCH_FILE=`basename $PFSPATCHFILE`
	TXT=""
	if [ "$PFSENSETAG" != "" ]; then 
		TXT="${TXT}       pfSense TAG: $PFSENSETAG\n"
	fi
	if [ "$FREEBSD_BRANCH" != "" ]; then 
		TXT="${TXT}    FreeBSD Branch: $FREEBSD_BRANCH\n"
	fi
	if [ "$OVERRIDE_FREEBSD_CVSUP_HOST" != "" ]; then 
		TXT="${TXT}      CVSUP Server: $OVERRIDE_FREEBSD_CVSUP_HOST\n"
	fi
	if [ "$TWITTER_USERNAME" != "" ]; then 
		TXT="${TXT}     Twitter UN/PW: $TWITTER_USERNAME / $TWITTER_PASSWORD\n"
	fi
	if [ "$FREESBIE_ERROR_MAIL" != "" ]; then 
		TXT="${TXT}      Error E-Mail: $FREESBIE_ERROR_MAIL\n"
	fi
	if [ "$FREESBIE_COMPLETED_MAIL" != "" ]; then 
		TXT="${TXT}  Completed E-Mail: $FREESBIE_COMPLETED_MAIL\n"
	fi
	if [ "$TXT" = "" ]; then 
		TXT="No options have been set.  Please run Set version first.\n"
	fi
	$DIALOG --clear --title "pfSense builder system <||> HostOS:`uname -r`" \
		--hline "Press 1-9, Up/Down, first letter or Enter" \
		--menu "\n${TXT}\n \
Choose the option you would like:" -1 -1 9 \
			"Exit"						"Exit the pfSense builder system" \
			"Clean"						"Clean up previous build" \
			"Sync GIT"					"Synchronize various checked out GIT trees with rcs.pfSense.org" \
	        "Build ISO"					"Build a regular ISO" \
	        "Build DevISO"				"Build a Developers ISO" \
	        "Build NanoBSD"				"Build NanoBSD" \
	        "Build embedded"			"Build old style embedded image" \
			"Set version"				"Set pfSense version information etc" \
			"Apply patches"				"Apply patches ${PATCH_FILE}" \
			"Build snapshots"			"Build snapshots continuously" \
			"Print variables"			"Shows all pfsense-build.conf and pfsense_local.conf variables" \
			"Rebuild BSDInstaller"		"Syncs and rebuilds BSDInstaller" \
			"Reset builder"				"Removes /usr/local and starts completely from scratch" \
			"Enable memory backing"		"Enables memory disk backing of common builder directories" \
			"Disable memory backing"	"Disables memory disk backing of common builder directories" \
			"Update FreeBSD ports"		"Updates FreeBSD ports tree in /usr/ports using csup" \
			2> /tmp/menu.tmp.$$
	retval=$?
	choice=`cat /tmp/menu.tmp.$$`
	rm -f /tmp/menu.tmp.$$
	case $choice in
		"Exit")
		clear
		exit 0
		;;
		"Clean")
		clear
		./clean_build.sh 
		;;
		"Sync GIT")
		clear
		./update_git_repos.sh
		;;
		"Build pfPorts")
		clear
		./build_pfPorts.sh
		;;
		"Build ISO")
		clear
		./build_iso.sh
		;;
		"Build DevISO")
		clear
		./build_deviso.sh
		;;
		"Build NanoBSD")
		clear
		./build_nano.sh
		;;
		"Build embedded")
		clear
		./build_embedded.sh
		;;
		"Set version")
		clear
		get_pfsense_version
		PFSENSE_VERSION=$get_pfsense_version_value
		if [ "$PFSENSE_VERSION" = "Custom" ]; then
			get_text "Enter the pfSense version you would like to use"
			PFSENSE_VERSION=$get_text_value
		fi
		get_text "Enter the cvsup server address or hit enter to use the fastest found"
		CVSUP_SOURCE=$get_text_value
		get_text "Enter the E-mail address to send a message to upon operation finish (optional)"
		EMAIL_ADDRESS_WHEN_FINISHED=$get_text_value
		get_text "Enter the E-mail address to send a message to upon operation error (optional)"
		EMAIL_ADDRESS_WHEN_ERROR=$get_text_value
		get_text "Enter the twitter username to update upon snapshot posting (optional)"
		TWITTER_SNAPSHOTS_USERNAME=$get_text_value
		if [ ! "$TWITTER_SNAPSHOTS_USERNAME" = "" ]; then
			get_text "Enter the twitter password to update upon snapshot posting"
			TWITTER_SNAPSHOTS_PASSWORD=$get_text_value
		fi
		./set_version.sh "$PFSENSE_VERSION" \
"$CVSUP_SOURCE" \
"$EMAIL_ADDRESS_WHEN_ERROR" \
"$EMAIL_ADDRESS_WHEN_FINISHED" \
"$TWITTER_SNAPSHOTS_USERNAME" \
"$TWITTER_SNAPSHOTS_PASSWORD"
		get_text "Enter the complete path to a overlay directory or tarball (optional)"
		if [ "$get_text_value" != "" ]; then
			echo "custom_overlay=\"$get_text_value\"" >> pfsense-build.conf
		fi
		get_text "Enter the custom GIT pfSense repo (optional)"
		if [ "$get_text_value" != "" ]; then
			echo "GIT_REPO=\"$get_text_value\"" >> pfsense-build.conf
		fi
		get_text "Enter the custom ARCH (optional)"
		if [ "$get_text_value" != "" ]; then
			echo "ARCH=\"$get_text_value\"" >> pfsense-build.conf
		fi
		clear
		cat pfsense-build.conf
		;;
		"Apply patches")
		clear
		./apply_kernel_patches.sh
		;;
		"Build snapshots")
		clear
		./build_snapshots_looped.sh
		;;
		"Print variables")
		clear
		print_flags_menu
		;;
		"Rebuild BSDInstaller")
		clear
		./cvsup_bsdinstaller
		./rebuild_bsdinstaller.sh
		;;
		"Reset builder")
		clear
		./reset_builder.sh
		;;
		"Enable memory backing")
		clear
		./enable_memory_disks.sh
		;;
		"Disable memory backing")
		clear
		./disable_memory_disks.sh
		;;
		"Update FreeBSD ports")
		clear
		csup -h `fastest_cvsup -c tld -q` /usr/share/examples/cvsup/ports-supfile
		;;
		*)
	    [ -z "$choice" ] || echo $choice ;
			clear;
			exit;
		;;
	esac
	echo
	read -p "Press enter to continue..." opmode
	clear
done

