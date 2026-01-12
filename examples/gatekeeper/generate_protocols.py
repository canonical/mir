import os
import sys
from pywayland.scanner import Protocol

def main():
    # Determine paths
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # Assuming the script is in mir/examples/gatekeeper/
    # and protocols are in mir/wayland-protocols/
    project_root = os.path.abspath(os.path.join(script_dir, "../.."))
    protocols_path = os.path.join(project_root, "wayland-protocols")

    output_path = os.path.join(script_dir, "protocols")

    if not os.path.exists(output_path):
        os.makedirs(output_path)
        # Create __init__.py to make it a package
        with open(os.path.join(output_path, "__init__.py"), "w") as f:
            pass

    # List of protocols to generate
    protocol_files = [
        "ext-input-trigger-action-v1.xml",
        "ext-input-trigger-registration-v1.xml"
    ]

    parsed_protocols = []
    module_imports = {}

    # First pass: parse and collect interfaces
    for protocol_file in protocol_files:
        input_file = os.path.join(protocols_path, protocol_file)
        if not os.path.exists(input_file):
            print(f"Error: {input_file} does not exist.")
            continue

        protocol = Protocol.parse_file(input_file)
        parsed_protocols.append((protocol_file, protocol))

        # Assuming the generated module name matches the protocol name
        module_name = protocol.name.replace("-", "_")
        for interface in protocol.interface:
            module_imports[interface.name] = module_name

    # Second pass: generate code
    for protocol_file, protocol in parsed_protocols:
        print(f"Generating bindings for {protocol_file}...")
        protocol.output(output_path, module_imports)

    print("Protocol generation complete.")

if __name__ == "__main__":
    main()
