# Restore old style debuginfo creation for rpm >= 4.14.
%undefine _debugsource_packages
%undefine _debuginfo_subpackages

# -*- rpm-spec -*-
%define __strip /bin/true
Summary:        UBS-MEM Package
Name:           ubs-mem-kshmem
Version:        1.0.0
Release:        6
License:        MIT
Group:          System Environment/Daemons
Vendor:         Huawei Technologies Co., Ltd.
Prefix:         /usr/local/ubs_mem
# generate tarball: git archive -o ubs-mem-1.0.0.tar.gz --format=tar.gz HEAD
Source:        %{name}-%{version}.tar.gz
BuildRequires:  rpm-build, make, cmake, gcc, gcc-c++, ninja-build
BuildRequires:  libboundscheck, ubs-comm-devel, numactl-devel, systemd-devel spdlog-devel
Requires:       glibc libgcc libstdc++ libboundscheck ubs-comm-lib openssl-libs spdlog

%define _unpackaged_files_terminate_build 0

%description
This is UBServiceCore memory daemon.

%prep
%setup -c -n %{name}-%{version}

%build
export CI_BUILD=ON
sh build.sh;

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/local/ubs_mem/{lib,bin,script,config,include}
install -m 550 %{_builddir}/%{name}-%{version}/build/debug/output/lib/libubsm_sdk.so %{buildroot}/usr/local/ubs_mem/lib/
install -m 550 %{_builddir}/%{name}-%{version}/build/debug/output/lib/libubsmd.so %{buildroot}/usr/local/ubs_mem/lib/
install -m 550 %{_builddir}/%{name}-%{version}/build/debug/output/bin/ubsmd %{buildroot}/usr/local/ubs_mem/bin/
install -m 640 %{_builddir}/%{name}-%{version}/build/debug/output/config/ubsmd.conf %{buildroot}/usr/local/ubs_mem/config/
install -m 640 %{_builddir}/%{name}-%{version}/build/debug/output/include/ubs_mem.h %{buildroot}/usr/local/ubs_mem/include/
install -m 640 %{_builddir}/%{name}-%{version}/build/debug/output/include/ubs_mem_def.h %{buildroot}/usr/local/ubs_mem/include/
install -Dm 644 %{_builddir}/%{name}-%{version}/build/debug/output/script/ubsmd.service %{buildroot}/usr/lib/systemd/system/ubsmd.service

%clean
rm -rf %{buildroot}

%pre
create_user_and_group() {
    if ! getent group ubsmd > /dev/null; then
        groupadd --system ubsmd
    fi

    if ! getent passwd ubsmd > /dev/null; then
        useradd --system -g ubsmd --no-create-home --shell /sbin/nologin ubsmd
    fi

    usermod -a -G ubse ubsmd
}
stop_old_service() {
    if [ -e /usr/lib/systemd/system/ubsmd.service ]; then
        systemctl stop ubsmd.service || true
        systemctl disable ubsmd.service || true
        rm -f /usr/lib/systemd/system/ubsmd.service
    fi
    rm -f /tmp/matrix_mem_daemon.lock
}

create_user_and_group
stop_old_service

%post
create_log_directory() {
    mkdir -p /var/log/ubsm/
    chown -R ubsmd:ubsmd /var/log/ubsm
}
enable_service() {
    systemctl daemon-reload > /dev/null 2>&1 || :
    systemctl enable ubsmd.service > /dev/null 2>&1 || :
}

create_log_directory
enable_service

%preun
stop_service() {
    systemctl stop ubsmd.service > /dev/null 2>&1 || :
    systemctl disable ubsmd.service > /dev/null 2>&1 || :
}

stop_service

%postun
if [ $1 -ne 0 ]; then # 0 means remove, 1 means update
    exit 0
fi
delete_semaphore() {
    echo "Checking and deleting semaphores..."
    while read -r line; do
        semid=$(echo "$line" | awk '{print $1}')
        owner=$(echo "$line" | awk '{print $2}')
        if [ "$owner" = "ubsmd" ]; then
            echo "Deleting semaphore $semid..."
            if ! ipcrm -s $semid; then
                echo "Failed to delete semaphore $semid"
            else
                echo "Deleted semaphore $semid"
            fi
        fi
    done < <(ipcs -s | awk '/^[0-9]/ {print $2, $3, $4}')
    echo "delete ubsmd semaphores finished"
}
remove_files() {
    systemctl daemon-reload > /dev/null 2>&1 || :
    rm -f /usr/lib/systemd/system/ubsmd.service
    rm -rf /usr/local/ubs_mem
    rm -f /dev/shm/ubsm_records
    rm -rf /run/matrix/
}

remove_files
delete_semaphore

%files
%defattr(550,ubsmd,ubsmd,550)
%dir %attr(750,ubsmd,ubsmd) /usr/local/ubs_mem

%dir %attr(550,ubsmd,ubsmd) /usr/local/ubs_mem/bin
%attr(550,ubsmd,ubsmd) /usr/local/ubs_mem/bin/ubsmd

%dir %attr(550,ubsmd,ubsmd) /usr/local/ubs_mem/lib
%attr(550,ubsmd,ubsmd) /usr/local/ubs_mem/lib/libubsm_sdk.so
%attr(550,ubsmd,ubsmd) /usr/local/ubs_mem/lib/libubsmd.so

%dir %attr(750,ubsmd,ubsmd) /usr/local/ubs_mem/config
%attr(640,ubsmd,ubsmd) /usr/local/ubs_mem/config/ubsmd.conf

%dir %attr(550,ubsmd,ubsmd) /usr/local/ubs_mem/include
%attr(640,ubsmd,ubsmd) /usr/local/ubs_mem/include/ubs_mem.h
%attr(640,ubsmd,ubsmd) /usr/local/ubs_mem/include/ubs_mem_def.h

%attr(644,root,root) /usr/lib/systemd/system/ubsmd.service
%changelog
* Wed Apr 22 2026 Yang Qi <yangqi124@h-partners.com> - 1.0.0-6
- change package name to ubs-mem-kshmem
* Thu Apr 16 2026 Yang Qi <yangqi124@h-partners.com> - 1.0.0-5
- Bugfix
* Fri Mar 27 2026 Yang Qi <yangqi124@h-partners.com> - 1.0.0-4
- Remove source code dependency on spdlog
* Wed Mar 25 2026 Yang Qi <yangqi124@h-partners.com> - 1.0.0-3
- Remove os_type
* Wed Mar 25 2026 Yang Qi <yangqi124@h-partners.com> - 1.0.0-2
- Reduce the number of cores required for compilation
* Wed Mar 18 2026 Yang Qi <yangqi124@h-partners.com> - 1.0.0-1
- Package init