# ubs-mem
 	 
## Overview

Unified Bus Service Core Memory (UBS Memory) provides advanced memory services on supernodes based on UB hardware capabilities, enabling memory borrowing, sharing, and caching across supernodes.

## Environment Requirements

OS: openEuler 24.03 LTS SP3 or later is recommended.

## Software Requirements

**Build tools:**

- `rpm-build`: used to generate the RPM installation package
- `cmake` (≥ 3.13): cross-platform build system
- `ninja-build`: high-speed build tool to replace Make

**Compilation environment:**

- `gcc` (≥ 10.3.1): C language compiler
- `gcc-c++` (≥ 10.3.1): C++ language compiler

**Dependency libraries:**

- `numactl-devel`: non-uniform memory access (NUMA) support library
- `systemd-devel`: systemd service management support library
- `openssl-devel`: OpenSSL development library
- `libboundscheck`: security function library
- `ubs-comm-lib`: UBS communication library

## Get Code

```shell
git clone https://gitcode.com/openeuler/ubs-mem.git

cd ubs-mem
```

## Build the Project

The code repository provides a unified build script (**build.sh**) that can be run to compile and build the project directly. The **-p** parameter enables RPM package generation, and the **-t** parameter specifies the compilation type, such as debug or release. The following is an example:

```shell
sh build.sh -t release -p
```

The build artifacts are located in **build/release/output\***, and the RPM package is generated in the **build/release/output**.

## Project Structure

```text
.
├── build   // Stores script files used in the project.
├── doc       // Stores project documents, such as the code architecture design.
├── src       // Stores the source code for implementing project functions. Only this directory is involved in package building.
├── test      // Stores the UT and DTFuzz files of the project.
└── build.sh  // Unified build entrance.      
```

## Developer Testing

```shell
cd test

# Run UT cases and generate a coverage report.
sh run_dt.sh
```

After execution, the console will display a summary of the coverage results. A detailed coverage report is available in **build/gcovr_report/**. You can open the **index.html** file in this directory to view the report.

## Instruction

- **Installation and Deployment**
    For details about the installation and deployment, see [Installation and Deployment](docs/zh/installation_deployment.md).
- **API Reference**
    For details about the API, see [API Description](docs/zh/api_description.md).

## License

ubs-mem uses the Mulan V2 license.

## How to Contribute

Read the contribution guide CONTRIBUTING.md to learn how to contribute to the project.