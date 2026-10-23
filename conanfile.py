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

	def layout(self):
		cmake_layout(self)

		self.cpp.source.includedirs = ["src"]
		self.cpp.build.libdirs = [f"src/SkyboltReflect"]

	def generate(self):
		tc = CMakeToolchain(self)
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
