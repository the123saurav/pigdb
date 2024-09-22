[![Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=the123saurav_pigdb&metric=ncloc)](https://sonarcloud.io/summary/new_code?id=the123saurav_pigdb)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=the123saurav_pigdb&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=the123saurav_pigdb)
[![Technical Debt](https://sonarcloud.io/api/project_badges/measure?project=the123saurav_pigdb&metric=sqale_index)](https://sonarcloud.io/summary/new_code?id=the123saurav_pigdb)
[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=the123saurav_pigdb&metric=bugs)](https://sonarcloud.io/summary/new_code?id=the123saurav_pigdb)

# pigdb
A toy embedded relational embedded database.

## Local Development

The project uses vcpkg to manage dependencies and builds using cmake.
Additionaly it needs following tools:
- autoconf
- automake
- pkg-config

First install vcpkg:
```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
```

Integrate vcpkg with cmake:
```
./vcpkg integrate install
```

The dependencies are frozen at the vcpkg commit as specified by `builtin-baseline` in vcpkg.json.

To install dependencies locally, run in project root:
```
<path_to_vcpkg> install
```

run cmake from PROJECT_ROOT/build:
```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=/path_to_vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake ..
make
```