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

## Modules

### Storage Engine


#### Heap File

- Created with page size and initial num pages.
- Consists of header of fixed length and actual data pages afterwards.
- The header has 
  `{pageSize, numPages, freeSpaceMapLess33, freeSpaceMapLess66, freeSpaceMapLess100}`
- The page is:

```
[Header | SlotArray | TupleBytes]

Header = {NumSlots, WriteLsn}

SlotArray is 2 bytes(allows 16KB pages) and stores offsets.
An empty slot array stores 0.
Note that a slot array can have holes in future.

Tuple = {32 bit checksum, 32 bit attr1, 32 bit attr2, 32 bit xmin, 32 bit xmax}
Since we only support fixed size int, we just need to make sure all tuples are having right alignment and do padding.
For 4 bit ints, every tuple should start at 4byte boundary.

Since there are no updates or deletes, there is no
need to store pointer(page, slot) for previous versions of the
tuple right now.
```

- The header is mmaped and mlocked at time of heap file creation.

- Note that the header(md) at all levels need to be consistent
  with data and each entity should be self contained(e.g tuple/page/heap). The only challenge it poses is
  how to keep free spacemap bounded.
  It is stored as a bitmap for compactness with each being set indicates the pageId belongs to that. A 4Kb bitmap could store 32000 pages.
  We store the spacemap in multiple places of 4KB,
  at say offset [X, X+ 4kb], [X + 32000 * page_size ]
  However initially we would have only 1 such zone.
  When a page needs to move from one zone to other, we unset the bit in 1 and set in other and do msync for now
  do atmost 2 pageIO.

  Note: future is to `use WAL` for this.

- Note there is no support for double write buffer right now.

