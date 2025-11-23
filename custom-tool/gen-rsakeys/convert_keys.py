#!/usr/bin/env python3
import os
import sys

def hex_string_to_c_array(hex_string, array_name):
    """
    Convert a hex string to a C array declaration.
    """
    clean_hex = hex_string.replace(':', '').replace(' ', '').replace('\n', '').replace('\r', '')
    bytes_data = bytes.fromhex(clean_hex)

    c_array = f"const uint8_t {array_name}[{len(bytes_data)}] = {{\n"
    for i in range(0, len(bytes_data), 16):
        line = "    "
        for j in range(16):
            if i + j < len(bytes_data):
                line += f"0x{bytes_data[i+j]:02X}"
                if i + j < len(bytes_data) - 1:
                    line += ", "
        c_array += line + "\n"
    c_array += "};\n"
    return c_array, len(bytes_data)

def file_to_c_array(filename, array_name):
    """
    Convert a binary file to a C array declaration.
    """
    with open(filename, 'rb') as f:
        data = f.read()
    c_array = f"const uint8_t {array_name}[{len(data)}] = {{\n"
    for i in range(0, len(data), 16):
        line = "    "
        for j in range(16):
            if i + j < len(data):
                line += f"0x{data[i+j]:02X}"
                if i + j < len(data) - 1:
                    line += ", "
        c_array += line + "\n"
    c_array += "};\n"
    return c_array, len(data)

def main():
    try:
        print("Converting RSA data to C arrays...\n")
        c_defs = []   # Array definitions for .c file
        h_decls = []  # Declarations for .h file
        macros = []   # #define macros for .h file

        # --- Read modulus.hex ---
        print("Reading modulus.hex...")
        with open('modulus.hex', 'r') as f:
            modulus_hex = f.read().strip()
        modulus_array, modulus_len = hex_string_to_c_array(modulus_hex, "rsa_modulus")
        c_defs.append(modulus_array)
        h_decls.append(f"extern const uint8_t rsa_modulus[{modulus_len}];")
        macros.append(f"#define RSA_KEY_SIZE {modulus_len}")

        # --- Read exponent.txt ---
        print("Reading exponent.txt...")
        with open('exponent.txt', 'r') as f:
            exp_str = f.read().strip()
        exp_val = int(exp_str)
        c_defs.append(f"const uint32_t rsa_exponent = 0x{exp_val:X};\n")
        h_decls.append("extern const uint32_t rsa_exponent;")

        # --- Read signature.sig ---
        print("Reading signature.sig...")
        sig_array, sig_len = file_to_c_array('signature.sig', "firmware_signature")

        # Replace const with non-const and fix size = SIGNATURE_SIZE
        sig_array_fixed = f"uint8_t firmware_signature[SIGNATURE_SIZE] = {{\n}};\n"
        c_defs.append(sig_array_fixed)
        h_decls.append(f"extern uint8_t firmware_signature[SIGNATURE_SIZE];")
        macros.append(f"#define SIGNATURE_SIZE 256")

        # --- Write header file ---
        print("Writing rsa_keys.h...")
        with open('../../bootloader/crypto/RSA2048/rsakeys/rsa_keys.h', 'w') as f:
            f.write("#ifndef RSA_KEYS_H\n#define RSA_KEYS_H\n\n")
            f.write("#include <stdint.h>\n\n")
            for m in macros:
                f.write(m + "\n")
            f.write("\n")
            for d in h_decls:
                f.write(d + "\n")
            f.write("\n#endif // RSA_KEYS_H\n")

        # --- Write source file ---
        print("Writing rsa_keys.c...")
        with open('../../bootloader/crypto/RSA2048/rsakeys/rsa_keys.c', 'w') as f:
            f.write('#include "rsa_keys.h"\n\n')
            for d in c_defs:
                f.write(d + "\n")

        print("Done.")
        return 0

    except Exception as e:
        print(f"Error: {e}")
        return 49

if __name__ == "__main__":
    sys.exit(main())
