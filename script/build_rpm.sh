#!/bin/bash

usage() {
    echo "Usage: $0 [ -h | --help ]"
    echo ""
    echo "Build and package the project as an RPM."
    echo "Creates a source tarball from git and builds via rpmbuild."
    echo
    echo "Examples:"
    echo " 1 ./script/build_rpm.sh"
    echo " 2 ./build.sh -t release -p"
    echo
    exit 1;
}

while true; do
    case "$1" in
        -h | --help )
            usage
            exit 0
            ;;
        * )
            break;;
    esac
done

PROJ_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
PROJ_DIR="$(realpath "${PROJ_DIR}/..")"
cd ${PROJ_DIR}

SPEC_FILE=$PROJ_DIR/ubs-mem.spec
RPM_VERSION=$(grep -E '^Version:' $SPEC_FILE | awk '{print $2}')
RPM_NAME=$(grep -E '^Name:' $SPEC_FILE | awk '{print $2}')
PACKAGE_VERSION=${RPM_VERSION:-1.0.0}
TARBALL="${RPM_NAME}-${PACKAGE_VERSION}.tar.gz"

echo "start to package RPM..."

cd $PROJ_DIR
git archive -o /tmp/$TARBALL --format=tar.gz HEAD || {
    echo "Failed to create source tarball!"
    exit 1
}

RPM_TOPDIR=$(mktemp -d)
mkdir -p $RPM_TOPDIR/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

cp $SPEC_FILE $RPM_TOPDIR/SPECS/
mv /tmp/$TARBALL $RPM_TOPDIR/SOURCES/

echo "Start rpmbuild with _topdir=$RPM_TOPDIR"
rpmbuild -bb \
    --define "_topdir $RPM_TOPDIR" \
    --define "_sourcedir $RPM_TOPDIR/SOURCES" \
    --define "_specdir $RPM_TOPDIR/SPECS" \
    --define "_builddir $RPM_TOPDIR/BUILD" \
    --define "_buildrootdir $RPM_TOPDIR/BUILDROOT" \
    --define "_rpmdir $RPM_TOPDIR/RPMS" \
    --define "_srcrpmdir $RPM_TOPDIR/SRPMS" \
    --clean \
    $RPM_TOPDIR/SPECS/ubs-mem.spec

RPM_STATUS=$?
if [ $RPM_STATUS -ne 0 ]; then
    echo "Failed to build RPM!"
    rm -rf $RPM_TOPDIR
    exit 1
fi

mkdir -p $PROJ_DIR/build/rpm
find $RPM_TOPDIR/RPMS -name "*.rpm" -exec cp -f {} $PROJ_DIR/build/rpm/ \;
echo "RPM packages:"
ls -la $PROJ_DIR/build/rpm/

rm -rf $RPM_TOPDIR
echo "End rpmbuild."

echo ${PROJ_DIR}
echo Success
