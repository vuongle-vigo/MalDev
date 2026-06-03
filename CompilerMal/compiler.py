"""
MalDev Compiler - Build pipeline for CPP via MSVC.

Workflow:
  1. Accept CPP files/folders as input
  2. Run convert_string.py to convert string literals to char arrays
  3. Run cl.exe to compile CPP -> ASM
  4. Run clear_temp_segment.py to process .asm
  5. Run ml/ml64.exe to assemble + link ALL .asm -> EXE (single call)

Supports x86 (ml) and x64 (ml64). Auto-parses .vcxproj for AdditionalIncludeDirectories and external source files.

Usage:
    python compiler.py file1.cpp file2.cpp [--arch x86|x64]
    python compiler.py ./src_folder --output ./build --arch x86
"""

import sys
sys.stdout.reconfigure(encoding='utf-8')
sys.stderr.reconfigure(encoding='utf-8')

import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent.resolve()
CONVERT_STRING_SCRIPT = SCRIPT_DIR / "convert_string.py"
CLEAR_TEMP_SCRIPT = SCRIPT_DIR / "clear_temp_segment.py"
VCVARS32_PATH = Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars32.bat")
VCVARS64_PATH = Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat")


def _run_cmd(vcvars_path: Path, cwd: Path, command: str, verbose: bool) -> subprocess.CompletedProcess:
    """
    Run MSVC command inside cmd with vcvars already sourced.
    Most reliable approach on Windows — no env dict parsing needed.
    """
    # Batch script: call vcvars -> run command -> exit
    bat_script = (
        f'@echo off\n'
        f'call "{vcvars_path}" >nul 2>&1\n'
        f'{command}\n'
        f'exit %ERRORLEVEL%\n'
    )

    with tempfile.NamedTemporaryFile(
        mode='w', suffix='.bat', delete=False, encoding='utf-8'
    ) as f:
        f.write(bat_script)
        bat_path = f.name

    try:
        result = subprocess.run(
            ['cmd', '/c', bat_path],
            cwd=str(cwd),
            capture_output=True,
            text=True,
        )
        # In output
        if result.stdout:
            for line in result.stdout.splitlines():
                if line.strip():
                    print(f"  {line}")
        if result.stderr:
            for line in result.stderr.splitlines():
                if line.strip():
                    print(f"  {line}", file=sys.stderr)
        return result
    finally:
        try:
            os.unlink(bat_path)
        except OSError:
            pass


def _find_cpp_files(root: Path) -> list[Path]:
    """Find all .cpp/.c files under root (file or folder)."""
    if root.is_file():
        if root.suffix.lower() in {'.c', '.cpp', '.cc', '.cxx'}:
            return [root]
        return []
    cpp_exts = {'.c', '.cpp', '.cc', '.cxx'}
    return sorted(
        p for p in root.rglob('*')
        if p.is_file() and p.suffix.lower() in cpp_exts and p != root
    )


def _find_vcxproj(folder: Path) -> Path | None:
    """Find the first .vcxproj file in folder (recursive)."""
    candidates = list(folder.rglob('*.vcxproj'))
    return candidates[0] if candidates else None


def _parse_vcxproj(vcxproj_path: Path) -> tuple[list[Path], list[Path]]:
    """
    Parse .vcxproj to extract:
    - additional_include_directories: include folder list
    - external_source_files: .cpp/.c files outside the project folder

    Returns (include_dirs, source_files).
    Relative paths in vcxproj are resolved relative to vcxproj parent folder.
    """
    include_dirs: list[Path] = []
    source_files: list[Path] = []
    vcxproj_dir = vcxproj_path.parent.resolve()

    try:
        content = vcxproj_path.read_text(encoding='utf-8', errors='replace')
    except Exception:
        return [], []

    # Extract AdditionalIncludeDirectories from ClCompile in all ItemDefinitionGroups
    inc_pattern = re.compile(
        r'<AdditionalIncludeDirectories>([^<]+)</AdditionalIncludeDirectories>',
        re.IGNORECASE
    )
    for m in inc_pattern.finditer(content):
        raw = m.group(1)
        for part in raw.split(';'):
            part = part.strip()
            if not part:
                continue
            # Skip macros like %(AdditionalIncludeDirectories)
            if part.startswith('$') or part.startswith('%('):
                continue
            p = Path(part)
            if p.is_absolute():
                if p.exists() and p.is_dir():
                    include_dirs.append(p)
            else:
                resolved = (vcxproj_dir / p).resolve()
                if resolved.exists() and resolved.is_dir():
                    include_dirs.append(resolved)

    # Extract ClCompile entries (source files)
    compile_pattern = re.compile(
        r'<ClCompile\s+Include\s*=\s*"([^"]+)"',
        re.IGNORECASE
    )
    for m in compile_pattern.finditer(content):
        raw = m.group(1).strip()
        if not raw:
            continue
        p = Path(raw)
        if p.is_absolute():
            src = p
        else:
            src = (vcxproj_dir / p).resolve()

        if src.exists() and src.suffix.lower() in {'.c', '.cpp', '.cc', '.cxx'}:
            source_files.append(src)

    return include_dirs, source_files


