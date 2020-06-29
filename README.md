# xbox-xgu-examples
Examples for original Xbox using nxdk and xgu (name pending).

**Important:** As of latest xgu changes, a small workaround needs to be added to `xgu.h` and `xgux.h` in order to compile.  
At the top of both header files is the line `#define XGU_API`. Replace with `#define XGU_API inline`.  
Also, manually add the inline keyword to the `nv10_get_spot_coeff` function in `xgux.h`.  
_Hopefully_, this is only a temporary workaround and xgu functions get moved to proper .c source files instead.

Press the start button in any sample to return to the dashboard.

## License
CC0 1.0 Universal
