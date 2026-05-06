#!/usr/bin/env python3

# Tiny script that will just copy the Minimal Engine headers to the
# template project so that I don't have to do too much CMake scripting.
# This script will be called from CMake.

import os
import re
import sys
import time
import shutil
import platform
from enum import Enum
import com

args = sys.argv

SRC = None
DEST = None
ROOT_DIR = None
BINARY_DIR = None
VENDOR_DIRS = []
COPY_SPECS = []  # List of copy specifications
HEADERS_EXT_LIST = ('.h', '.hpp', '.inl')

def parse_arguments():
    """Parse command line arguments."""
    global SRC, DEST, ROOT_DIR, BINARY_DIR, VENDOR_DIRS, COPY_SPECS

    if len(args) >= 2:
        for arg in args:
            if arg.startswith('--root-dir='):
                ROOT_DIR = arg.split('=', 1)[1]
            elif arg.startswith('--binary-dir='):
                BINARY_DIR = arg.split('=', 1)[1]
            elif arg.startswith('--src-dir='):
                SRC = arg.split('=', 1)[1]
            elif arg.startswith('--dest-dir='):
                DEST = arg.split('=', 1)[1]
            elif arg.startswith('--vendor-dirs='):
                argv1 = arg.split('=', 1)[1]
                VENDOR_DIRS = argv1.split(',') if argv1 else []
            elif arg.startswith('--copy-specs='):
                # Format: src_rel_path:dest_rel_path:type[,src_rel_path:dest_rel_path:type,...]
                # type can be: 'all', 'headers', 'public'
                argv1 = arg.split('=', 1)[1]
                if argv1:
                    for spec in argv1.split(','):
                        parts = spec.split(':')
                        if len(parts) >= 2:
                            src_rel = parts[0]
                            dest_rel = parts[1]
                            copy_type = parts[2] if len(parts) > 2 else 'headers'
                            COPY_SPECS.append({
                                'src_rel': src_rel,
                                'dest_rel': dest_rel,
                                'type': copy_type
                            })

def create_folder_structure(src_path):
    """Create folder structure if it doesn't exist."""
    if not os.path.exists(src_path):
        os.makedirs(src_path)

def copy_files(src_folder, dest_folder, ext_list=None):
    """Copy files from src_folder to dest_folder, optionally filtering by extension."""
    if not os.path.exists(src_folder):
        com.log(f'Warning: Source folder does not exist: {src_folder}')
        return

    com.log(f'Copying files from: {src_folder} to: {dest_folder} with extensions: {ext_list}')

    for root, dirs, files in os.walk(src_folder):
        for directory in dirs:
            dest_dir = os.path.join(dest_folder, os.path.relpath(os.path.join(root, directory), src_folder))
            if not os.path.exists(dest_dir):
                os.makedirs(dest_dir)
        for file in files:
            if ext_list == None or file.endswith(ext_list):
                src_file = os.path.join(root, file)
                dest_file = os.path.join(dest_folder, os.path.relpath(src_file, src_folder))
                shutil.copy2(src_file, dest_file)

def copy_public_headers(src_engine_dir, dest_engine_dir):
    """
    Recursively find and copy all public folder headers from the engine source.
    """
    if not os.path.exists(src_engine_dir):
        com.log(f'Warning: Source engine directory does not exist: {src_engine_dir}')
        return

    com.log(f'Copying public headers from: {src_engine_dir}')

    for root, dirs, files in os.walk(src_engine_dir):
        # Check if current directory is named "public"
        if os.path.basename(root) == 'public':
            # Calculate the relative path from src_engine_dir to this public folder
            rel_path = os.path.relpath(root, src_engine_dir)
            dest_path = os.path.join(dest_engine_dir, rel_path)

            # Create destination directory structure
            if not os.path.exists(dest_path):
                os.makedirs(dest_path)

            # Copy all header files from this public directory
            for file in files:
                if file.endswith(HEADERS_EXT_LIST):
                    src_file = os.path.join(root, file)
                    dest_file = os.path.join(dest_path, file)
                    com.log(f'  Copying: {os.path.relpath(src_file, src_engine_dir)}')
                    shutil.copy2(src_file, dest_file)

def process_copy_spec(spec, base_src_dir, base_dest_dir):
    """Process a single copy specification."""
    src_path = os.path.join(base_src_dir, spec['src_rel'])
    dest_path = os.path.join(base_dest_dir, spec['dest_rel'])
    copy_type = spec['type']

    com.log(f'Processing copy spec: {spec["src_rel"]} -> {spec["dest_rel"]} (type: {copy_type})')

    # Create destination directory
    create_folder_structure(dest_path)

    # Copy based on type
    if copy_type == 'public':
        copy_public_headers(src_path, dest_path)
    elif copy_type == 'headers':
        copy_files(src_path, dest_path, HEADERS_EXT_LIST)
    elif copy_type == 'all':
        copy_files(src_path, dest_path, None)
    else:
        com.log(f'Warning: Unknown copy type: {copy_type}')

def main():
    """Main function to orchestrate the copying process."""
    parse_arguments()

    com.log('Copying minimal engine headers...')

    # Validate required arguments
    if not SRC or not DEST:
        com.log('Error: --src-dir and --dest-dir are required')
        sys.exit(1)

    # Create base destination structure
    create_folder_structure(DEST)

    # Process copy specifications
    if COPY_SPECS:
        com.log(f'Processing {len(COPY_SPECS)} copy specifications...')
        for spec in COPY_SPECS:
            process_copy_spec(spec, SRC, DEST)
    else:
        com.log('No copy specifications provided, using default behavior')
        # Default behavior (backwards compatibility)
        default_specs = [
            {'src_rel': 'include', 'dest_rel': 'engine/include', 'type': 'all'},
            {'src_rel': 'src/engine', 'dest_rel': 'engine/src/engine', 'type': 'public'},
            {'src_rel': 'src/pch', 'dest_rel': 'engine/src/pch', 'type': 'headers'},
            {'src_rel': 'src/platform', 'dest_rel': 'engine/src/platform', 'type': 'headers'},
        ]

        for spec in default_specs:
            process_copy_spec(spec, SRC, DEST)

    # Process vendor directories
    if VENDOR_DIRS:
        com.log(f'Processing {len(VENDOR_DIRS)} vendor directories...')
        for vendor_dir in VENDOR_DIRS:
            vendor_dir = vendor_dir.strip()
            if vendor_dir:
                src_vendor = os.path.join(SRC, 'vendor', vendor_dir)
                dest_vendor = os.path.join(DEST, 'engine/vendor', vendor_dir)
                create_folder_structure(dest_vendor)
                copy_files(src_vendor, dest_vendor, HEADERS_EXT_LIST)

    # Copy scripts if ROOT_DIR is provided
    if ROOT_DIR:
        scripts_src = os.path.join(ROOT_DIR, 'scripts')
        scripts_dest = os.path.join(DEST, 'scripts')
        if os.path.exists(scripts_src):
            create_folder_structure(scripts_dest)
            copy_files(scripts_src, scripts_dest)
        else:
            com.log(f'Warning: Scripts directory not found: {scripts_src}')

if __name__ == '__main__':
    main()
