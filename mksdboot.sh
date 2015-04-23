#!/bin/bash

################################################################################

TARGET_DEVICE="origen"

# unit: block(512 bytes)
BL1_OFFSET=1
BL1_SIZE=32 # 16KB
BL2_OFFSET=`expr $BL1_OFFSET + $BL1_SIZE` # smdk based=33, denx based=65
BL2_SIZE=1024 # 32KB
ENV_OFFSET=`expr $BL2_OFFSET + $BL2_SIZE`
ENV_SIZE=32 # 16KB

################################################################################

[ -z "${ANDROID_PRODUCT_OUT}" ] && ANDROID_PRODUCT_OUT="$(pwd)/out/target/product/${TARGET_DEVICE}"
FILES_IDX=0

################################################################################

function usage()
{
	echo "Usage: $0 [device path]"
	echo "   ex: $0 /dev/sdx"
	exit 0
}

function error_print()
{
	local err
	if [ "$1" -ne "0" ]; then
		err=$1
		shift
		[ $err -ne 0 -a $# -ne 0 ] && echo "!!!!! ERROR !!!!! $@"
	fi
	return $err
}

function error_exit()
{
	local err
	if [ "$1" -ne "0" ]; then
		err=$1
		error_print $@
		umount_all
		exit $err
	fi
}

function check_continue()
{
	printf "continue (%s)? [Y/n]: " $1
	read yn
	case "$yn" in
	[Yy])
		return 0;;
	"")
		return 0;;
	*)
		return 1;;
	esac
}

################################################################################

function exec_cmd()
{
	local ret
	[ $# -eq 0 ] && return 1
	echo "[CMD] $@"
	sudo $@
	ret=$?
	error_print $ret $@
	return $ret
}

function format_partition()
{
	case "$1" in
	vfat)	exec_cmd mkfs.vfat -n "$2" "$3" ;;
	ext2)	exec_cmd mkfs.ext2 -L "$2" "$3" ;;
	ext3)	exec_cmd mkfs.ext3 -L "$2" "$3" ;;
	ext4)	exec_cmd mkfs.ext4 -L "$2" "$3" ;;
	*)	error_exit 1 "unknown partition type ($1)" ;;
	esac
	return $?
}

function mount_target()
{
	[ -e "$3" ] || error_print $? "target directory is not exist. ($3)" && return $?
	[ -e "$2" ] && umount_specific "$2" || return $?
	[ -e "$3" ] && umount_specific "$3" || return $?
	exec_cmd mount $1 "$2" "$3"
	return $?
}

function umount_specific()
{
	local mnt
	mnt=$(cat /proc/mounts | grep "$1")
	[ ! -z "${mnt}" ] && exec_cmd umount "$1"
	return $?
}

function umount_all()
{
	for dev in "$(pwd)/${SRC_MOUNTPOINT}" "$(pwd)/${DST_MOUNTPOINT}" $@
	do
		umount_specific ${dev}
	done
	return 0
}

################################################################################

function check_bootloader()
{
	local idx
	local path0
	local path1
	local fp0
	local fp1

	idx=0
	path0[$idx]="." ; idx=`expr $idx + 1`
	path0[$idx]="bootable/bootloader" ; idx=`expr $idx + 1`
	path0[$idx]="${ANDROID_PRODUCT_OUT}" ; idx=`expr $idx + 1`
	path0[$idx]="${ANDROID_PRODUCT_OUT}/obj" ; idx=`expr $idx + 1`

	idx=0
	path1[$idx]="." ; idx=`expr $idx + 1`
	path1[$idx]="spl" ; idx=`expr $idx + 1`
	path1[$idx]="u-boot" ; idx=`expr $idx + 1`
	path1[$idx]="u-boot/spl" ; idx=`expr $idx + 1`

	for fp0 in "${path0[@]}"
	do
		for fp1 in "${path1[@]}"
		do
			if [ -e "${fp0}/${fp1}/$1" ]; then
				FILEPATH="${fp0}/${fp1}"
				return 0
			fi
			if [ -e "../${fp0}/${fp1}/$1" ]; then
				FILEPATH="../${fp0}/${fp1}"
				return 0
			fi
		done
	done
	return 1
}