def _detect_include_dirs(
    cpp_files: list[Path], vcxproj_includes: list[Path]
) -> list[Path]:
    """
    Scan all #include directives in CPP files and return the list of
    folders containing the included files (excluding system headers like <...>).
    Combined with include dirs from vcxproj.
    """
    include_re = re.compile(r'#include\s+"([^"]+)"')
    found_dirs: list[Path] = []
    seen: set[str] = set()

    for cpp_file in cpp_files:
        try:
            content = cpp_file.read_text(encoding='utf-8', errors='replace')
        except Exception:
            continue
        for m in include_re.finditer(content):
            inc_name = m.group(1)
            inc_path = cpp_file.parent / inc_name
            if inc_path.exists():
                inc_dir = inc_path.parent.resolve()
                key = str(inc_dir)
                if key not in seen:
                    seen.add(key)
                    found_dirs.append(inc_dir)

    # Thêm include dirs từ vcxproj (nếu chưa có)
    for d in vcxproj_includes:
        key = str(d.resolve())
        if key not in seen:
            seen.add(key)
            found_dirs.append(d)

    return found_dirs


def _step_convert_strings(cpp_files: list[Path], verbose: bool) -> bool:
    """Step 1: Run convert_string.py on CPP files."""
    print(f"\n{'='*60}")
    print(f"[STEP 1] Convert string literals -> char array")
    print(f"{'='*60}")

    if not CONVERT_STRING_SCRIPT.exists():
        print(f"Error: {CONVERT_STRING_SCRIPT} not found", file=sys.stderr)
        return False

    for cpp_file in cpp_files:
        cmd = [sys.executable, str(CONVERT_STRING_SCRIPT), str(cpp_file)]
        if verbose:
            cmd.append('-v')
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.stdout:
            print(result.stdout)
        if result.returncode != 0:
            print(result.stderr, file=sys.stderr)
            return False
    return True


def _step_compile_cl(
    cpp_files: list[Path], output_dir: Path, vcvars_path: Path,
    include_dirs: list[Path], verbose: bool
) -> list[Path]:
    """
    Step 2: Run cl.exe to compile CPP -> ASM.
    cl /c /GS- /FA /Fo /I /std:c++20
    Returns list of generated .asm files.
    """
    print(f"\n{'='*60}")
    print(f"[STEP 2] Compile CPP -> ASM (cl.exe)")
    print(f"{'='*60}")

    output_dir.mkdir(parents=True, exist_ok=True)

    # Build /I args for cl.exe
    include_args: list[str] = []
    for inc_dir in include_dirs:
        include_args.append(f'/I"{inc_dir}"')

    asm_files = []

    for cpp_file in cpp_files:
        cpp_name = cpp_file.stem

        cmd_parts = [
            'cl',
            '/c',
            '/GS-',
            '/FA',
            f'/Fo"{output_dir}"',
            '/std:c++20',
        ] + include_args + [
            f'"{cpp_file}"',
        ]
        command = ' '.join(cmd_parts)

        print(f"\n--- Compiling: {cpp_file.name} ---")
        print(f"[CMD] {command}")
        result = _run_cmd(vcvars_path, output_dir, command, verbose)

        # MSVC produces .asm even with warnings (not errors).
        # Only fail if .obj was also not produced.
        asm_candidate = output_dir / f"{cpp_name}.asm"
        obj_candidate = output_dir / f"{cpp_name}.obj"

        if asm_candidate.exists():
            asm_files.append(asm_candidate)
            print(f"  [OK] ASM: {asm_candidate}")
        elif result.returncode != 0 and not obj_candidate.exists():
            print(f"Compile failed: {cpp_file.name}", file=sys.stderr)
            return []

    print(f"\nGenerated {len(asm_files)} .asm file(s)")
    return asm_files


