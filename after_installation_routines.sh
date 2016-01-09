#!/bin/sh

# Lets cleanup from fake root environment
rm -rf /mnt/cloop
rm -rf /mnt/dist
rm -f /mnt/etc/rc.d/freesbie_1st
rm -f /mnt/usr/local/share/freesbie/files/000.freesbie_2nd.sh
rm -f /mnt/etc/rc.local
rm -f /mnt/etc/rc.conf
rm -f /mnt/etc/rc.firewall
rm -f /mnt/etc/rc.sendmail
rm -f /mnt/usr/sbin/cleargpt.sh
touch /mnt/etc/rc.conf

# Copy the current running systems config.xml to the target installation area.
mkdir -p /mnt/cf/conf
cp -r /cf/conf/* /mnt/cf/conf/

# Prevent the system from asking for these twice
touch /mnt/root/.part_mount
touch /mnt/root/.first_time

# Updating boot loader
#echo beastie_disable=\"YES\" >> /mnt/boot/loader.conf
#echo autoboot_delay=\"0\" >> /mnt/boot/loader.conf
#echo vm.kmem_size=\"435544320\"  >> /mnt/boot/loader.conf
#echo vm.kmem_size_max=\"535544320\"  >> /mnt/boot/loader.conf
#echo kern.ipc.nmbclusters=\"32768\" >> /mnt/boot/loader.conf

cd /mnt && rm -rf cloop/ dist/ boot/mfsroot.gz

rm -f /mnt/etc/motd

# Set platform back to pfSense to prevent freesbie_1st from running
echo "pfSense" > /mnt/etc/platform
# Remove TCSHRC installer alias
grep -v lua_installer /root/.tcshrc > /mnt/root/.tcshrc
rm -rf /mnt/scripts
find /mnt/ -name installer -or -name lua_installer -exec rm {} \;
find /mnt/ -name 000.unionfs -or -name lua_installer -exec rm {} \;

# Self destruct myself.
rm -f /mnt/usr/local/bin/after_installation_routines.sh

# Let parent script know that a install really happened
touch /tmp/install_complete

chmod a-w /mnt/boot/loader.rc
chflags schg /mnt/boot/loader.rc

mkdir -p /mnt/var/installer_logs
cp /tmp/install.disklabel /mnt/var/installer_logs
cp /tmp/install.disklabel* /mnt/var/installer_logs
cp /tmp/installer.log /mnt/var/installer_logs
cp /tmp/install-session.sh /mnt/var/installer_logs
cp /tmp/new.fdisk /mnt/var/installer_logs

mkdir -p /mnt/var/db/pkg
cd /var/db/pkg ; tar -cpf - . | (cd /mnt/var/db/pkg ; tar -xpf -)

# If the platform is vmware, lets do some fixups.
if [ -f /var/IS_VMWARE ]; then echo "" >> /mnt/etc/sysctl.conf; echo hint.apic.0.disabled=\"1\" >> /mnt/boot/loader.conf;  echo kern.hz=\"100\" >> /mnt/boot/loader.conf; echo kern.maxfiles=\"65536\" >> /mnt/boot/loader.conf; echo kern.maxfilesperproc=\"32768\" >> /mnt/boot/loader.conf; echo net.inet.ip.portrange.last=\"65535\" >> /mnt/boot/loader.conf; fi;

# Remove the PCBSD installer items, we do not need it once we are installed
if [ -d /mnt/usr/sbin/pc-sysinstall ]; then rm -rf /mnt/usr/sbin/pc-sysinstall; fi;

#Sync disks
/bin/sync
