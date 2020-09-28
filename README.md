# xbox-xgu-examples
Examples for original Xbox using nxdk and xgu (name pending).

---

## Quick Guide
This quick guide assumes you've already installed the dependencies for mainline nxdk, covered [here](https://github.com/XboxDev/nxdk/wiki/Install-the-Prerequisites).

These examples are based on [an in-development branch of nxdk](https://github.com/dracc/nxdk/tree/xgu_submodule) containing the upcoming GPU middleware library, xgu.  
Clone both this repository and said branch to the same directory. There should be 2 folders exactly next to eachother: `nxdk` and `xbox-xgu-examples`.

You can clone the in-dev branch like so: `git clone --recurse-submodules -b xgu_submodule https://github.com/dracc/nxdk`

**IMPORTANT:** Before compiling any examples, a tiny adjustment currently needs to be made to `xgu.h` and `xgux.h` (in `nxdk/lib/xgu`) to prevent linker errors.  
- In `xgu.h`, append `inline` to the end of `#define XGU_API`, near the top of the file.
- In `xgux.h`, append `inline` to the end of `#define XGUX_API`, also near the top of the file.

<sup>This isn't a very desirable fix for the hardcore developer as it still produces a few warnings in files that use these headers, but it at least prevents the duplicate symbol errors that occur if not added.  
There may be plans to migrate xgu functions from header files to source files in the future, which will remove the need for this workaround.</sup>

Now `cd` to any example directory and run `make`.

Press the start button in any example to return to the dashboard.

---

## License
CC0 1.0 Universal.

Note: `common/swizzle.c` and `common/swizzle.h` are licensed under LGPL.
