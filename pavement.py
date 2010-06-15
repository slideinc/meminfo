import errno
import os
from setuptools import Extension

from paver.easy import *
from paver.path import path
from paver.setuputils import setup


setup(
    name="meminfo",
    description="C extension for finding precise in-memory sizes of python objects",
    version="1.0.1",
    license="bsd",
    author="Libor Michalek",
    author_email="libor@pobox.com",
	py_modules=['meminfo'],
    ext_modules=[Extension(
        '_meminfo',
        ['_meminfomodule.c'],
        include_dirs=('.',),
        extra_compile_args=['-Wall'])],
    classifiers = [
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: BSD License",
        "Natural Language :: English",
        "Operating System :: Unix",
        "Programming Language :: C",
        "Topic :: System :: Monitoring",
    ]
)

MANIFEST = (
    "LICENSE",
    "setup.py",
    "paver-minilib.zip",
    "_meminfomodule.c",
)

@task
def manifest():
    path('MANIFEST.in').write_lines('include %s' % x for x in MANIFEST)

@task
@needs('generate_setup', 'minilib', 'manifest', 'setuptools.command.sdist')
def sdist():
    pass

@task
def clean():
    for p in map(path, ('meminfo.egg-info', 'dist', 'build', 'MANIFEST.in')):
        if p.exists():
            if p.isdir():
                p.rmtree()
            else:
                p.remove()
    for p in path(__file__).abspath().parent.walkfiles():
        if p.endswith(".pyc") or p.endswith(".pyo"):
            try:
                p.remove()
            except OSError, exc:
                if exc.args[0] == errno.EACCES:
                    continue
                raise
