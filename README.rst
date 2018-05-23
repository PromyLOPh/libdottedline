libdottedline
=============

Line coding algorithms. Currently implements 8b10b encoding in software only.

Tables must be generated with `python3 eightbtenb/__init__.py` first. The
CPython extension can be built with setup.py

To run the tests define the preprocessor symbol _TEST, compile src/8b10b.c and
run the resulting binary.

