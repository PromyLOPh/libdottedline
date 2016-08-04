from setuptools import setup, find_packages, Extension
setup(
    name = "libdottedline",
    version = "0.1",
    packages = ['eightbtenb'],
    ext_modules = [Extension('ceightbtenb', ['src/pythonmodule.c', 'src/8b10b.c'], extra_compile_args=['-std=c99', '-O3', '-march=native'])]
)

