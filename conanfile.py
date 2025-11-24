from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps
import os

class SkyboltReflectConan(ConanFile):
	implements = ["auto_shared_fpic"]
	name = "skybolt-reflect"
	version = "1.0.0"
	settings = "os", "compiler", "arch", "build_type"
	options = {
		"fPIC": [True, False],
		"shared": [True, False]
	}
	default_options = {
		"fPIC": True,
		"shared": False
	}

	exports_sources = "*"
	no_copy_source = True

	def requirements(self):
		self.requires("catch2/2.13.8")

	def generate(self):
		tc = CMakeToolchain(self)
		
		tc.generate()

		deps = CMakeDeps(self)
		deps.generate()

	def build(self):
		cmake = CMake(self)
		cmake.configure(variables={"CMAKE_INSTALL_PREFIX": f"{self.build_folder}/Install"})
		cmake.build()
		
	def package(self):
		cmake = CMake(self)
		cmake.install()
		
	def package_info(self):
		self.cpp_info.includedirs = ["include"]
		self.cpp_info.libs = ["SkyboltReflect"]
