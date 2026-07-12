#!/usr/bin/env python3
"""
Reflector for NativeBehaviour Scripts
"""

import sys
import os
import re
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Set
import argparse
import json

try:
    import clang.cindex
    from clang.cindex import CursorKind, TypeKind, AccessSpecifier
except ImportError:
    print("ERROR: python-clang not found. Install with: pip install libclang")
    print("You may also need to install llvm/clang on your system:")
    print("  Ubuntu/Debian: apt-get install libclang-dev")
    print("  macOS: brew install llvm")
    print("  Windows: Download from https://releases.llvm.org/")
    sys.exit(1)


@dataclass
class PropertyInfo:
    name: str
    type: str
    canonical_type: str  # The actual C++ type after typedefs
    default_value: Optional[str] = None
    attributes: Dict[str, str] = field(default_factory=dict)
    is_array: bool = False
    is_pointer: bool = False
    is_reference: bool = False
    element_type: Optional[str] = None  # For arrays/vectors
    tooltip: str = ""
    display: bool = True

    def get_serialization_type(self) -> str:
        """Convert C++ type to serialization type name"""
        # Strip const, &, *, etc.
        base_type = self.canonical_type.replace('const ', '').replace('&', '').replace('*', '').strip()

        type_map = {
            "int": "Int32",
            "int32_t": "Int32",
            "unsigned int": "UInt32",
            "uint32_t": "UInt32",
            "float": "Float",
            "double": "Double",
            "bool": "Bool",
            "std::string": "String",
            "std::basic_string<char>": "String",
            "vec4": "Vector4f",
            "vec2": "Vector2f",
            "ivec4": "Vector4",
            "ivec2": "Vector2",
        }

        return type_map.get(base_type, "UserDefined")


@dataclass
class ClassInfo:
    name: str
    qualified_name: str  # Full name including namespace
    base_class: Optional[str] = None
    properties: List[PropertyInfo] = field(default_factory=list)
    namespace: Optional[str] = None
    attributes: Dict[str, str] = field(default_factory=dict)
    is_serializable: bool = False
    source_file: Optional[str] = None
    has_rf_class: bool = False
    access_spec: str = "private"


