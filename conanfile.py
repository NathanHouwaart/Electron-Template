from conan import ConanFile
from conan.tools.files import copy
import os

class MyAddonConan(ConanFile):
    name = "my_addon"
    version = "0.1"
    requires = ("etl/20.44.0", "tiny-aes-c/1.0.0")
    generators = ("CMakeDeps", "CMakeToolchain")
    settings = "os", "compiler", "build_type", "arch"

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