def _step_clear_temp_segment(asm_files: list[Path], verbose: bool) -> list[Path]:
    """
    Step 3: Run clear_temp_segment.py on .asm files.
    Returns list of processed .asm files (same list, just verifies existence).
    """
    print(f"\n{'='*60}")
    print(f"[STEP 3] Process temp segments (clear_temp_segment.py)")
    print(f"{'='*60}")

    if not CLEAR_TEMP_SCRIPT.exists():
        print(f"Error: {CLEAR_TEMP_SCRIPT} not found", file=sys.stderr)
        return []

    for asm_file in asm_files:
        cmd = [sys.executable, str(CLEAR_TEMP_SCRIPT), str(asm_file)]
        if verbose:
            cmd.append('-v')
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.stdout:
            print(result.stdout)
        if result.returncode != 0:
            print(f"Error processing {asm_file.name}", file=sys.stderr)
            if result.stderr:
                print(result.stderr, file=sys.stderr)
            return []

    print(f"\nProcessed {len(asm_files)} .asm file(s)")
    return asm_files


def _step_assemble_ml64(
    asm_files: list[Path], cpp_files: list[Path], output_dir: Path, vcvars_path: Path, arch: str, verbose: bool
) -> Path | None:
    """
    Step 4: Assemble + Link ALL .asm -> .exe in a single ml/ml64 call.
    Exe name is derived from the first CPP file.
    """
    print(f"\n{'='*60}")
    print(f"[STEP 4] Assemble + Link ASM -> EXE ({('ml' if arch == 'x86' else 'ml64')}.exe)")
    print(f"{'='*60}")

    if not asm_files:
        print("No .asm files to assemble", file=sys.stderr)
        return None

    output_dir.mkdir(parents=True, exist_ok=True)
    exe_name = cpp_files[0].stem + ".exe"
    exe_path = output_dir / exe_name

    asm_args = ' '.join(f'"{asm}"' for asm in asm_files)
    assembler = 'ml' if arch == 'x86' else 'ml64'
    cmd_parts = [
        assembler,
        asm_args,
        f'/Fo"{output_dir}"',
        '/link',
        f'/OUT:"{exe_path}"',
        '/entry:main',
    ]
    command = ' '.join(cmd_parts)

    print(f"[CMD] {command}")
    result = _run_cmd(vcvars_path, output_dir, command, verbose)

    if result.returncode != 0:
        print(f"Assemble/link failed", file=sys.stderr)
        return None

    if exe_path.exists():
        print(f"  [OK] EXE: {exe_path}")
        return exe_path

    print(f"EXE not found after assemble", file=sys.stderr)
    return None