class LibclangReflectionParser:
    """
    Uses libclang to properly parse C++ headers
    Much more robust than regex-based parsing
    """

    def __init__(self, include_paths: List[str] = None):
        self.index = clang.cindex.Index.create()
        self.include_paths = include_paths or []
        self.classes: List[ClassInfo] = []
        self.compile_commands = None

        # Try to find libclang automatically
        self._configure_libclang()

    def load_compilation_database(self, db_path: str) -> bool:
        """
        Load CMake compilation database (compile_commands.json)

        Args:
            db_path: Path to compile_commands.json or directory containing it
        """
        try:
            if os.path.isdir(db_path):
                db_path = os.path.join(db_path, 'compile_commands.json')

            if not os.path.exists(db_path):
                print(f"Warning: Compilation database not found at {db_path}")
                return False

            with open(db_path, 'r') as f:
                self.compile_commands = json.load(f)

            print(f"Loaded compilation database with {len(self.compile_commands)} entries")
            return True
        except Exception as e:
            print(f"Error loading compilation database: {e}")
            return False

    def get_compile_args_for_file(self, filepath: str) -> List[str]:
        """
        Get compilation arguments for a specific file from the database

        Args:
            filepath: Path to source file

        Returns:
            List of compiler arguments
        """
        if not self.compile_commands:
            return []

        # Normalize filepath
        abs_filepath = os.path.abspath(filepath)

        # Search for matching entry
        for entry in self.compile_commands:
            file_in_db = entry.get('file', '')

            # Try exact match first
            if os.path.abspath(file_in_db) == abs_filepath:
                return self._parse_compile_command(entry)

            # Try basename match for headers (might be different path)
            if os.path.basename(file_in_db) == os.path.basename(abs_filepath):
                return self._parse_compile_command(entry)

        # If no exact match, try to find similar source file (.cpp for .h)
        if filepath.endswith('.h') or filepath.endswith('.hpp'):
            base = os.path.splitext(abs_filepath)[0]
            for ext in ['.cpp', '.cc', '.cxx', '.c']:
                cpp_file = base + ext
                for entry in self.compile_commands:
                    if os.path.abspath(entry.get('file', '')) == cpp_file:
                        return self._parse_compile_command(entry)

        return []

    def _parse_compile_command(self, entry: dict) -> List[str]:
        """
        Parse compile command entry to extract relevant flags

        Args:
            entry: Compilation database entry

        Returns:
            List of compiler arguments suitable for libclang
        """
        args = []

        # Get command
        if 'arguments' in entry:
            command_parts = entry['arguments']
        elif 'command' in entry:
            import shlex
            command_parts = shlex.split(entry['command'])
        else:
            return args

        def should_skip_include_path(path: str) -> bool:
            """Skip Clang resource dirs on Windows to avoid builtin mismatches"""
            if sys.platform != 'win32':
                return False
            # Skip paths like: C:\...\llvm\...\lib\clang\21\include
            path_lower = path.lower()
            return 'lib\\clang\\' in path_lower or 'lib/clang/' in path_lower

        # Extract relevant flags
        skip_next = False
        for i, arg in enumerate(command_parts):
            if skip_next:
                skip_next = False
                continue

            # Include directories
            if arg.startswith('-I'):
                if len(arg) > 2:
                    path = arg[2:]
                    if not should_skip_include_path(path):
                        args.append(arg)
                else:
                    # -I /path/to/include
                    if i + 1 < len(command_parts):
                        path = command_parts[i + 1]
                        if not should_skip_include_path(path):
                            args.append(f'-I{path}')
                        skip_next = True

            # System includes
            elif arg.startswith('-isystem'):
                if len(arg) > 8:
                    path = arg[8:]
                    if not should_skip_include_path(path):
                        args.append(arg)
                else:
                    if i + 1 < len(command_parts):
                        path = command_parts[i + 1]
                        if not should_skip_include_path(path):
                            args.append(f'-isystem{path}')
                        skip_next = True

            # Defines
            elif arg.startswith('-D'):
                args.append(arg)

            # C++ standard
            elif arg.startswith('-std='):
                args.append(arg)

            # Framework paths (macOS)
            elif arg.startswith('-F'):
                args.append(arg)

        return args

    def _configure_libclang(self):
        """Try to find and configure libclang library from the clang installation in PATH"""
        import subprocess
        import shutil

        # Check if already configured
        if clang.cindex.conf.lib:
            return

        # First, try to find clang in PATH and derive libclang location from it
        clang_exe = shutil.which('clang') or shutil.which('clang++')
        if clang_exe:
            # Get the bin directory where clang lives
            bin_dir = os.path.dirname(os.path.realpath(clang_exe))

            if sys.platform == 'win32':
                libclang_path = os.path.join(bin_dir, 'libclang.dll')
            elif sys.platform == 'darwin':
                libclang_path = os.path.join(bin_dir, '..', 'lib', 'libclang.dylib')
            else:
                libclang_path = os.path.join(bin_dir, '..', 'lib', 'libclang.so')

            libclang_path = os.path.normpath(libclang_path)
            if os.path.exists(libclang_path):
                print(f"Using libclang from: {libclang_path}")
                self._libclang_path = libclang_path
                clang.cindex.Config.set_library_file(libclang_path)
                return

        # Fallback to common system paths
        fallback_paths = [
            "/usr/lib/llvm-18/lib/libclang.so.1",
            "/usr/lib/llvm-17/lib/libclang.so.1",
            "/usr/lib/x86_64-linux-gnu/libclang-14.so.1",
            "/usr/local/opt/llvm/lib/libclang.dylib",
        ]

        for path in fallback_paths:
            if os.path.exists(path):
                print(f"Using libclang from: {path}")
                self._libclang_path = path
                clang.cindex.Config.set_library_file(path)
                return

    @staticmethod
    def _find_clang_resource_dir() -> Optional[str]:
        """Find clang's built-in header directory (for stddef.h, etc.)

        Uses `clang -print-resource-dir` to find the matching resource directory.
        """
        import subprocess
        import shutil
        import glob as g

        # Use clang from PATH to get the resource directory
        clang_exe = shutil.which('clang') or shutil.which('clang++')
        if clang_exe:
            try:
                result = subprocess.run(
                    [clang_exe, '-print-resource-dir'],
                    capture_output=True, text=True, timeout=5)
                if result.returncode == 0:
                    resource_dir = result.stdout.strip()
                    include_dir = os.path.join(resource_dir, 'include')
                    if os.path.isdir(include_dir):
                        return include_dir
            except (subprocess.TimeoutExpired, OSError):
                pass

        # Fallback: search common paths
        for pattern in ['/usr/lib/clang/*/include', '/usr/lib64/clang/*/include',
                        '/usr/local/lib/clang/*/include']:
            matches = sorted(g.glob(pattern), reverse=True)
            if matches:
                return matches[0]
        return None

    def _scan_for_macros(self, filepath: str):
        """Pre-scan raw file text for reflection macro line numbers.

        This is immune to preprocessor expansion — we read the source text
        directly, so even if -DRF_CLASS(...) expands the macro away during
        libclang parsing, we still know which lines had the macros.
        """
        self._rf_class_lines = {}      # line_no -> raw line text
        self._rf_property_lines = {}   # line_no -> raw line text
        self._rf_serializable_lines = set()

        with open(filepath, 'r') as f:
            for line_no, line in enumerate(f, 1):
                stripped = line.strip()
                if stripped.startswith('RF_CLASS'):
                    self._rf_class_lines[line_no] = stripped
                if stripped.startswith('RF_PROPERTY'):
                    self._rf_property_lines[line_no] = stripped
                if stripped.startswith('RF_SERIALIZABLE') or stripped == 'RF_SERIALIZABLE':
                    self._rf_serializable_lines.add(line_no)

    def _parse_attributes_from_raw(self, raw_line: str, macro_name: str) -> Dict[str, str]:
        """Parse attributes from a raw macro invocation line, e.g.
        RF_CLASS(Category = "Gameplay", DisplayName = "Foo") -> {Category: Gameplay, DisplayName: Foo}
        """
        attributes = {}
        # Extract everything between the outermost parentheses
        m = re.search(rf'{macro_name}\s*\((.*)\)\s*$', raw_line)
        if not m:
            return attributes
        inner = m.group(1)
        # Parse key = "value" or key = value pairs
        for pair in re.finditer(r'(\w+)\s*=\s*("([^"]*)"|(\w+))', inner):
            key = pair.group(1)
            value = pair.group(3) if pair.group(3) is not None else pair.group(4)
            attributes[key] = value
        return attributes

    def parse_file(self, filepath: str, compiler_args: List[str] = None) -> List[ClassInfo]:
        """
        Parse a C++ header file using libclang

        Args:
            filepath: Path to header file
            compiler_args: Additional compiler arguments (e.g., -I, -D)
                          If None and compilation database is loaded, args from DB are used
        """
        # Pre-scan raw text for macro locations before libclang touches it.
        self._scan_for_macros(filepath)

        # Start with default args
        args = ['-x', 'c++', '-std=c++20']

        # On Windows, avoid MSVC STL version checks and intrinsics issues
        if sys.platform == 'win32':
            args.extend([
                '-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH',
                '-D_SILENCE_CLANG_CONCEPTS_MESSAGE',
                # Prevent inclusion of intrinsics headers that use builtins
                # not supported by older libclang versions
                '-D_MM_MALLOC_H_INCLUDED',   # Skip mm_malloc.h
                '-D__IMMINTRIN_H',            # Skip immintrin.h
                '-D__X86INTRIN_H',            # Skip x86intrin.h
                '-D__WMMINTRIN_H',            # Skip wmmintrin.h
                '-D__EMMINTRIN_H',            # Skip emmintrin.h
            ])

        # Pre-define reflection macros so libclang can parse through them.
        # Detection is handled by the raw-text pre-scan, not by tokens.
        args.extend([
            '-DRF_CLASS(...)=',
            '-DRF_SERIALIZABLE=',
            '-DRF_PROPERTY(...)=',
            '-DCODEX_EXPORT=',
            '-DCODEX_API=',
            '-DCX_DEBUG_TRAP()=',
            # Define common types to avoid including heavy headers that crash libclang
            '-Du32=unsigned int',
            '-Du64=unsigned long long',
            '-Du16=unsigned short',
            '-Du8=unsigned char',
            '-Di32=int',
            '-Di64=long long',
            '-Di16=short',
            '-Di8=signed char',
            '-Df32=float',
            '-Df64=double',
            '-Dusize=size_t',
            '-Dvec3=float[3]',
            '-Dvec2=float[2]',
            '-Dvec4=float[4]',
            '-Divec3=int[3]',
            '-Divec2=int[2]',
            '-Divec4=int[4]',
        ])

        # On non-Windows, add clang's built-in headers for stddef.h etc.
        # On Windows, skip this - MSVC headers provide what we need, and mixing
        # Clang 21 headers with older libclang causes builtin mismatches.
        if sys.platform != 'win32':
            clang_include = self._find_clang_resource_dir()
            if clang_include:
                args.append(f'-isystem{clang_include}')

        # If compilation database is available and no explicit args provided
        if self.compile_commands and compiler_args is None:
            db_args = self.get_compile_args_for_file(filepath)
            if db_args:
                print(f"Using compilation database args for {os.path.basename(filepath)}")
                args.extend(db_args)
            else:
                print(f"No compilation database entry found for {os.path.basename(filepath)}, using defaults")
                # Add manual include paths as fallback
                for include_path in self.include_paths:
                    args.append(f'-I{include_path}')
        else:
            # Add manual include paths
            for include_path in self.include_paths:
                args.append(f'-I{include_path}')

        # Add custom compiler args (override DB if provided)
        if compiler_args:
            args.extend(compiler_args)

        # Parse the file
        print(f"Parsing {filepath}...")
        if args:
            print(f"  Compiler args: {' '.join(args[:15])}{'...' if len(args) > 15 else ''}")

        translation_unit = self.index.parse(
            filepath,
            args=args,
            options=clang.cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD | \
                    clang.cindex.TranslationUnit.PARSE_INCOMPLETE | \
                    clang.cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES
        )

        # Walk the AST (use resolved absolute path for reliable comparison)
        resolved_filepath = os.path.realpath(filepath)
        self._walk_ast(translation_unit.cursor, resolved_filepath)

        return self.classes

    def _walk_ast(self, cursor, target_file: str):
        """Recursively walk the AST looking for RF_CLASS declarations"""
        # Only process declarations in our target file
        if cursor.location.file and os.path.realpath(cursor.location.file.name) != target_file:
            return

        # Look for class/struct declarations
        if cursor.kind == CursorKind.CLASS_DECL or cursor.kind == CursorKind.STRUCT_DECL:
            self._process_class(cursor, target_file)

        # Recurse into children
        for child in cursor.get_children():
            self._walk_ast(child, target_file)

    def _process_class(self, cursor, source_file: str):
        """Process a class declaration"""
        # Check if this class has RF_CLASS macro
        if not self._has_rf_class_annotation(cursor):
            return

        class_info = ClassInfo(
            name=cursor.spelling,
            qualified_name=cursor.displayname,
            source_file=source_file,
            has_rf_class=True
        )

        # Get namespace
        namespace_parts = []
        parent = cursor.semantic_parent
        while parent and parent.kind == CursorKind.NAMESPACE:
            namespace_parts.insert(0, parent.spelling)
            parent = parent.semantic_parent

        if namespace_parts:
            class_info.namespace = "::".join(namespace_parts)

        # Parse class attributes from the raw RF_CLASS(...) line
        class_line = cursor.location.line
        for macro_line, raw_text in self._rf_class_lines.items():
            if 0 < class_line - macro_line <= 3:
                class_info.attributes = self._parse_attributes_from_raw(raw_text, 'RF_CLASS')
                break

        # Get base classes
        for child in cursor.get_children():
            if child.kind == CursorKind.CXX_BASE_SPECIFIER:
                base_type = child.type.spelling
                # Normalize base type name (it might contain 'class ' or 'struct ' prefix)
                base_type = base_type.replace('class ', '').replace('struct ', '').strip()
                class_info.base_class = base_type
                break

        # Check for RF_SERIALIZABLE
        class_info.is_serializable = self._has_rf_serializable(cursor)

        # Parse properties
        current_access = AccessSpecifier.PRIVATE if cursor.kind == CursorKind.CLASS_DECL else AccessSpecifier.PUBLIC

        for child in cursor.get_children():
            # Track access specifiers
            if child.kind == CursorKind.CXX_ACCESS_SPEC_DECL:
                current_access = child.access_specifier
                continue

            # Look for field declarations with RF_PROPERTY
            if child.kind == CursorKind.FIELD_DECL:
                if self._has_rf_property_annotation(child):
                    prop = self._process_property(child)
                    if prop:
                        class_info.properties.append(prop)

        if class_info.is_serializable:
            self.classes.append(class_info)

    def _has_rf_class_annotation(self, cursor) -> bool:
        """Check if cursor has RF_CLASS by comparing against pre-scanned line numbers.

        RF_CLASS(...) must appear on one of the lines directly before the
        class/struct keyword (typically 1-3 lines above).
        """
        class_line = cursor.location.line
        for macro_line in self._rf_class_lines:
            if 0 < class_line - macro_line <= 3:
                return True
        return False

    def _has_rf_serializable(self, cursor) -> bool:
        """Check if class has RF_SERIALIZABLE using pre-scanned line numbers."""
        class_start = cursor.location.line
        class_end = cursor.extent.end.line
        for macro_line in self._rf_serializable_lines:
            if class_start < macro_line < class_end:
                return True
        return False

    def _has_rf_property_annotation(self, cursor) -> bool:
        """Check if field has RF_PROPERTY using pre-scanned line numbers."""
        field_line = cursor.location.line
        for macro_line in self._rf_property_lines:
            if 0 < field_line - macro_line <= 2:
                return True
        return False

    def _process_property(self, cursor) -> Optional[PropertyInfo]:
        """Process a field declaration into PropertyInfo"""
        prop_type = cursor.type.spelling
        canonical_type = cursor.type.get_canonical().spelling

        # Check if it's an array/vector
        is_array = False
        element_type = None

        if 'vector<' in canonical_type or 'array<' in canonical_type:
            is_array = True
            # Try to extract element type
            # This is simplified - libclang can give us the template argument
            type_obj = cursor.type.get_canonical()
            if type_obj.kind == TypeKind.ELABORATED:
                type_obj = type_obj.get_named_type()

            # Get template arguments
            if hasattr(type_obj, 'get_num_template_arguments'):
                num_args = type_obj.get_num_template_arguments()
                if num_args > 0:
                    element_type = type_obj.get_template_argument_type(0).spelling

        # Get default value
        default_value = None
        for child in cursor.get_children():
            if child.kind == CursorKind.INTEGER_LITERAL or \
               child.kind == CursorKind.FLOATING_LITERAL or \
               child.kind == CursorKind.STRING_LITERAL or \
               child.kind == CursorKind.CXX_BOOL_LITERAL_EXPR:
                # Get the literal value from tokens
                tokens = list(child.get_tokens())
                if tokens:
                    default_value = tokens[0].spelling

        # Parse attributes
        attributes = self._parse_property_attributes(cursor)

        prop = PropertyInfo(
            name=cursor.spelling,
            type=prop_type,
            canonical_type=canonical_type,
            default_value=default_value,
            attributes=attributes,
            is_array=is_array,
            element_type=element_type,
            is_pointer='*' in canonical_type,
            is_reference='&' in canonical_type
        )

        prop.tooltip = attributes.get('Tooltip', "")
        display_attr = attributes.get('Display', "true").lower()
        prop.display = display_attr == "true"

        return prop

    def _parse_property_attributes(self, cursor) -> Dict[str, str]:
        """Parse RF_PROPERTY attributes for a specific field"""
        field_line = cursor.location.line
        for macro_line, raw_text in self._rf_property_lines.items():
            if 0 < field_line - macro_line <= 2:
                return self._parse_attributes_from_raw(raw_text, 'RF_PROPERTY')
        return {}


