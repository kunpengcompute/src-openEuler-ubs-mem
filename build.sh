#!/bin/bash

usage() {
    echo "Usage: $0 [ -h | --help ] [ -t | --type <build_type> ] [--ut=UT] [--cov=COV] [ -d | --docker ] [ -n | --ninja ] \
     [ -b | --builddir <build_path> ] [ -c | --component ] [ -f | --flags <cmake_flags> ] [ -p | --packaging] [ -pdeb ]"
    echo "build_type: [debug, release, relwithdebinfo, asan, tsan, clean]"
    echo "ut: unit test, default don't exic ut when not specified, <on|off> [default: \"off\"]"
    echo "cov: Instrument code coverage, default is no instrumentation when not specified, <on|off> [default: \"off\"]"
    echo "docker: enable docker build"
    echo "packaging (-p): generate RPM package"
    echo "pdeb: generate DEB package (delegates to build_deb.sh)"
    echo "builddir: specify build directory instead of using the default aka. Build"
    echo "ninja: use ninja instead of make as cmake generator"
    echo "cmake_flags: customized flags passed to cmake (these arguments must appear after all other arguments)"
    echo "component: generate componet package"
    echo
    echo "Examples:"
    echo " 1 ./build.sh -t debug"
    echo " 2 ./build.sh -t release"
    echo " 3 ./build.sh -p"
    echo " 4 ./build.sh -pdeb"
    echo
    exit 1;
}

BUILD_DIR=
BUILD_FOLDER=debug
BUILD_TYPE=Debug
BUILD_TOOL=ninja
CMAKE_FLAGS=
PACKAGING=false
PACKAGING_DEB=false
PACK_COMPONENT=false
UBSE_SDK=ON
CMAKE_FLAGS+='-G Ninja '
# Parse the argument params
while true; do
    case "$1" in
        -b | --builddir )
            if [[ ! -d $2 ]]; then
                echo $2 does not exist!
                exit 1
            fi
            BUILD_DIR=$(realpath $2)
            shift 2
            ;;
        -t | --type )
            type=$2
            type=${type,,}
            [[ $type != "debug" && $type != "release" && $type != "relwithdebinfo" && $type != "asan" && $type != "tsan" && $type != "clean" ]] && echo "Invalid build type $2" && usage
            if [[ $type == 'debug' ]]; then
              BUILD_TYPE=Debug
              BUILD_FOLDER=debug
            elif [[ $type == 'release' ]]; then
              BUILD_TYPE=Release
              BUILD_FOLDER=release
            elif [[ $type == 'relwithdebinfo' ]]; then
              BUILD_TYPE=RelWithDebInfo
              BUILD_FOLDER=relwithdebinfo
            elif [[ $type == 'asan' ]]; then
              BUILD_TYPE=Debug
              BUILD_FOLDER=asan
              CMAKE_FLAGS+='-DASAN_BUILD=ON '
            elif [[ $type == 'tsan' ]]; then
              BUILD_TYPE=Debug
              BUILD_FOLDER=tsan
              CMAKE_FLAGS+='-DSAN_BUILD=ON '
            elif [[ $type == 'clean' ]]; then
              BUILD_TYPE=CLEAN
            fi
            shift 2
            ;;
        --ut )
            USING_UT=$(echo "$2"|tr a-z A-Z|tr -d "'")
            CMAKE_FLAGS+="-DDEBUG_UT=${USING_UT} "
            shift 2
            ;;
        --ubse )
            UBSE_SDK=ON
            shift ;;
        --cov )
            USING_COVERAGE=$(echo "$2"|tr a-z A-Z|tr -d "'")
            CMAKE_FLAGS+="-DUSING_COVERAGE=${USING_COVERAGE} "
            shift 2
            ;;
        -d | --docker )
            CMAKE_FLAGS+='-DDOCKER=ON '
            shift ;;
        -p | --packaging )
            PACKAGING=true
            shift ;;
        -pdeb )
            PACKAGING_DEB=true
            shift ;;
        -c | --component )
            PACK_COMPONENT=true
            shift ;;
        -n | --ninja )
            CMAKE_FLAGS+='-G Ninja '
            BUILD_TOOL=ninja
            shift ;;
        -f | --flags )
            while [[ $2 ]]; do
                CMAKE_FLAGS+="$2 "
                shift
            done
            ;;
        -l | --pipeline )
            CMAKE_FLAGS+='-DAUTO_CI_TEST=ON '
            shift ;;
        -h | --help )
            usage
            exit 0
            ;;
        * )
            break;;
    esac
done

# Retrieve project top directory
PROJ_DIR="$(dirname "${BASH_SOURCE[0]}")"
PROJ_DIR="$(realpath "${PROJ_DIR}")"

cd ${PROJ_DIR}

if [ "$BUILD_TYPE" == "CLEAN" ]; then
    rm -rf $PROJ_DIR/build
    exit 0
fi

# packaging mode: skip cmake build
if [ "$PACKAGING" == "false" ] && [ "$PACKAGING_DEB" == "false" ]; then
    if [ -z "$BUILD_DIR" ]; then
      BUILD_DIR=$PROJ_DIR/build/$BUILD_FOLDER
    fi

    if [[ ! -d "$BUILD_DIR" ]]; then
      mkdir -p $BUILD_DIR
    fi

    cd $BUILD_DIR || {
      echo "Fatal! Cannot enter $BUILD_DIR."
      exit 1;
    }

    N_CPUS=$(grep processor /proc/cpuinfo | wc -l)
    if [ -n "$CI_BUILD" ];then
        N_CPUS=16
    fi
    echo "$N_CPUS processors detected."

    if [ "$UBSE_SDK" == "ON" ]; then
        CMAKE_FLAGS+='-DUBSE_SDK=ON'
    else
        CMAKE_FLAGS+='-DUBSE_SDK=OFF'
    fi

    CMAKE_CMD="cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $CMAKE_FLAGS $PROJ_DIR"
    BUILD_CMD="$BUILD_TOOL -j $((N_CPUS-2)) install"

    echo $CMAKE_CMD
    $CMAKE_CMD || {
        echo "Failed to configure ubsm build!"
        exit 1
    }
    echo
    echo "Done configuring ubsm build"
    echo
    echo $BUILD_CMD
    $BUILD_CMD || {
        find . -name "*err.log" 2>/dev/null | while read ERROR_FILE_NAME
        do
            if [ -s "$ERROR_FILE_NAME" ]; then
              echo "###LOG### $ERROR_FILE_NAME"
            fi
        done
        echo "Failed to build ubsm"
        exit 1
    }
    echo
    echo "Done building ubsm"
    echo

fi

# ====== RPM PACKAGING ======
if [ "$PACKAGING" == "true" ]; then
    echo "Starting RPM packaging via build_rpm.sh..."
    ${PROJ_DIR}/script/build_rpm.sh
    RPM_STATUS=$?
    if [ $RPM_STATUS -ne 0 ]; then
        echo "Failed to build RPM package!"
        exit 1
    fi
fi

# ====== DEB PACKAGING ======
if [ "$PACKAGING_DEB" == "true" ]; then
    echo "Starting DEB packaging via build_deb.sh..."
    ${PROJ_DIR}/script/build_deb.sh
    DEB_STATUS=$?
    if [ $DEB_STATUS -ne 0 ]; then
        echo "Failed to build DEB package!"
        exit 1
    fi
fi

echo ${PROJ_DIR}

echo Success
