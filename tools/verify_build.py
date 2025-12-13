#!/usr/bin/env python3
"""
XpuMusic Build Verification Tool
Python implementation for detailed build analysis and testing
"""

import os
import sys
import subprocess
import time
from pathlib import Path
from typing import List, Tuple, Dict
import json

class BuildVerifier:
    def __init__(self, project_root: str = None):
        if project_root is None:
            self.project_root = Path(__file__).parent.parent
        else:
            self.project_root = Path(project_root)

        self.build_dir = self.project_root / "build"
        self.exe_dir = self.build_dir / "bin" / "Debug"
        self.lib_dir = self.build_dir / "lib" / "Debug"
        self.test_audio = self.project_root / "test_1khz.wav"

        self.results = {
            "tests_passed": 0,
            "tests_total": 0,
            "executables": {},
            "libraries": {},
            "functional_tests": {},
            "performance": {},
            "warnings": []
        }

    def log(self, message: str, level: str = "INFO"):
        """Log message with timestamp"""
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{timestamp}] {level}: {message}")

    def check_executable(self, name: str) -> bool:
        """Check if executable exists and get its info"""
        exe_path = self.exe_dir / name
        if exe_path.exists():
            stat = exe_path.stat()
            size_mb = stat.st_size / (1024 * 1024)
            self.results["executables"][name] = {
                "exists": True,
                "size_bytes": stat.st_size,
                "size_mb": round(size_mb, 2),
                "modified": stat.st_mtime
            }
            return True
        else:
            self.results["executables"][name] = {"exists": False}
            return False

    def check_library(self, name: str) -> bool:
        """Check if library exists"""
        lib_path = self.lib_dir / name
        if lib_path.exists():
            stat = lib_path.stat()
            self.results["libraries"][name] = {
                "exists": True,
                "size_bytes": stat.st_size
            }
            return True
        else:
            self.results["libraries"][name] = {"exists": False}
            return False

    def run_executable_test(self, exe_name: str, timeout: int = 30) -> Tuple[bool, str]:
        """Run an executable test and capture output"""
        exe_path = self.exe_dir / exe_name
        if not exe_path.exists():
            return False, "Executable not found"

        try:
            start_time = time.time()
            result = subprocess.run(
                [str(exe_path)],
                capture_output=True,
                text=True,
                timeout=timeout,
                cwd=str(self.exe_dir)
            )
            duration = time.time() - start_time

            self.results["functional_tests"][exe_name] = {
                "exit_code": result.returncode,
                "duration_sec": round(duration, 2),
                "stdout": result.stdout[:500],  # Limit output
                "stderr": result.stderr[:500]
            }

            return result.returncode == 0, f"Duration: {duration:.2f}s"

        except subprocess.TimeoutExpired:
            return False, "Test timed out"
        except Exception as e:
            return False, str(e)

    def run_audio_playback_test(self) -> Tuple[bool, str]:
        """Test actual audio playback"""
        if not self.test_audio.exists():
            return False, "Test audio file not found"

        player_exe = self.exe_dir / "final_wav_player.exe"
        if not player_exe.exists():
            return False, "Audio player not found"

        try:
            # Run with timeout
            result = subprocess.run(
                [str(player_exe), str(self.test_audio)],
                capture_output=True,
                text=True,
                timeout=10  # 10 second timeout for audio playback
            )

            if result.returncode == 0:
                # Check if playback was successful
                if "OK" in result.stdout or "completed" in result.stdout.lower():
                    return True, "Audio played successfully"
                else:
                    return False, "Audio playback completed but may have issues"
            else:
                return False, f"Audio playback failed: {result.stderr[:100]}"

        except subprocess.TimeoutExpired:
            return False, "Audio playback timed out"
        except Exception as e:
            return False, f"Error during audio test: {str(e)}"

    def verify_build_structure(self) -> bool:
        """Verify the build directory structure"""
        self.log("Verifying build structure...")

        required_dirs = [self.build_dir, self.exe_dir]
        for dir_path in required_dirs:
            if not dir_path.exists():
                self.results["warnings"].append(f"Missing directory: {dir_path}")
                return False
        return True

    def check_required_files(self) -> Dict[str, bool]:
        """Check all required files exist"""
        self.log("Checking required files...")

        # Required executables
        required_exes = [
            "music-player.exe",
            "final_wav_player.exe",
            "test_decoders.exe",
            "test_audio_direct.exe",
            "test_simple_playback.exe"
        ]

        # Optional executables
        optional_exes = [
            "foobar2k-player.exe",
            "optimization_integration_example.exe",
            "simple_performance_test.exe"
        ]

        # Required libraries
        required_libs = [
            "core_engine.lib",
            "platform_abstraction.lib",
            "sdk_impl.lib"
        ]

        file_results = {}

        # Check required executables
        for exe in required_exes:
            exists = self.check_executable(exe)
            file_results[f"exe_{exe}"] = exists
            self.results["tests_total"] += 1
            if exists:
                self.results["tests_passed"] += 1
                self.log(f"  ✓ {exe}")
            else:
                self.log(f"  ✗ {exe} - MISSING", "ERROR")

        # Check optional executables
        for exe in optional_exes:
            exists = self.check_executable(exe)
            file_results[f"opt_exe_{exe}"] = exists
            if exists:
                self.log(f"  ✓ {exe} (optional)")
            else:
                self.log(f"  - {exe} (optional - not built)")

        # Check libraries
        for lib in required_libs:
            exists = self.check_library(lib)
            file_results[f"lib_{lib}"] = exists
            self.results["tests_total"] += 1
            if exists:
                self.results["tests_passed"] += 1
                self.log(f"  ✓ {lib}")
            else:
                self.log(f"  ✗ {lib} - MISSING", "ERROR")

        return file_results

    def run_functional_tests(self) -> None:
        """Run all functional tests"""
        self.log("Running functional tests...")

        # List of test executables to run
        test_exes = [
            "test_decoders.exe",
            "test_audio_direct.exe",
            "test_simple_playback.exe"
        ]

        for test_exe in test_exes:
            self.results["tests_total"] += 1
            success, message = self.run_executable_test(test_exe)

            if success:
                self.results["tests_passed"] += 1
                self.log(f"  ✓ {test_exe} - {message}")
            else:
                self.log(f"  ✗ {test_exe} - {message}", "ERROR")

        # Audio playback test
        if self.test_audio.exists():
            self.results["tests_total"] += 1
            success, message = self.run_audio_playback_test()
            if success:
                self.results["tests_passed"] += 1
                self.log(f"  ✓ Audio playback test - {message}")
            else:
                self.log(f"  ✗ Audio playback test - {message}", "WARNING")

    def analyze_performance(self) -> None:
        """Analyze build performance metrics"""
        self.log("Analyzing performance metrics...")

        # Calculate total build size
        total_exe_size = sum(
            info.get("size_bytes", 0)
            for info in self.results["executables"].values()
            if info.get("exists", False)
        )

        total_lib_size = sum(
            info.get("size_bytes", 0)
            for info in self.results["libraries"].values()
            if info.get("exists", False)
        )

        self.results["performance"] = {
            "total_executable_size_mb": round(total_exe_size / (1024 * 1024), 2),
            "total_library_size_mb": round(total_lib_size / (1024 * 1024), 2),
            "total_build_size_mb": round((total_exe_size + total_lib_size) / (1024 * 1024), 2),
            "num_executables": len([e for e in self.results["executables"].values() if e.get("exists", False)]),
            "num_libraries": len([l for l in self.results["libraries"].values() if l.get("exists", False)])
        }

        self.log(f"  Total build size: {self.results['performance']['total_build_size_mb']} MB")

    def generate_report(self, output_file: str = None) -> str:
        """Generate verification report"""
        report = []
        report.append("XpuMusic Build Verification Report")
        report.append("=" * 50)
        report.append(f"Date: {time.strftime('%Y-%m-%d %H:%M:%S')}")
        report.append("")

        # Summary
        report.append("SUMMARY")
        report.append("-" * 20)
        report.append(f"Tests passed: {self.results['tests_passed']}/{self.results['tests_total']}")

        success_rate = (self.results['tests_passed'] / self.results['tests_total'] * 100) if self.results['tests_total'] > 0 else 0
        report.append(f"Success rate: {success_rate:.1f}%")
        report.append("")

        # Executables
        report.append("EXECUTABLES")
        report.append("-" * 20)
        for name, info in self.results["executables"].items():
            if info.get("exists", False):
                report.append(f"  ✓ {name} - {info.get('size_mb', 0)} MB")
            else:
                report.append(f"  ✗ {name} - MISSING")
        report.append("")

        # Libraries
        report.append("LIBRARIES")
        report.append("-" * 20)
        for name, info in self.results["libraries"].items():
            if info.get("exists", False):
                size_kb = info.get("size_bytes", 0) / 1024
                report.append(f"  ✓ {name} - {size_kb:.1f} KB")
            else:
                report.append(f"  ✗ {name} - MISSING")
        report.append("")

        # Performance
        report.append("PERFORMANCE METRICS")
        report.append("-" * 20)
        perf = self.results["performance"]
        report.append(f"Total build size: {perf.get('total_build_size_mb', 0)} MB")
        report.append(f"Executables: {perf.get('num_executables', 0)}")
        report.append(f"Libraries: {perf.get('num_libraries', 0)}")
        report.append("")

        # Warnings
        if self.results["warnings"]:
            report.append("WARNINGS")
            report.append("-" * 20)
            for warning in self.results["warnings"]:
                report.append(f"  ⚠ {warning}")
            report.append("")

        # Functional test results
        if self.results["functional_tests"]:
            report.append("FUNCTIONAL TESTS")
            report.append("-" * 20)
            for test_name, test_info in self.results["functional_tests"].items():
                status = "✓ PASSED" if test_info.get("exit_code", -1) == 0 else "✗ FAILED"
                duration = test_info.get("duration_sec", 0)
                report.append(f"  {status} {test_name} ({duration}s)")
            report.append("")

        # Verdict
        report.append("VERDICT")
        report.append("-" * 20)
        if self.results["tests_passed"] == self.results["tests_total"]:
            report.append("✅ BUILD VERIFICATION SUCCESSFUL")
        else:
            report.append("⚠️ BUILD VERIFICATION COMPLETED WITH ISSUES")

        report_text = "\n".join(report)

        # Save to file if requested
        if output_file:
            with open(output_file, 'w') as f:
                f.write(report_text)
            self.log(f"Report saved to: {output_file}")

        return report_text

    def run_full_verification(self) -> bool:
        """Run complete build verification"""
        self.log("Starting XpuMusic build verification...")
        self.log(f"Project root: {self.project_root}")

        # Verify build structure
        if not self.verify_build_structure():
            self.log("Build structure verification failed", "ERROR")
            return False

        # Check all required files
        self.check_required_files()

        # Run functional tests
        self.run_functional_tests()

        # Analyze performance
        self.analyze_performance()

        # Generate report
        report_file = self.build_dir / "verification_report.txt"
        self.generate_report(str(report_file))

        # Also save JSON results
        json_file = self.build_dir / "verification_results.json"
        with open(json_file, 'w') as f:
            json.dump(self.results, f, indent=2)

        # Return verdict
        return self.results["tests_passed"] == self.results["tests_total"]


def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(description="XpuMusic Build Verification Tool")
    parser.add_argument("--project-root", help="Path to XpuMusic project root")
    parser.add_argument("--report", help="Save report to file")
    parser.add_argument("--json", help="Save JSON results to file")

    args = parser.parse_args()

    verifier = BuildVerifier(args.project_root)

    success = verifier.run_full_verification()

    # Print summary
    print("\n" + "=" * 50)
    if success:
        print("✅ VERIFICATION SUCCESSFUL")
        sys.exit(0)
    else:
        print("⚠️ VERIFICATION COMPLETED WITH ISSUES")
        sys.exit(1)


if __name__ == "__main__":
    main()