# Reuse the CXRGenerator from the original implementation
class CXRGenerator:
    """Generates .cxr.cpp files with ISerializable implementation"""

    def __init__(self, class_info: ClassInfo, source_file_path: str):
        self.class_info = class_info
        self.source_file_path = source_file_path

    def generate(self) -> str:
        """Generate the complete .cxr.cpp file content"""
        output = []

        # Header
        output.append(self._generate_header())
        output.append("")

        # Includes
        output.append(self._generate_includes())
        output.append("")

        # Namespace
        if self.class_info.namespace:
            output.append(f"namespace {self.class_info.namespace} {{")
            output.append("")

        # Type registration
        output.append(self._generate_type_registration())
        output.append("")

        # Archive method (single bidirectional save/load)
        output.append(self._generate_archive_method())
        output.append("")

        # GetTypeInfo method
        output.append(self._generate_type_info_method())
        output.append("")

        # Close namespace
        if self.class_info.namespace:
            output.append(f"}} // namespace {self.class_info.namespace}")

        return "\n".join(output)

    def _generate_header(self) -> str:
        return f"""// Auto-generated reflection file for {self.class_info.name}
// Generated by Reflector
// DO NOT MODIFY - This file is automatically generated"""

    def _get_relative_include_path(self) -> str:
        """
        Calculate the relative path from CWD to the source file
        for the #include directive
        """
        try:
            # Get current working directory
            cwd = os.getcwd()

            # Get absolute path of the source file
            abs_source = os.path.abspath(self.source_file_path)

            # Calculate relative path from CWD to source file
            rel_path = os.path.relpath(abs_source, cwd)

            # Convert to forward slashes for consistency (works on all platforms)
            rel_path = rel_path.replace(os.sep, '/')

            return rel_path
        except Exception as e:
            # Fallback to just the filename if relative path calculation fails
            return os.path.basename(self.source_file_path)

    def _generate_includes(self) -> str:
        # Get relative path for include
        relative_include = self._get_relative_include_path()

        includes = [
            '#include <modex.h>',
            f'#include <{relative_include}>',
        ]
        return "\n".join(includes)

    def _generate_type_registration(self) -> str:
        full_name = f"{self.class_info.namespace}::{self.class_info.name}" if self.class_info.namespace else self.class_info.name
        return f"""// Static type registration
static bool s_{self.class_info.name}_Registered = []() {{
    codex::NBMan::register_type<{full_name}>("{self.class_info.name}");
    return true;
}}();"""

    def _generate_archive_method(self) -> str:
        # One bidirectional method: ar("key", field) saves or loads depending on the
        # backend. Containers (e.g. std::vector) are handled automatically by the Archive
        # dispatch, so no per-array codegen is needed.
        lines = [
            f"void {self.class_info.name}::archive(codex::Archive& ar) {{"
        ]

        # Don't chain into NativeBehaviour: it is an interface with no reflected properties.
        if self.class_info.base_class and "NativeBehaviour" not in self.class_info.base_class:
            lines.append(f"    {self.class_info.base_class}::archive(ar);")
            lines.append("")

        for prop in self.class_info.properties:
            lines.append(f'    ar("{prop.name}", {prop.name});')

        lines.append("}")
        return "\n".join(lines)

    def _generate_type_info_method(self) -> str:
        lines = [
            f"const codex::rf::TypeInfo& {self.class_info.name}::type_info() const {{",
            f'    static codex::rf::TypeInfo type_info("{self.class_info.name}");',
            "    static bool initialized = false;",
            "    ",
            "    if (!initialized) {"
        ]

        for prop in self.class_info.properties:
            # Use original name as default for display name
            display_name = prop.attributes.get('DisplayName', prop.name)
            category = prop.attributes.get('Category', 'General')
            tooltip = prop.tooltip
            display_val = "true" if prop.display else "false"

            lines.append(f'        type_info.add_property(')
            lines.append(f'            "{prop.name}",')
            lines.append(f'            codex::rf::type_of({prop.name}),')
            lines.append(f'            offsetof({self.class_info.name}, {prop.name}),')
            lines.append(f'            "{display_name}",')
            lines.append(f'            "{category}",')
            lines.append(f'            "{tooltip}",')
            lines.append(f'            {display_val}')
            lines.append(f'        );')

        lines.append("        initialized = true;")
        lines.append("    }")
        lines.append("    ")
        lines.append("    return type_info;")
        lines.append("}")
        return "\n".join(lines)

    def _get_default_value(self, type_name: str) -> str:
        # Clean up type
        clean_type = type_name.replace('const ', '').replace('&', '').replace('*', '').strip()

        defaults = {
            "int": "0",
            "int32_t": "0",
            "unsigned int": "0",
            "uint32_t": "0",
            "float": "0.0f",
            "double": "0.0",
            "bool": "false",
            "std::string": '""',
            "std::basic_string<char>": '""',
            "ivec3": "ivec3()",
            "ivec4": "ivec4()",
            "ivec2": "ivec2()",
            "vec3": "vec3()",
            "vec4": "vec4()",
            "vec2": "vec2()",
        }
        return defaults.get(clean_type, f"{clean_type}()")


