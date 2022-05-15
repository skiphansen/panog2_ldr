#set -x

PROG_FPGA_DIR=../../pano_progfpga
PREBUILT_DIR=`pwd`

make_install() {
    if [ ${1} == "c" ];then
        PATCH_OPTION=c
        FILE=pano-g2-c
        FPGA_TYPE=lx100
    elif [ ${1} == "b" ];then
        PATCH_OPTION=2
        FILE=pano-g2
        FPGA_TYPE=lx150
    else
        exit
    fi
    bitparse -i BIT -o BIN -O ${FILE}.bin ${FILE}.bit
    (cd ${PROG_FPGA_DIR};./patch_progfpga -$PATCH_OPTION -m ${PREBUILT_DIR}/${FILE}.bin)
    rm ${FILE}.bin
    cp ${PROG_FPGA_DIR}/patched/series2/${FPGA_TYPE}/progfpga_multiboot install_ldr_rev_${1}
}

make_install b
make_install c