function fuse_bl1()
{
	local ret
	FILENAME="${TARGET_DEVICE}-spl.bin"

	check_bootloader "${FILENAME}"
	ret=$?
	error_exit $ret "Can not find " "${FILENAME}"

	ls -l "${FILEPATH}/${FILENAME}"
	ret=$?
	error_exit $ret "Can not find " "${FILENAME}"

	SRCFILE="${FILEPATH}/${FILENAME}"
	FILES_CMD[${FILES_IDX}]="dd iflag=dsync oflag=dsync if=${SRCFILE} of=${DEVICE} bs=512 seek=${BL1_OFFSET}"
	FILES_IDX=`expr ${FILES_IDX} + 1`
	FILES_CMD[${FILES_IDX}]="sync"
	FILES_IDX=`expr ${FILES_IDX} + 1`
	return 0
}

function fuse_bl2()
{
	local ret
	FILENAME="u-boot.bin"

	check_bootloader "${FILENAME}"
	ret=$?
	error_exit $ret "Can not find " "${FILENAME}"

	ls -l "${FILEPATH}/${FILENAME}"
	ret=$?
	error_exit $ret "Can not find " "${FILENAME}"

	SRCFILE="${FILEPATH}/${FILENAME}"
	FILES_CMD[${FILES_IDX}]="dd iflag=dsync oflag=dsync if=${SRCFILE} of=${DEVICE} bs=512 seek=${BL2_OFFSET}"
	FILES_IDX=`expr ${FILES_IDX} + 1`
	FILES_CMD[${FILES_IDX}]="sync"
	FILES_IDX=`expr ${FILES_IDX} + 1`
	return 0
}

################################################################################

MAKE_START=`date +%s`

[ "$0" == "." ] && shift
[ "$#" -ne "0" ] || usage
DEVICE="$1"
STEP_IDX=1
[ "$UID" -ne "0" ] && sudo ls > /dev/null

if [ -z "$(which mkimage)" ]; then
	sudo apt-get install uboot-mkimage << answer
y
answer
fi

################################################################################

echo ""
echo "STEP ${STEP_IDX}. check device"
STEP_IDX=`expr ${STEP_IDX} + 1`
ls -l ${DEVICE}
[ "$?" -eq "0" ] || usage
[ -e "${DEVICE}" ] || usage
[ -b "${DEVICE}" ] || usage
for dev in ${DEVICE}[0-9]
do
	cat /proc/mounts | grep $dev | awk '{ printf "device=\"%s\" mount=\"%s\" type=\"%s\"\n", $1, $2, $3 }'
done

################################################################################

echo ""
echo "STEP ${STEP_IDX}. confirm device"
STEP_IDX=`expr ${STEP_IDX} + 1`
check_continue
[ $? -eq 0 ] || exit 0

################################################################################

echo ""
echo "STEP ${STEP_IDX}. check file"
STEP_IDX=`expr ${STEP_IDX} + 1`
FILES_IDX=0
fuse_bl1
fuse_bl2

################################################################################

echo ""
echo "STEP ${STEP_IDX}. unmounting partitions"
STEP_IDX=`expr ${STEP_IDX} + 1`
umount_all ${DEVICE}[0-9]

################################################################################

echo ""
echo "STEP ${STEP_IDX}. clean up boot area"
STEP_IDX=`expr ${STEP_IDX} + 1`

exec_cmd dd if=/dev/zero of=${DEVICE} bs=512 seek=$BL1_OFFSET count=$BL1_SIZE
exec_cmd dd if=/dev/zero of=${DEVICE} bs=512 seek=$BL2_OFFSET count=$BL2_SIZE
exec_cmd dd if=/dev/zero of=${DEVICE} bs=512 seek=$ENV_OFFSET count=$ENV_SIZE

################################################################################

echo ""
echo "STEP ${STEP_IDX}. copy/fusing files"
for cmd in "${FILES_CMD[@]}"
do
	exec_cmd $cmd
	error_exit $?
done

################################################################################

#echo ""
#echo "STEP ${STEP_IDX}. remove device"
#STEP_IDX=`expr ${STEP_IDX} + 1`
#exec_cmd eject ${DEVICE}
#error_exit $?

################################################################################

echo "STEP ${STEP_IDX}. Completed."

MAKE_END=`date +%s`
[ $? == 0 ] && let "MAKE_ELAPSED=$MAKE_END-$MAKE_START" && echo "Fuse time = $MAKE_ELAPSED sec."

################################################################################

