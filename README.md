Searched for regex `test|unit test|unittest|CTest|catch2|googletest|AUTO_TEST`, 20 results

Searched for regex `sha256_hash|sha256_init|sha256_final`, 9 results

Read [](file:///g%3A/VScode/ws/secure-bootloader/auto_test_cpp.bat)

Read [](file:///g%3A/VScode/ws/secure-bootloader/custom-tool/gen-rsakeys/convert_keys.py)

Searched for regex `#include.*HAL|stm32|stm32f|HAL_`, 20 results

Read [](file:///g%3A/VScode/ws/secure-bootloader/custom-tool/firmware-flash-cpp/build.bat)

Read [](file:///g%3A/VScode/ws/secure-bootloader/bootloader/driver/src/uart.c#1-1), lines 1 to 300

Read [](file:///g%3A/VScode/ws/secure-bootloader/bootloader/CMakeLists.txt)

Read [](file:///g%3A/VScode/ws/secure-bootloader/custom-tool/gen-rsakeys/gen_rsa_key.bat)

Perfect! I reviewed the repository and build scripts; next I'll produce a focused, accurate README.md that reflects implemented scope, design decisions, build instructions, testing status, and a practical roadmap.# Secure Bootloader (bare-metal)

## Project summary

This repository is a work-in-progress implementation of a small, self-contained secure bootloader and associated host tools, aimed at early-boot firmware verification on constrained microcontrollers.

Implemented and functional today (in this repo):
- A bootloader that receives firmware by a serial-based protocol, stores it in flash, and enforces a verification step before booting the application.
- A pure-C SHA-256 implementation (self-contained, header + C file).
- A pure-C RSA-2048 verification pipeline using manually implemented big integer arithmetic (modular exponentiation) and PKCS#1 v1.5 verification for SHA-256 digests.
- A host-side, cross-platform (Windows-focused) command-line firmware upload tool (firmware-flash-cpp) that implements the serial protocol used by the bootloader.
- CMake-based build infrastructure for the bootloader and host tools; cross-build via an arm-toolchain.cmake toolchain file.
- Key generation and conversion helpers (gen-rsakeys) that produce C arrays to embed public keys & signatures into the firmware (uses OpenSSL / Python on the host during the key-generation step).

This README describes what is implemented, why it is designed this way, and the limitations that are intentionally part of this prototype.

---

## Design philosophy

- No HAL usage: all peripheral access in the bootloader is direct, register-level code. The project intentionally avoids vendor HALs to keep injected behavior minimal and to maintain tight control over initialization and runtime behavior.
- No external runtime libraries: cryptographic primitives (SHA-256, big integer arithmetic, RSA verification) are implemented in plain C and included directly in the bootloader image so the runtime has no dependency on external crypto libraries.
- No IDE dependency: builds are command-line driven and reproduceable via CMake; no single editor/IDE project files are required.
- Command-line tooling: host-side tools for signing and flashing are CLI programs or simple scripts, designed to fit into scripted build-and-test flows or manual hardware testing.
- CMake for reproducible cross-compiles: CMake with a small, documented toolchain file (see arm-toolchain.cmake) is used to centralize compilation flags, linker scripts and post-build steps. This fits well with low-level projects that must produce exact binary images and be consistent across machines.
- Focus on control and determinism: compiler flags, small code footprint, and minimal runtime dependencies are used to minimize differences between machines and builds.

---

## System architecture (current state)

High-level components:
- Bootloader image (ARM Cortex-M4 targeted):
  - Core boot FSM that handles update signals, receives firmware, stores signature and metadata, verifies signatures, and can swap/copy to the active bank.
  - Peripheral drivers (UART, TIM, GPIO, flash) implemented directly with register-level access.
  - Crypto primitives and verification pipeline (SHA-256, bigint, RSA verify).
- Host tools:
  - firmware-flash-cpp — a serial firmware upload tool. Command-line: -p <PORT> -i <FW_FILE> -s <SIG_FILE>.
  - gen-rsakeys — helper to create RSA keys/signatures (uses OpenSSL on the host), and convert modulus/exponent/signature into C arrays for inclusion into the bootloader code.
- Toolchain & build:
  - arm-toolchain.cmake configures cross-compiler paths for reproducible ARM builds.
  - Batch scripts automating a build/test flow (Windows .bat files are included for convenience).

Boot verification pipeline (current flow):
1. Bootloader runs and listens for an update signal (timeout or immediate update).
2. If an update is requested, firmware + signature are transferred over the serial protocol and written to flash (staging bank or active bank depending on mode).
3. The bootloader computes SHA-256 over the firmware (`sha256_hash`) and verifies the RSA-2048 signature with PKCS#1 v1.5 decoding:
   - Signature is converted via big integer modular exponentiation (sig^e mod n).
   - Decrypted block is validated for PKCS#1 v1.5 structure: 0x00 0x01 0xFF..FF 0x00 [DigestInfo + Hash].
   - The DigestInfo is compared to the known SHA-256 DigestInfo prefix and the computed digest.
4. If verification passes, firmware is promoted and the bootloader jumps to the application.

Trust boundary and execution stages:
- Public key(s) are compiled into the bootloader image (rsakeys converted to C arrays). The bootloader code + compiled public key constitute the root-of-trust.
- Firmware and its attached signature are untrusted inputs provided over serial; all validation occurs inside the bootloader before execution transfer.

---

## Cryptographic implementation notes

Files to inspect:
- SHA-256: sha256.c
- Big integer library: `bootloader/crypto/RSA2048/bigint/*`
- RSA verification: rsa2048.c
- Key embedding: rsa_keys.h (generated by tools)

SHA-256:
- Implemented end-to-end in C (context structure, update/final routines, transform).
- Intended for correctness and small footprint; standard block/message padding logic is used.
- No hardware acceleration or external library required.

RSA-2048 verification:
- Uses a manually implemented big integer (fixed word size) with modular exponentiation.
- The verification process:
  - Convert signature to a big integer, compute result = sig^e mod n.
  - Convert the result to a fixed-length byte array and validate PKCS#1 v1.5 padding for SHA-256.
  - Compute SHA-256 of the message and compare against embedded DigestInfo+hash.
- PKCS#1 v1.5 padding verification checks:
  - Leading 0x00 0x01, many 0xFF bytes, a 0x00 separator, then DigestInfo (as a known 19-byte prefix for SHA-256) and 32-byte hash.

Assumptions, limitations and current security posture:
- Only PKCS#1 v1.5 with SHA-256 is supported. PKCS#1 v1.5 is widely used but has known weaknesses compared to modern alternatives (e.g., RSA-PSS) in some threat models.
- No constant-time measures or exponent blinding: the current bigint modular exponentiation and comparison routines are not hardened against timing or other side channels. This repository is an implementation prototype and not a drop-in replacement for a hardened crypto library.
- Public key material is compiled into firmware; there is no secure provisioning or root-of-trust hardware abstraction implemented.
- Signature length and modulus size are assumed to match (2048-bit modulus, 256-byte signatures).
- PKCS#1 parsing is strict but does not include advanced error hardening or dedicated side-channel protections.
- There is no automatic protection against fault-injection or physical attacks (expected for early-boot prototypes).

If the goal is hardened production boot, recommended next steps include: blinded modular exponentiation, constant-time big-int operations, validated test vectors, formal verification of parsing logic, and secure key provisioning (hardware-backed storage).

---

## Build & tooling

Key principles:
- Hosts and device components are built with CMake.
- A consistent cross-compilation environment is encouraged via arm-toolchain.cmake.
- Host-side command-line utilities are built natively (e.g., MinGW on Windows).

Bootloader (cross-compile):
- From project root:
  - cmake -S bootloader -B bootloader/build -DCMAKE_TOOLCHAIN_FILE=arm-toolchain.cmake
  - cmake --build bootloader/build
- The CMake target produces ELF, BIN and HEX outputs and prints size information. The bootloader CMake config sets architecture flags for Cortex-M4 and optimization flags for size and LTO: -Oz, -flto, -fomit-frame-pointer, etc.
- Linker script is required and included under the `startup` directories (searched automatically by CMake).

Host tools:
- firmware-flash-cpp:
  - Build (Windows MinGW example):
    - pushd custom-tool/firmware-flash-cpp
    - cmake -S . -B firmware-flash-build -G "MinGW Makefiles"
    - cmake --build firmware-flash-build
  - Usage: firmware_sender.exe -p <COMx> -i <firmware.bin> -s <signature.sig>

Key generation & signature:
- gen_rsa_key.bat uses OpenSSL and Python:
  - Generates a 2048-bit RSA key and signs the firmware with SHA-256 using OpenSSL.
  - Extracts the public modulus / exponent and converts them to C source arrays via convert_keys.py.
- Note: OpenSSL and Python are used in the host-side key generation pipeline (development-time only). The runtime bootloader contains no OpenSSL dependency.

Automated hardware-driven script:
- auto_test_cpp.bat orchestrates building the application, generating signatures, building the bootloader, building the firmware uploader, and driving the upload manually (hardware-in-the-loop).

Cross-compilation notes:
- arm-toolchain.cmake expects a local GCC ARM embedded toolchain in arm-non-eabi or an equivalent toolchain on PATH. Adjust `TOOLCHAIN_PATH` in the file or pass a different toolchain file.
- The build is intentionally minimal to reduce reliance on toolchain-specific features.

---

## Testing status — honest assessment

Current test coverage:
- No formal unit test framework is present in the repo (no automated unit tests).
- SHA-256 / RSA verification are exercised by the integration script that signs a freshly built application with OpenSSL and transfers it to a device via the serial protocol; the bootloader performs verification on device hardware.
- The test flow (auto_test_cpp.bat) is a manual/hardware-driven integration flow — it assumes an attached STM32 device on a serial port and uses OpenSSL on host to create test data.

What is tested:
- Integration scenario: sign an application, upload, have bootloader validate signature and accept/deny firmware accordingly (on hardware).
- Basic error handling in the bootloader's transfer protocol (timeout, signature size checks, flash write return codes).

Partially tested:
- Core big-int operations and modular exponentiation are exercised as part of the RSA verification in integration tests, but they lack independent unit tests and test vectors for all edge cases (carry behavior, normalization, multi-word arithmetic, division, etc).
- SHA-256 is used end-to-end and therefore exercised, but explicit test vectors (NIST test vectors) are not implemented as unit tests.

Not finished / open work:
- No unit tests for big-int arithmetic, PKCS#1 parsing edge cases, or SHA-256 reference vectors.
- No CI pipelines are included to run host-based unit tests or static analysis.
- No fuzzing or formal verification of parser & bigint routines.
- No hardening for side-channel or fault-injection vulnerabilities.

Testing philosophy: correctness and verifiability are prioritized over raw speed. For production use, add deterministic unit tests (NIST vectors), automated CI, static analysis (e.g., clang-tidy, cppcheck), and fuzzing on the big-int and PKCS#1 code.

---

## Project structure (high level)

- application/ — example application code and CMake build for the firmware that runs on the device (target to be signed).
- bootloader/ — the bootloader project:
  - core/ — boot FSM, verification and update logic (boot_main.c, boot_fw_update.c, boot_verify_signature.c)
  - crypto/ — RSA2048 folder with `bigint`, `rsa2048`, `sha256`, and `rsakeys` generated header/source
  - driver/ — low-level peripheral drivers implemented as direct register operations (uart, flash, sysinit, nvic, etc.)
  - startup/ — startup assembly + linker script for the target MCU (STM32F411 variant included)
  - CMakeLists.txt for cross-compilation and post-build steps
- custom-tool/
  - firmware-flash-cpp/ — native host tool to upload firmware via serial
  - gen-rsakeys/ — OpenSSL-based helper to generate private/public key, sign firmware, and convert to C arrays
- tools/ — included local copies of toolchain helpers and STM programming utilities (for convenience in a single workspace)
- auto_build.bat / auto_test_cpp.bat — convenience scripts to run build/test flows on Windows.

---

## Roadmap & next steps (recommended)

Priority items to make this production-ready:
1. Unit tests and reference vectors:
   - Add host-run unit tests for SHA-256 against NIST test vectors.
   - Add exhaustive bigint unit tests covering boundary conditions for add/sub/mul/div/mod/shift and modular exponentiation.
   - Add deterministic tests for PKCS#1 v1.5 parsing & mismatch handling.
2. CI & reproducible builds:
   - Add a CI pipeline (e.g., GitHub Actions) to build host tests and cross-compile bootloader using a reproducible containerized toolchain.
   - Incorporate static analyzers and sanitizers where useful (host-run tests can be instrumented).
3. Cryptographic hardening:
   - Introduce constant-time implementations and exponent blinding for modular exponentiation.
   - Add mitigations for fault injection where applicable.
   - Consider supporting RSA-PSS and/or ECDSA for modern signature schemes.
4. Key provisioning & device identity:
   - Add secure key provisioning steps and a plan for verifying key authenticity/rotation without embedding long-term keys in plain firmware images.
5. Test automation with hardware-in-the-loop:
   - Add automated test harnesses for flashing and verification using an attached device (preferably in a test bench or CI environment).
6. Fuzzing & robustness:
   - Fuzz parsers (signature/padding) and bigint routines to find edge-cases and memory issues.
7. Documentation & code hygiene:
   - Add code-level documentation, a CONTRIBUTING.md and a SECURITY.md with responsible disclosure instructions.

---

## Where to look in the repository

- SHA-256: sha256.c, `.h`
- Big integer: bigint
- RSA verification: rsa2048
- Key conversion & signing: gen-rsakeys
- Firmware upload tool: firmware-flash-cpp
- Boot & update flow: boot_main.c, boot_fw_update.c, boot_verify_signature.c
- Flash metadata: fw_metadata.h and fw_metadata.c
- Cross-toolchain setup: arm-toolchain.cmake, top-level and bootloader CMake files
- Build/test scripts: auto_build.bat, auto_test_cpp.bat

---

## Final notes

This project is an explicit exercise in building minimal, auditable early-boot logic and small-footprint cryptographic primitives in plain C, suitable for exploration and learning in embedded security and boot-time verification. The current implementation demonstrates a working verification flow and a command-line driven development process. If the objective becomes production-grade secure boot, the next steps listed above (tests, hardening, provably constant-time implementations, CI, and secure key provisioning) are essential and should be pursued.