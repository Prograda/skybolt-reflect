from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
import os

class SkyboltReflectConan(ConanFile):
	implements = ["auto_shared_fpic"]
	name = "skybolt-reflect"
	version = "1.1.0"
	settings = "os", "compiler", "arch", "build_type"
	options = {
		"fPIC": [True, False],
		"shared": [True, False],
	}
	default_options = {
		"fPIC": True,
		"shared": False
	}

	exports_sources = "*"
	no_copy_source = True

	test_requires = "catch2/3.16.1"

	def _is_cmake_multi_generator(self) -> bool:
		gen = self.conf.get("tools.cmake.cmaketoolchain:generator")
		if gen:
			return "Visual" in gen or "Xcode" in gen or "Multi-Config" in gen
		else:
			compiler = self.settings.get_safe("compiler")
			if compiler == "msvc":
				return True
			else:
				return False

	def _build_type_path_if_multi_generator(self) -> bool:
		return f"{self.settings.build_type}" if self._is_cmake_multi_generator() else ""

	def layout(self):
		cmake_layout(self)

		self.cpp.source.includedirs = ["src"]
		self.cpp.build.libdirs = [f"src/SkyboltReflect/{self._build_type_path_if_multi_generator()}"]

	def generate(self):
		tc = CMakeToolchain(self)
		# catch_discover_tests should run just prior to tests as part of the host context to 
		# ensure the runtime dependencies are available to it.
		tc.variables["CMAKE_CATCH_DISCOVER_TESTS_DISCOVERY_MODE"] = "PRE_TEST"
		tc.generate()

		deps = CMakeDeps(self)
		deps.generate()

	def build(self):
		cmake = CMake(self)
		cmake.configure()
		cmake.build()
		# Use `tools.build:skip_test=True` configuration option to skip tests
		cmake.test()
		
	def package(self):
		cmake = CMake(self)
		cmake.install()
		
	def package_info(self):
		self.cpp_info.includedirs = ["include"]
		self.cpp_info.libs = ["SkyboltReflect"]
