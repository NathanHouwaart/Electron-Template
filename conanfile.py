from conan import ConanFile
from conan.tools.files import copy
import os

class MyAddonConan(ConanFile):
    name = "my_addon"
    version = "0.1"
    requires = ("etl/20.44.0", "tiny-aes-c/1.0.0")
    generators = ("CMakeDeps", "CMakeToolchain")
    settings = "os", "compiler", "build_type", "arch"

    def configure(self):
        # We only care about Windows builds using MSVC
        if self.settings.os == "Windows" and self.settings.compiler == "msvc":
            # For Release, RelWithDebInfo, and Debug builds, 
            # we want the DLL (Dynamic) runtime, which corresponds to /MD or /MDd.
            self.settings.compiler.runtime = "dynamic" 
            
            # Note: Conan 2.x will handle the switch between /MD (Release) 
            # and /MDd (Debug) automatically based on self.settings.build_type
            # when you use the 'dynamic' setting.

    def layout(self):
        self.folders.generators = os.path.join("build", "generators")

    def generate(self):
        dest = os.path.join("conan_includes")
        os.makedirs(dest, exist_ok=True)
        for dep in self.dependencies.values():
            for inc in dep.cpp_info.includedirs:
                src = os.path.join(dep.package_folder, inc)
                if os.path.exists(src):
                    copy(self, pattern="*.h", src=src, dst=os.path.join(dest, dep.ref.name))
