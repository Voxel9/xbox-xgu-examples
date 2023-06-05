# Xbox xgu Sample Programs
Example GPU-accelerated graphics programs for the original Xbox using nxdk and xgu.

---

## Quick Guide
This quick guide assumes you've already cloned and setup nxdk, covered [here](https://github.com/XboxDev/nxdk/wiki/Getting-Started).

Clone this repository into the same directory which the nxdk directory resides in.  
You can use `git clone --recurse-submodules https://github.com/Voxel9/xbox-xgu-examples` to do this.  
Both the `nxdk` and `xbox-xgu-examples` directories should now be next to eachother.

**IMPORTANT:** Before compiling any examples, a tiny adjustment currently needs to be made to `external/xgu/xgu.h` and `external/xgu/xgux.h` to prevent linker errors.  
- In `xgu.h`, append `static inline` to the end of `#define XGU_API`.
- In `xgux.h`, append `static inline` to the end of `#define XGUX_API`.

Now `cd` to any example directory and run `make`.

Press the start button in any example to return to the dashboard.

---

## License
CC0 1.0 Universal.

Note: `common/swizzle.c` and `common/swizzle.h` are licensed under LGPL.