def main():
    parser = argparse.ArgumentParser(
        description='Libclang-based reflection generator for NativeBehaviour scripts',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Single file
  python reflection_generator_libclang.py my_script.h -o ./generated

  # Multiple files
  python reflection_generator_libclang.py script1.h script2.h script3.h -o ./generated

  # Using wildcards (bash will expand)
  python reflection_generator_libclang.py src/scripts/*.h -o ./generated

  # With include paths
  python reflection_generator_libclang.py my_script.h -o ./generated -I./include -I../engine/include

  # Using CMake compilation database
  python reflection_generator_libclang.py my_script.h -o ./generated --compile-commands=build

  # Process entire directory with compilation database
  python reflection_generator_libclang.py src/scripts/*.h -o ./generated --compile-commands=build -v
        """
    )

    parser.add_argument(
        'input_files',
        nargs='+',
        help='Input C++ header file(s) - can specify multiple files or use wildcards'
    )

    parser.add_argument(
        '-o', '--output',
        dest='output_dir',
        default='.',
        help='Output directory for .cxr.cpp files (default: current directory)'
    )

    parser.add_argument(
        '-I', '--include',
        action='append',
        dest='includes',
        default=[],
        help='Add include directory'
    )

    parser.add_argument(
        '-D', '--define',
        action='append',
        dest='defines',
        default=[],
        help='Add preprocessor define'
    )

    parser.add_argument(
        '--std',
        default='c++20',
        help='C++ standard (default: c++20)'
    )

    parser.add_argument(
        '--compile-commands',
        dest='compile_commands',
        help='Path to compile_commands.json or directory containing it'
    )

    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Verbose output'
    )

    args = parser.parse_args()

    # Expand glob patterns if needed
    import glob
    input_files = []
    for pattern in args.input_files:
        if '*' in pattern or '?' in pattern:
            # Glob pattern
            matches = glob.glob(pattern, recursive=True)
            input_files.extend(matches)
        else:
            # Regular file
            input_files.append(pattern)

    # Remove duplicates and filter for header files
    input_files = list(set(input_files))
    input_files = [f for f in input_files if f.endswith(('.h', '.hpp', '.hxx'))]

    if not input_files:
        print(f"Error: No header files found matching the input patterns")
        sys.exit(1)

    # Check that files exist
    missing_files = [f for f in input_files if not os.path.exists(f)]
    if missing_files:
        print(f"Error: The following files were not found:")
        for f in missing_files:
            print(f"  - {f}")
        sys.exit(1)

    # Build compiler args
    compiler_args = [f'-std={args.std}']
    for define in args.defines:
        compiler_args.append(f'-D{define}')

    # Create parser
    parser_obj = LibclangReflectionParser(include_paths=args.includes)

    # Load compilation database if provided
    if args.compile_commands:
        if not parser_obj.load_compilation_database(args.compile_commands):
            print("Warning: Failed to load compilation database, using manual include paths")

    # Only pass compiler_args if we have manual includes/defines
    # Otherwise let it use the compilation database
    manual_args = compiler_args if (args.includes or args.defines or args.std != 'c++20') else None

    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)

    # Process each file
    total_generated = 0
    total_skipped = 0
    total_errors = 0

    print(f"\nProcessing {len(input_files)} file(s)...")
    print("=" * 80)

    for idx, input_file in enumerate(input_files, 1):
        print(f"\n[{idx}/{len(input_files)}] {input_file}")
        print("-" * 80)

        try:
            # Parse the file
            if args.verbose:
                print(f"Parsing with libclang...")

            # Reset classes for each file
            parser_obj.classes = []
            classes = parser_obj.parse_file(input_file, compiler_args=manual_args)

            if not classes:
                print(f"  No RF_CLASS declarations found")
                total_skipped += 1
                continue

            # Generate .cxr.cpp files for each class
            for class_info in classes:
                if not class_info.is_serializable:
                    if args.verbose:
                        print(f"  Skipping {class_info.name} - not marked as RF_SERIALIZABLE")
                    total_skipped += 1
                    continue

                generator = CXRGenerator(class_info, input_file)
                output_content = generator.generate()

                output_filename = f"{Path(input_file).stem}.cxr.cpp"
                output_path = os.path.join(args.output_dir, output_filename)

                with open(output_path, 'w', encoding='utf-8') as f:
                    f.write(output_content)

                print(f"  Generated: {output_filename}")
                if args.verbose:
                    print(f"    Class: {class_info.qualified_name}")
                    print(f"    Base: {class_info.base_class}")
                    print(f"    Properties: {len(class_info.properties)}")

                total_generated += 1

        except Exception as e:
            print(f"  Error processing file: {e}")
            if args.verbose:
                import traceback
                traceback.print_exc()
            total_errors += 1

    # Summary
    print("\n" + "=" * 80)
    print("SUMMARY:")
    print(f"  Files processed: {len(input_files)}")
    print(f"  Classes generated: {total_generated}")
    print(f"  Skipped: {total_skipped}")
    print(f"  Errors: {total_errors}")
    print(f"  Output directory: {os.path.abspath(args.output_dir)}")
    print("=" * 80)

    return 0 if total_errors == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
