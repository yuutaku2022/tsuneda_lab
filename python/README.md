# Python migration notes

This folder contains a Python port of the C++ workflow used for threshold-based embedding experiments.

## Requirements

Install dependencies with:

```powershell
& 'C:/Users/yuuta/.rye/py/cpython@3.12.7/python.exe' -m pip install -r ./python/requirements.txt
```

## Run

Quick smoke test:

```powershell
& 'C:/Users/yuuta/.rye/py/cpython@3.12.7/python.exe' './python/compare_th_if.py' --quick
```

Full run:

```powershell
& 'C:/Users/yuuta/.rye/py/cpython@3.12.7/python.exe' './python/compare_th_if.py'
```

Results are written under the output/cover_image_comparison_python directory.