def main():
    parser = argparse.ArgumentParser(
        description="MalDev Compiler - Build CPP via MSVC (cl + ml64)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python compiler.py HttpClient.cpp main.cpp ApiResolve.cpp CRT.cpp
  python compiler.py ./src_folder --output ./build --arch x86
  python compiler.py ./src_folder -I ./include -I ./headers --keep-asm -v
        """
    )
    parser.add_argument(
        'input_path',
        nargs='+',
        type=Path,
        help="CPP source file(s) or folder(s)"
    )
    parser.add_argument(
        '--output', '-o',
        type=Path,
        default=None,
        metavar='DIR',
        help="Output folder for ASM, OBJ, EXE. Default: input folder"
    )
    parser.add_argument(
        '--keep-asm',
        action='store_true',
        help="Keep .asm files after build"
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help="Show detailed step output"
    )
    parser.add_argument(
        '--vcvars',
        type=Path,
        default=None,
        metavar='PATH',
        help="Path to vcvars.bat (auto-selected based on --arch by default)"
    )
    parser.add_argument(
        '--arch',
        choices=['x64', 'x86'],
        default='x64',
        help="Target architecture: x64 (default) uses ml64, x86 uses ml"
    )
    parser.add_argument(
        '--include', '-I',
        action='append',
        type=Path,
        metavar='DIR',
        dest='include_dirs',
        default=[],
        help="Include directory for cl.exe (repeatable). Auto-detected from #include in code."
    )

    args = parser.parse_args()

    # Validate all input paths
    for p in args.input_path:
        if not p.exists():
            print(f"Error: Not found: {p}", file=sys.stderr)
            sys.exit(1)

    if args.vcvars:
        vcvars_path = args.vcvars.resolve()
    else:
        vcvars_path = (VCVARS32_PATH if args.arch == 'x86' else VCVARS64_PATH)

    if not vcvars_path.exists():
        print(f"Error: vcvars.bat not found: {vcvars_path}", file=sys.stderr)
        sys.exit(1)

    # Determine output_dir
    if args.output:
        output_dir = args.output.resolve()
    else:
        first = args.input_path[0]
        output_dir = first.parent.resolve() if first.is_file() else first.resolve()

    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"""
{'='*60}
  MalDev Compiler - MSVC Build Pipeline
{'='*60}
  Input   : {', '.join(str(p) for p in args.input_path)}
  Output  : {output_dir}
  Arch    : {args.arch}
  vcvars  : {vcvars_path}
{'='*60}
    """)

    # Collect CPP files from all input paths
    cpp_files: list[Path] = []
    seen_paths: set[Path] = set()
    vcxproj_includes: list[Path] = []
    vcxproj_external_sources: list[Path] = []

    for input_path in args.input_path:
        files = _find_cpp_files(input_path.resolve())
        for f in files:
            if f.resolve() not in seen_paths:
                cpp_files.append(f)
                seen_paths.add(f.resolve())

        # If folder, look for .vcxproj to load project config
        if input_path.is_dir():
            vcxproj_path = _find_vcxproj(input_path.resolve())
            if vcxproj_path:
                print(f"[*] Found .vcxproj: {vcxproj_path}")
                inc, ext_src = _parse_vcxproj(vcxproj_path)
                for d in inc:
                    if d not in vcxproj_includes:
                        vcxproj_includes.append(d)
                        print(f"    Include dir: {d}")
                for s in ext_src:
                    if s.resolve() not in seen_paths:
                        vcxproj_external_sources.append(s)
                        print(f"    External source: {s}")
                        cpp_files.append(s)
                        seen_paths.add(s.resolve())

    if not cpp_files:
        print(f"No .cpp/.c files found", file=sys.stderr)
        sys.exit(1)

    print(f"\nFound {len(cpp_files)} CPP file(s):")
    for f in cpp_files:
        print(f"  - {f}")
    print()

    # Auto-detect include dirs from #include in code + vcxproj
    auto_include_dirs = _detect_include_dirs(cpp_files, vcxproj_includes)
    all_include_dirs = auto_include_dirs.copy()
    for d in args.include_dirs:
        resolved = d.resolve()
        if resolved not in all_include_dirs:
            all_include_dirs.append(resolved)

    if all_include_dirs:
        print(f"Found {len(all_include_dirs)} include dir(s):")
        for d in all_include_dirs:
            print(f"  - {d}")
        print()

    # Step 1: Convert strings
    if not _step_convert_strings(cpp_files, args.verbose):
        sys.exit(1)

    # Step 2: cl.exe compile
    asm_files = _step_compile_cl(cpp_files, output_dir, vcvars_path, all_include_dirs, args.verbose)
    if not asm_files:
        print("No .asm files generated", file=sys.stderr)
        sys.exit(1)

    # Step 3: clear_temp_segment.py
    if not _step_clear_temp_segment(asm_files, args.verbose):
        sys.exit(1)

    # Step 4: ml/ml64 assemble + link
    exe_path = _step_assemble_ml64(asm_files, cpp_files, output_dir, vcvars_path, args.arch, args.verbose)

    # Clean up .asm files
    if not args.keep_asm:
        for asm_file in asm_files:
            try:
                asm_file.unlink()
                print(f"[CLEAN] Removed: {asm_file}")
            except OSError:
                pass

    # Result
    print(f"\n{'='*60}")
    if exe_path and exe_path.exists():
        print(f"  BUILD SUCCESS!")
        print(f"  EXE: {exe_path}")
        if not args.keep_asm:
            print(f"  ASM: removed (use --keep-asm to keep)")
    else:
        print(f"  BUILD FAILED", file=sys.stderr)
        sys.exit(1)
    print(f"{'='*60}")


if __name__ == '__main__':
    main()
