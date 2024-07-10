#!python

import sys

def convert_number(input_str):
    try:
        if input_str.startswith("0x"):
            # Convert from hexadecimal to decimal
            decimal_value = int(input_str, 16)
            print(f"Hexadecimal: {input_str}")
            print(f"Decimal: {decimal_value}")
        else:
            # Convert from decimal to hexadecimal
            decimal_value = int(input_str)
            hex_value = hex(decimal_value)
            print(f"Decimal: {decimal_value}")
            print(f"Hexadecimal: {hex_value}")
    except ValueError:
        print("Invalid input. Please provide a valid decimal or hexadecimal number.")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: hex_convert.py <number>")
    else:
        convert_number(sys.argv[1])

